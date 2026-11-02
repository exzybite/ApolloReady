#pragma once
#include <d3d11.h>
#include <dxgi1_2.h>
#include <winrt/base.h>

namespace platf {
  bool is_running_as_system();
  int run_wgc_worker(int argc, char **argv);

  // The service retains encoding, application lifecycle and masks. Only WGC
  // runs in the console user's process, exchanging a keyed GPU texture through
  // inherited handles. No network listener or external executable is needed.
  class wgc_worker_capture {
  public:
    ~wgc_worker_capture();
    bool start(ID3D11Device *device, HWND target, unsigned width, unsigned height, DXGI_FORMAT format, bool cursor, const wchar_t *executable = nullptr);
    // 1: new frame; 0: waiting; -1: worker exited or GPU synchronization failed.
    int copy_frame(ID3D11DeviceContext *context, ID3D11Texture2D *destination);
    void set_cursor_visible(bool visible);
    HRESULT last_error = S_OK;
    LONG last_stage = 0;
  private:
    HANDLE process = nullptr, job = nullptr;
    HANDLE control_mapping = nullptr;
    volatile LONG *cursor_control = nullptr;
    winrt::com_ptr<ID3D11Texture2D> texture;
    winrt::com_ptr<IDXGIKeyedMutex> mutex;
  };
}
