/**
 * @file src/platform/windows/display_wgc.cpp
 * @brief Definitions for WinRT Windows.Graphics.Capture API
 */
// platform includes
#include <dxgi1_2.h>

// Gross hack to work around MINGW-packages#22160
#define ____FIReference_1_boolean_INTERFACE_DEFINED__

#include <windows.graphics.capture.interop.h>
#include <winrt/windows.foundation.h>
#include <winrt/windows.foundation.metadata.h>
#include <winrt/windows.graphics.directx.direct3d11.h>

// local includes
#include "display.h"
#include "misc.h"
#include "src/logging.h"
#include "src/config.h"
#include "apolloready_session.h"
#include "apolloready_mask.h"

namespace platf {
  using namespace std::literals;
}

namespace winrt {
  using namespace Windows::Foundation;
  using namespace Windows::Foundation::Metadata;
  using namespace Windows::Graphics::Capture;
  using namespace Windows::Graphics::DirectX::Direct3D11;

  extern "C" {
    HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(::IDXGIDevice *dxgiDevice, ::IInspectable **graphicsDevice);
  }

}  // namespace winrt
#include "apolloready_dxgi_access.h"

namespace platf::dxgi {
  wgc_capture_t::wgc_capture_t() {
    InitializeConditionVariable(&frame_present_cv);
  }

  wgc_capture_t::~wgc_capture_t() {
    if (window_mode) {
      reset_window_source();
      return;
    }
    if (capture_session) {
      capture_session.Close();
    }
    if (frame_pool) {
      frame_pool.Close();
    }
    item = nullptr;
    capture_session = nullptr;
    frame_pool = nullptr;
  }

  /**
   * @brief Initialize the Windows.Graphics.Capture backend.
   * @return 0 on success, -1 on failure.
   */
  int wgc_capture_t::init(display_base_t *display, const ::video::config_t &config) {
    HRESULT status;
    dxgi::dxgi_t dxgi;
    winrt::com_ptr<::IInspectable> d3d_comhandle;
    window_mode = config::video.apolloready_masks;
    window_service_mode = window_mode && platf::is_running_as_system();
    if (!window_service_mode) {
    try {
      if (!winrt::GraphicsCaptureSession::IsSupported()) {
        BOOST_LOG(error) << "Screen capture is not supported on this device for this release of Windows!"sv;
        return -1;
      }
      if (FAILED(status = display->device->QueryInterface(IID_IDXGIDevice, (void **) &dxgi))) {
        BOOST_LOG(error) << "Failed to query DXGI interface from device [0x"sv << util::hex(status).to_string_view() << ']';
        return -1;
      }
      if (FAILED(status = winrt::CreateDirect3D11DeviceFromDXGIDevice(*&dxgi, d3d_comhandle.put()))) {
        BOOST_LOG(error) << "Failed to query WinRT DirectX interface from device [0x"sv << util::hex(status).to_string_view() << ']';
        return -1;
      }
    } catch (winrt::hresult_error &e) {
      BOOST_LOG(error) << "Screen capture is not supported on this device for this release of Windows: failed to acquire device: [0x"sv << util::hex(e.code()).to_string_view() << ']';
      return -1;
    }
    }

    DXGI_OUTPUT_DESC output_desc;
    if (d3d_comhandle) uwp_device = d3d_comhandle.as<winrt::IDirect3DDevice>();
    display->output->GetDesc(&output_desc);

    window_mode = config::video.apolloready_masks;
    if (window_mode) {
      window_display = display;
      window_monitor = output_desc.Monitor;
      display->capture_format = config.dynamicRange ? DXGI_FORMAT_R16G16B16A16_FLOAT : DXGI_FORMAT_B8G8R8A8_UNORM;
      if (display->display_rotation != DXGI_MODE_ROTATION_IDENTITY && display->display_rotation != DXGI_MODE_ROTATION_UNSPECIFIED) {
        BOOST_LOG(error) << "ApolloReady: rotated outputs are not supported by the experimental window backend";
        return -1;
      }
      D3D11_TEXTURE2D_DESC texture {};
      texture.Width = display->width_before_rotation;
      texture.Height = display->height_before_rotation;
      texture.MipLevels = texture.ArraySize = texture.SampleDesc.Count = 1;
      texture.Format = display->capture_format;
      texture.Usage = D3D11_USAGE_DEFAULT;
      texture.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
      if (FAILED(display->device->CreateTexture2D(&texture, nullptr, &window_output)) ||
          FAILED(display->device->CreateRenderTargetView(window_output.get(), nullptr, &window_output_rt))) return -1;
      mask_window_output();
      BOOST_LOG(info) << "ApolloReady: experimental window-only capture enabled; monitor fallback disabled";
      return 0;
    }

    auto monitor_factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    if (monitor_factory == nullptr ||
        FAILED(status = monitor_factory->CreateForMonitor(output_desc.Monitor, winrt::guid_of<winrt::IGraphicsCaptureItem>(), winrt::put_abi(item)))) {
      BOOST_LOG(error) << "Screen capture is not supported on this device for this release of Windows: failed to acquire display: [0x"sv << util::hex(status).to_string_view() << ']';
      return -1;
    }

    if (config.dynamicRange) {
      display->capture_format = DXGI_FORMAT_R16G16B16A16_FLOAT;
    } else {
      display->capture_format = DXGI_FORMAT_B8G8R8A8_UNORM;
    }

    try {
      frame_pool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(uwp_device, static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(display->capture_format), 2, item.Size());
      capture_session = frame_pool.CreateCaptureSession(item);
      frame_pool.FrameArrived({this, &wgc_capture_t::on_frame_arrived});
    } catch (winrt::hresult_error &e) {
      BOOST_LOG(error) << "Screen capture is not supported on this device for this release of Windows: failed to create capture session: [0x"sv << util::hex(e.code()).to_string_view() << ']';
      return -1;
    }
    try {
      if (winrt::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsBorderRequired")) {
        capture_session.IsBorderRequired(false);
      } else {
        BOOST_LOG(warning) << "Can't disable colored border around capture area on this version of Windows";
      }
    } catch (winrt::hresult_error &e) {
      BOOST_LOG(warning) << "Screen capture may not be fully supported on this device for this release of Windows: failed to disable border around capture area: [0x"sv << util::hex(e.code()).to_string_view() << ']';
    }
    try {
      if (winrt::ApiInformation::IsPropertyPresent(L"Windows.Graphics.Capture.GraphicsCaptureSession", L"MinUpdateInterval")) {
        capture_session.MinUpdateInterval(winrt::TimeSpan{ 10000000 / (config.framerate * 2) });
      }
      else {
        BOOST_LOG(warning) << "Can't set MinUpdateInterval";
      }
    }
    catch (winrt::hresult_error &e) {
      BOOST_LOG(warning) << "Screen capture may not be fully supported on this device for this release of Windows: failed to set MinUpdateInterval: [0x"sv << util::hex(e.code()).to_string_view() << ']';
    }
    try {
      capture_session.StartCapture();
    } catch (winrt::hresult_error &e) {
      BOOST_LOG(error) << "Screen capture is not supported on this device for this release of Windows: failed to start capture: [0x"sv << util::hex(e.code()).to_string_view() << ']';
      return -1;
    }
    return 0;
  }

  void wgc_capture_t::mask_window_output() {
    auto session = apolloready::sessions.read();
    bool closing = session.status == apolloready::phase::closing;
    bool paused = window_was_hidden;
    auto now = std::chrono::steady_clock::now();
    if (!window_has_image && window_mask_closing == closing && window_mask_paused == paused &&
        now - window_mask_updated < 33ms) return;
    if (window_output_rt) {
      const float color[] = {0.008f, 0.012f, 0.02f, 1.0f};
      window_display->device_ctx->ClearRenderTargetView(window_output_rt.get(), color);
    }
    auto pixels = apolloready::render_mask(window_display->width_before_rotation,
      window_display->height_before_rotation, closing, paused, config::sunshine.locale,
      static_cast<unsigned>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 3000) / 33);
    if (!pixels.empty()) {
      if (window_display->capture_format == DXGI_FORMAT_B8G8R8A8_UNORM) {
        window_display->device_ctx->UpdateSubresource(window_output.get(), 0, nullptr, pixels.data(), window_display->width_before_rotation * 4, 0);
      } else if (window_display->capture_format == DXGI_FORMAT_R16G16B16A16_FLOAT) {
        auto hdr = apolloready::mask_to_scrgb(pixels);
        window_display->device_ctx->UpdateSubresource(window_output.get(), 0, nullptr, hdr.data(), window_display->width_before_rotation * 8, 0);
      }
    }
    window_has_image = false;
    window_mask_closing = closing;
    window_mask_paused = paused;
    window_mask_updated = now;
    apolloready::sessions.publish(window_generation, apolloready::phase::waiting);
  }

  void wgc_capture_t::reset_window_source() {
    window_worker.reset();
    // The Closed callback owns only this flag, never a raw pointer to the duplicator.
    window_closed = std::make_shared<std::atomic_bool>(false);
    try { if (capture_session) capture_session.Close(); } catch (winrt::hresult_error const &) {}
    try { if (frame_pool) frame_pool.Close(); } catch (winrt::hresult_error const &) {}
    capture_session = nullptr;
    frame_pool = nullptr;
    item = nullptr;
    if (window_process) CloseHandle(window_process);
    window_process = nullptr;
    window_target = nullptr;
    window_has_image = false;
  }

  namespace {
    std::uint64_t creation_time(HANDLE process) {
      FILETIME created, exited, kernel, user;
      if (!GetProcessTimes(process, &created, &exited, &kernel, &user)) return 0;
      return (std::uint64_t(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    }
  }

  bool wgc_capture_t::bind_window_source() {
    auto session = apolloready::sessions.read();
    if (!session.active || session.generation != window_generation || session.status == apolloready::phase::closing) return false;
    HWND target = reinterpret_cast<HWND>(session.window);
    DWORD pid = session.process_id;
    if (!target) {
      auto eligible = [&](HWND candidate) {
        RECT rect;
        MONITORINFO monitor {sizeof(MONITORINFO)};
        DWORD cloaked = 0, owner = 0;
        GetWindowThreadProcessId(candidate, &owner);
        return candidate && owner && owner != GetCurrentProcessId() && IsWindowVisible(candidate) && !IsIconic(candidate) &&
          !(GetWindowLongPtr(candidate, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) &&
          MonitorFromWindow(candidate, MONITOR_DEFAULTTONULL) == window_monitor &&
          SUCCEEDED(DwmGetWindowAttribute(candidate, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && !cloaked &&
          GetWindowRect(candidate, &rect) && GetMonitorInfo(window_monitor, &monitor) &&
          rect.left <= monitor.rcMonitor.left && rect.top <= monitor.rcMonitor.top &&
          rect.right >= monitor.rcMonitor.right && rect.bottom >= monitor.rcMonitor.bottom;
      };
      // A fast game can lose foreground during stream/display initialization.
      // Prefer a qualifying window owned by the exact process we just launched.
      // Launchers without a game window retain the existing foreground heuristic.
      target = GetForegroundWindow();
      DWORD foreground_pid = 0;
      GetWindowThreadProcessId(target, &foreground_pid);
      if (session.launched_process_id && (foreground_pid != session.launched_process_id || !eligible(target))) {
        struct search_t {
          DWORD pid;
          decltype(eligible) *accept;
          HWND found = nullptr;
        } search {session.launched_process_id, &eligible};
        EnumWindows([](HWND candidate, LPARAM context) -> BOOL {
          auto &search = *reinterpret_cast<search_t *>(context);
          DWORD owner = 0;
          GetWindowThreadProcessId(candidate, &owner);
          if (owner == search.pid && (*search.accept)(candidate)) {
            search.found = candidate;
            return FALSE;
          }
          return TRUE;
        }, reinterpret_cast<LPARAM>(&search));
        if (search.found) target = search.found;
      }
      auto now = std::chrono::steady_clock::now();
      if (target != window_candidate) {
        window_candidate = target;
        window_candidate_since = now;
      }
      if (!eligible(target)) {
        window_candidate_since = now;
        return false;
      }
      if (now - window_candidate_since < 300ms) return false;
      GetWindowThreadProcessId(target, &pid);
    }
    if (session.window) {
      DWORD owner = 0;
      GetWindowThreadProcessId(target, &owner);
      if (!IsWindow(target) || owner != pid) {
        apolloready::sessions.publish(window_generation, apolloready::phase::closing);
        return false;
      }
    }
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return false;
    auto created = creation_time(process);
    auto close_process = util::fail_guard([&]() { if (process) CloseHandle(process); });
    if (!created || created < session.started || WaitForSingleObject(process, 0) != WAIT_TIMEOUT) return false;
    if (session.window) {
      DWORD owner = 0;
      GetWindowThreadProcessId(target, &owner);
      if (!IsWindow(target) || owner != pid || created != session.process_started) {
        apolloready::sessions.publish(window_generation, apolloready::phase::closing);
        return false;
      }
    } else if (!apolloready::sessions.bind(window_generation, reinterpret_cast<std::uintptr_t>(target), pid, created)) {
      return false;
    }
    window_last_attempt = std::chrono::steady_clock::now();
    try {
      if (window_service_mode) {
        auto worker = std::make_unique<platf::wgc_worker_capture>();
        if (!worker->start(window_display->device.get(), target, window_display->width_before_rotation,
              window_display->height_before_rotation, window_display->capture_format, window_cursor_visible)) {
          BOOST_LOG(error) << "ApolloReady: could not start user-session capture [0x" << std::hex
            << static_cast<unsigned>(worker->last_error) << std::dec << ']';
          return false;
        }
        window_worker = std::move(worker);
        window_target = target;
        window_process = process;
        process = nullptr;
        BOOST_LOG(info) << "ApolloReady: user-session window source selected, PID " << pid;
        return true;
      }
      auto factory = winrt::get_activation_factory<winrt::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
      winrt::check_hresult(factory->CreateForWindow(target, winrt::guid_of<winrt::IGraphicsCaptureItem>(), winrt::put_abi(item)));
      window_pool_size = item.Size();
      if (window_pool_size.Width <= 0 || window_pool_size.Height <= 0) return false;
      auto flag = window_closed;
      item.Closed([flag](auto const &, auto const &) { flag->store(true); });
      frame_pool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(uwp_device,
        static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(window_display->capture_format), 2, window_pool_size);
      capture_session = frame_pool.CreateCaptureSession(item);
      capture_session.IsCursorCaptureEnabled(window_cursor_visible);
      capture_session.StartCapture();
      window_target = target;
      window_process = process;
      process = nullptr;
      BOOST_LOG(info) << "ApolloReady: window source selected, PID " << pid
        << (pid == session.launched_process_id ? " (launched process)" : " (foreground heuristic)")
        << "; waiting for its first frame";
      return true;
    } catch (winrt::hresult_error const &error) {
      BOOST_LOG(warning) << "ApolloReady: window capture unavailable [" << util::hex(error.code()).to_string_view() << "]; remaining masked";
      reset_window_source();
      return false;
    }
  }

  capture_e wgc_capture_t::next_window_frame(ID3D11Texture2D **out, uint64_t &out_time) {
    // The only possible output is our own mask texture or a copy of the bound HWND.
    // In particular this path never calls CreateForMonitor or Desktop Duplication.
    auto output = [&]() {
      *out = window_output.get();
      (*out)->AddRef();
      out_time = qpc_counter();
      return capture_e::ok;
    };
    auto session = apolloready::sessions.read();
    if (session.generation != window_generation) {
      reset_window_source();
      window_candidate = nullptr;
      window_last_attempt = {};
      window_generation = session.generation;
      window_was_hidden = false;
      mask_window_output();
    }
    if (!session.active || session.status == apolloready::phase::closing) {
      mask_window_output();
      return output();
    }
    if (!capture_session && !window_worker && std::chrono::steady_clock::now() - window_last_attempt >= 1s) bind_window_source();
    if (!capture_session && !window_worker) { mask_window_output(); return output(); }
    auto valid_source = [&]() {
      DWORD owner = 0;
      GetWindowThreadProcessId(window_target, &owner);
      auto current = apolloready::sessions.read();
      return current.active && current.generation == window_generation && current.status != apolloready::phase::closing &&
        !window_closed->load() && IsWindow(window_target) && owner == current.process_id &&
        window_process && WaitForSingleObject(window_process, 0) == WAIT_TIMEOUT;
    };
    auto close_source = [&]() {
      if (apolloready::sessions.publish(window_generation, apolloready::phase::closing))
        BOOST_LOG(info) << "ApolloReady: closing - source invalid, switching to mask";
      mask_window_output();
      reset_window_source();
    };
    if (!valid_source()) { close_source(); return output(); }
    if (IsIconic(window_target) || !IsWindowVisible(window_target)) {
      window_was_hidden = true;
      mask_window_output();
      return output();  // Losing focus/minimizing never ends a session.
    }
    if (window_was_hidden) {
      // Exclusive fullscreen can leave a capture pool stalled after restore.
      // Reopen only the already bound HWND, retaining PID and launch checks.
      window_was_hidden = false;
      mask_window_output();
      reset_window_source();
      window_last_attempt = {};
      BOOST_LOG(info) << "ApolloReady: restored window - reopening its capture";
      return output();
    }
    try {
      if (window_worker) {
        const auto copied = window_worker->copy_frame(window_display->device_ctx.get(), window_output.get());
        if (copied < 0) {
          BOOST_LOG(warning) << "ApolloReady: window capture worker ended [0x" << std::hex
            << static_cast<unsigned>(window_worker->last_error) << std::dec << ", stage " << window_worker->last_stage << "]; remaining masked";
          mask_window_output();reset_window_source();return output();
        }
        if (copied == 0) return output();
        if (!valid_source()) { close_source(); return output(); }
        if (!apolloready::sessions.publish(window_generation, apolloready::phase::ready)) {
          mask_window_output();return output();
        }
        if (!window_has_image) BOOST_LOG(info) << "ApolloReady: ready - first usable user-session window frame";
        window_has_image = true;
        return output();
      }
      auto frame = frame_pool.TryGetNextFrame();
      if (!frame) return output();  // A static image is safe to repeat: it is window content.
      auto close_frame = util::fail_guard([&]() { if (frame) frame.Close(); });
      auto size = frame.ContentSize();
      if (size.Width != window_pool_size.Width || size.Height != window_pool_size.Height) {
        mask_window_output();
        if (size.Width > 0 && size.Height > 0) {
          frame.Close();
          frame = nullptr;
          frame_pool.Recreate(uwp_device,
            static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(window_display->capture_format), 2, size);
          window_pool_size = size;
        }
        return output();
      }
      texture2d_t source;
      auto access = frame.Surface().as<winrt::IDirect3DDxgiInterfaceAccess>();
      winrt::check_hresult(access->GetInterface(IID_ID3D11Texture2D, reinterpret_cast<void **>(&source)));
      D3D11_TEXTURE2D_DESC desc;
      source->GetDesc(&desc);
      if (desc.Width != window_display->width_before_rotation || desc.Height != window_display->height_before_rotation ||
          size.Width != static_cast<int>(desc.Width) || size.Height != static_cast<int>(desc.Height) ||
          desc.Format != window_display->capture_format) {
        mask_window_output();
        return output();  // First version accepts exact output-sized sources only.
      }
      window_display->device_ctx->CopyResource(window_output.get(), source.get());
      if (!valid_source()) { close_source(); return output(); }
      if (!apolloready::sessions.publish(window_generation, apolloready::phase::ready)) {
        mask_window_output();
        return output();
      }
      if (!window_has_image) BOOST_LOG(info) << "ApolloReady: ready - first usable window frame";
      window_has_image = true;
    } catch (winrt::hresult_error const &error) {
      BOOST_LOG(warning) << "ApolloReady: window frame unavailable [" << util::hex(error.code()).to_string_view() << "]; masking";
      mask_window_output();
      reset_window_source();  // Retry the same bound HWND; never discover a replacement.
    }
    return output();
  }

  /**
   * This function runs in a separate thread spawned by the frame pool and is a producer of frames.
   * To maintain parity with the original display interface, this frame will be consumed by the capture thread.
   * Acquire a read-write lock, make the produced frame available to the capture thread, then wake the capture thread.
   */
  void wgc_capture_t::on_frame_arrived(winrt::Direct3D11CaptureFramePool const &sender, winrt::IInspectable const &) {
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFrame frame {nullptr};
    try {
      frame = sender.TryGetNextFrame();
    } catch (winrt::hresult_error &e) {
      BOOST_LOG(warning) << "Failed to capture frame: "sv << e.code();
      return;
    }
    if (frame != nullptr) {
      AcquireSRWLockExclusive(&frame_lock);
      if (produced_frame) {
        produced_frame.Close();
      }

      produced_frame = frame;
      ReleaseSRWLockExclusive(&frame_lock);
      WakeConditionVariable(&frame_present_cv);
    }
  }

  /**
   * @brief Get the next frame from the producer thread.
   * If not available, the capture thread blocks until one is, or the wait times out.
   * @param timeout how long to wait for the next frame
   * @param out a texture containing the frame just captured
   * @param out_time the timestamp of the frame just captured
   */
  capture_e wgc_capture_t::next_frame(std::chrono::milliseconds timeout, ID3D11Texture2D **out, uint64_t &out_time) {
    if (window_mode) return next_window_frame(out, out_time);
    // this CONSUMER runs in the capture thread
    release_frame();

    AcquireSRWLockExclusive(&frame_lock);
    if (produced_frame == nullptr && SleepConditionVariableSRW(&frame_present_cv, &frame_lock, timeout.count(), 0) == 0) {
      ReleaseSRWLockExclusive(&frame_lock);
      if (GetLastError() == ERROR_TIMEOUT) {
        return capture_e::timeout;
      } else {
        return capture_e::error;
      }
    }
    if (produced_frame) {
      consumed_frame = produced_frame;
      produced_frame = nullptr;
    }
    ReleaseSRWLockExclusive(&frame_lock);
    if (consumed_frame == nullptr) {  // spurious wakeup
      return capture_e::timeout;
    }

    auto capture_access = consumed_frame.Surface().as<winrt::IDirect3DDxgiInterfaceAccess>();
    if (capture_access == nullptr) {
      return capture_e::error;
    }
    capture_access->GetInterface(IID_ID3D11Texture2D, (void **) out);
    out_time = consumed_frame.SystemRelativeTime().count();  // raw ticks from query performance counter
    return capture_e::ok;
  }

  capture_e wgc_capture_t::release_frame() {
    if (consumed_frame != nullptr) {
      consumed_frame.Close();
      consumed_frame = nullptr;
    }
    return capture_e::ok;
  }

  int wgc_capture_t::set_cursor_visible(bool x) {
    window_cursor_visible = x;
    if (window_worker) window_worker->set_cursor_visible(x);
    if (window_mode && !capture_session) return 0;
    try {
      if (capture_session.IsCursorCaptureEnabled() != x) {
        capture_session.IsCursorCaptureEnabled(x);
      }
      return 0;
    } catch (winrt::hresult_error &) {
      return -1;
    }
  }

  int display_wgc_ram_t::init(const ::video::config_t &config, const std::string &display_name) {
    if (display_base_t::init(config, display_name) || dup.init(this, config)) {
      return -1;
    }

    texture.reset();
    return 0;
  }

  /**
   * @brief Get the next frame from the Windows.Graphics.Capture API and copy it into a new snapshot texture.
   * @param pull_free_image_cb call this to get a new free image from the video subsystem.
   * @param img_out the captured frame is returned here
   * @param timeout how long to wait for the next frame
   * @param cursor_visible whether to capture the cursor
   */
  capture_e display_wgc_ram_t::snapshot(const pull_free_image_cb_t &pull_free_image_cb, std::shared_ptr<platf::img_t> &img_out, std::chrono::milliseconds timeout, bool cursor_visible) {
    HRESULT status;
    texture2d_t src;
    uint64_t frame_qpc;
    dup.set_cursor_visible(cursor_visible);
    auto capture_status = dup.next_frame(timeout, &src, frame_qpc);
    if (capture_status != capture_e::ok) {
      return capture_status;
    }

    auto frame_timestamp = std::chrono::steady_clock::now() - qpc_time_difference(qpc_counter(), frame_qpc);
    D3D11_TEXTURE2D_DESC desc;
    src->GetDesc(&desc);

    // Create the staging texture if it doesn't exist. It should match the source in size and format.
    if (texture == nullptr) {
      capture_format = desc.Format;
      BOOST_LOG(info) << "Capture format ["sv << dxgi_format_to_string(capture_format) << ']';

      D3D11_TEXTURE2D_DESC t {};
      t.Width = width;
      t.Height = height;
      t.MipLevels = 1;
      t.ArraySize = 1;
      t.SampleDesc.Count = 1;
      t.Usage = D3D11_USAGE_STAGING;
      t.Format = capture_format;
      t.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

      auto status = device->CreateTexture2D(&t, nullptr, &texture);

      if (FAILED(status)) {
        BOOST_LOG(error) << "Failed to create staging texture [0x"sv << util::hex(status).to_string_view() << ']';
        return capture_e::error;
      }
    }

    // It's possible for our display enumeration to race with mode changes and result in
    // mismatched image pool and desktop texture sizes. If this happens, just reinit again.
    if (desc.Width != width || desc.Height != height) {
      BOOST_LOG(info) << "Capture size changed ["sv << width << 'x' << height << " -> "sv << desc.Width << 'x' << desc.Height << ']';
      return capture_e::reinit;
    }
    // It's also possible for the capture format to change on the fly. If that happens,
    // reinitialize capture to try format detection again and create new images.
    if (capture_format != desc.Format) {
      BOOST_LOG(info) << "Capture format changed ["sv << dxgi_format_to_string(capture_format) << " -> "sv << dxgi_format_to_string(desc.Format) << ']';
      return capture_e::reinit;
    }

    // Copy from GPU to CPU
    device_ctx->CopyResource(texture.get(), src.get());

    if (!pull_free_image_cb(img_out)) {
      return capture_e::interrupted;
    }
    auto img = (img_t *) img_out.get();

    // Map the staging texture for CPU access (making it inaccessible for the GPU)
    if (FAILED(status = device_ctx->Map(texture.get(), 0, D3D11_MAP_READ, 0, &img_info))) {
      BOOST_LOG(error) << "Failed to map texture [0x"sv << util::hex(status).to_string_view() << ']';

      return capture_e::error;
    }

    // Now that we know the capture format, we can finish creating the image
    if (complete_img(img, false)) {
      device_ctx->Unmap(texture.get(), 0);
      img_info.pData = nullptr;
      return capture_e::error;
    }

    std::copy_n((std::uint8_t *) img_info.pData, height * img_info.RowPitch, (std::uint8_t *) img->data);

    // Unmap the staging texture to allow GPU access again
    device_ctx->Unmap(texture.get(), 0);
    img_info.pData = nullptr;

    if (img) {
      img->frame_timestamp = frame_timestamp;
    }

    return capture_e::ok;
  }

  capture_e display_wgc_ram_t::release_snapshot() {
    return dup.release_frame();
  }
}  // namespace platf::dxgi
