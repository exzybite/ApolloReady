#include "apolloready_wgc_worker.h"
#include "apolloready_dxgi_access.h"
#include <d3d11_1.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#define ____FIReference_1_boolean_INTERFACE_DEFINED__
#include <windows.graphics.capture.interop.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

extern "C" HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(IDXGIDevice *, IInspectable **);

namespace {
  struct handle_owner {
    HANDLE value = nullptr;
    ~handle_owner() { if(value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
  };
  struct mapped_view {
    void *value = nullptr;
    ~mapped_view() { if(value) UnmapViewOfFile(value); }
  };
}

namespace platf {
  wgc_worker_capture::~wgc_worker_capture() {
    if (process) {
      // This is our private capture worker, never the game process.
      if (WaitForSingleObject(process, 0) == WAIT_TIMEOUT) TerminateProcess(process, 0);
      WaitForSingleObject(process, 3000);
      CloseHandle(process);
    }
    if (job) CloseHandle(job);
    if (cursor_control) UnmapViewOfFile(const_cast<LONG *>(cursor_control));
    if (control_mapping) CloseHandle(control_mapping);
  }

  bool wgc_worker_capture::start(ID3D11Device *device, HWND target, unsigned width, unsigned height, DXGI_FORMAT format, bool cursor, const wchar_t *executable) {
    try {
      D3D11_TEXTURE2D_DESC desc{};
      desc.Width=width;desc.Height=height;desc.Format=format;
      desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;
      desc.BindFlags=D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
      desc.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
      winrt::check_hresult(device->CreateTexture2D(&desc,nullptr,texture.put()));
      mutex=texture.as<IDXGIKeyedMutex>();
      auto resource=texture.as<IDXGIResource1>();
      SECURITY_ATTRIBUTES security{sizeof(SECURITY_ATTRIBUTES),nullptr,TRUE};
      handle_owner shared;
      winrt::check_hresult(resource->CreateSharedHandle(&security,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,nullptr,&shared.value));
      control_mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,&security,PAGE_READWRITE,0,2*sizeof(LONG),nullptr);
      if(!control_mapping)winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
      cursor_control=static_cast<volatile LONG *>(MapViewOfFile(control_mapping,FILE_MAP_ALL_ACCESS,0,0,2*sizeof(LONG)));
      if(!cursor_control)winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
      set_cursor_visible(cursor);
      winrt::com_ptr<IDXGIDevice> dxgiDevice;winrt::check_hresult(device->QueryInterface(IID_PPV_ARGS(dxgiDevice.put())));
      winrt::com_ptr<IDXGIAdapter> adapter;winrt::check_hresult(dxgiDevice->GetAdapter(adapter.put()));
      DXGI_ADAPTER_DESC adapterDesc{};winrt::check_hresult(adapter->GetDesc(&adapterDesc));
      DWORD session=0;handle_owner token;
      const bool serviceMode=is_running_as_system();
      if (!ProcessIdToSessionId(GetCurrentProcessId(),&session) ||
          !(serviceMode ? WTSQueryUserToken(session,&token.value) : OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY|TOKEN_DUPLICATE,&token.value)))
        winrt::throw_hresult(HRESULT_FROM_WIN32(GetLastError()));
      wchar_t exe[MAX_PATH];if(!GetModuleFileNameW(nullptr,exe,MAX_PATH))return false;
      const auto workerExecutable=executable?executable:exe;
      std::wstring command=L"\""+std::wstring(workerExecutable)+L"\" --apolloready-wgc-worker "+
        std::to_wstring(reinterpret_cast<std::uintptr_t>(shared.value))+L" "+
        std::to_wstring(reinterpret_cast<std::uintptr_t>(target))+L" "+
        std::to_wstring(adapterDesc.AdapterLuid.LowPart)+L" "+
        std::to_wstring(adapterDesc.AdapterLuid.HighPart)+L" "+std::to_wstring(cursor?1:0)+L" "+
        std::to_wstring(reinterpret_cast<std::uintptr_t>(control_mapping));
      SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&bytes);
      std::vector<unsigned char> storage(bytes);
      auto attributes=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
      if(!InitializeProcThreadAttributeList(attributes,1,0,&bytes))return false;
      HANDLE inherited[]={shared.value,control_mapping};
      const bool attributeOk=UpdateProcThreadAttribute(attributes,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,inherited,sizeof(inherited),nullptr,nullptr);
      STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.StartupInfo.lpDesktop=const_cast<LPWSTR>(L"winsta0\\default");startup.lpAttributeList=attributes;
      job=CreateJobObjectW(nullptr,nullptr);
      JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};limits.BasicLimitInformation.LimitFlags=JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
      if (!job || !SetInformationJobObject(job,JobObjectExtendedLimitInformation,&limits,sizeof(limits))) {
        DeleteProcThreadAttributeList(attributes);return false;
      }
      void *environment=nullptr;CreateEnvironmentBlock(&environment,token.value,FALSE);
      PROCESS_INFORMATION child{};
      constexpr auto flags=CREATE_UNICODE_ENVIRONMENT|CREATE_NO_WINDOW|CREATE_SUSPENDED|EXTENDED_STARTUPINFO_PRESENT;
      const bool created=attributeOk && (serviceMode ?
        CreateProcessAsUserW(token.value,workerExecutable,command.data(),nullptr,nullptr,TRUE,flags,environment,nullptr,&startup.StartupInfo,&child) :
        CreateProcessW(workerExecutable,command.data(),nullptr,nullptr,TRUE,flags,environment,nullptr,&startup.StartupInfo,&child));
      const auto error=GetLastError();
      if(environment)DestroyEnvironmentBlock(environment);
      DeleteProcThreadAttributeList(attributes);
      if(!created)winrt::throw_hresult(HRESULT_FROM_WIN32(error));
      process=child.hProcess;
      if(!AssignProcessToJobObject(job,process)) {
        TerminateProcess(process,1);CloseHandle(child.hThread);return false;
      }
      ResumeThread(child.hThread);CloseHandle(child.hThread);
      return true;
    } catch(winrt::hresult_error const &e) {
      last_error = e.code().value;
      return false;
    }
  }

  void wgc_worker_capture::set_cursor_visible(bool visible) {
    if(cursor_control) InterlockedExchange(cursor_control,visible?1:0);
  }

  int wgc_worker_capture::copy_frame(ID3D11DeviceContext *context, ID3D11Texture2D *destination) {
    if(cursor_control) last_stage=cursor_control[1];
    if(!process || WaitForSingleObject(process,0)!=WAIT_TIMEOUT) {
      DWORD exit=0;if(process)GetExitCodeProcess(process,&exit);
      last_error=static_cast<HRESULT>(exit);return -1;
    }
    const auto status=mutex->AcquireSync(1,0);
    if(status==WAIT_TIMEOUT)return 0;
    if(status!=S_OK){last_error=status;return -1;}
    context->CopyResource(destination,texture.get());
    context->Flush();
    last_error=mutex->ReleaseSync(0);
    return SUCCEEDED(last_error)?1:-1;
  }

  int run_wgc_worker(int argc,char **argv) {
    // Internal mode accepts only inherited handles and a single HWND. It never
    // loads host configuration, credentials or creates HTTP/stream listeners.
    if(argc!=8)return 2;
    try {
      handle_owner shared{reinterpret_cast<HANDLE>(std::stoull(argv[2]))};
      handle_owner controlHandle{reinterpret_cast<HANDLE>(std::stoull(argv[7]))};
      mapped_view control{MapViewOfFile(controlHandle.value,FILE_MAP_ALL_ACCESS,0,0,2*sizeof(LONG))};
      if(!control.value)return 7;
      const auto cursorControl=static_cast<const volatile LONG *>(control.value);
      auto stage=static_cast<volatile LONG *>(control.value)+1;
      InterlockedExchange(stage,1);
      winrt::init_apartment(winrt::apartment_type::multi_threaded);
      InterlockedExchange(stage,2);
      const auto target=reinterpret_cast<HWND>(std::stoull(argv[3]));
      const LUID luid{static_cast<DWORD>(std::stoul(argv[4])),static_cast<LONG>(std::stol(argv[5]))};
      if(!IsWindow(target))return 3;
      DWORD targetPid=0;GetWindowThreadProcessId(target,&targetPid);
      handle_owner targetProcess{OpenProcess(SYNCHRONIZE,FALSE,targetPid)};
      if(!targetProcess.value)return 4;
      winrt::com_ptr<IDXGIFactory1> factory;winrt::check_hresult(CreateDXGIFactory1(IID_PPV_ARGS(factory.put())));
      winrt::com_ptr<IDXGIAdapter1> adapter;
      for(UINT index=0;;index++) {
        winrt::com_ptr<IDXGIAdapter1> candidate;
        const auto status=factory->EnumAdapters1(index,candidate.put());
        if(status==DXGI_ERROR_NOT_FOUND)break;
        winrt::check_hresult(status);
        DXGI_ADAPTER_DESC1 info{};candidate->GetDesc1(&info);
        if(info.AdapterLuid.LowPart==luid.LowPart && info.AdapterLuid.HighPart==luid.HighPart){adapter=std::move(candidate);break;}
      }
      if(!adapter)return 5;
      InterlockedExchange(stage,3);
      winrt::com_ptr<ID3D11Device> device;winrt::com_ptr<ID3D11DeviceContext> context;
      winrt::check_hresult(D3D11CreateDevice(adapter.get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,device.put(),nullptr,context.put()));
      auto device1=device.as<ID3D11Device1>();winrt::com_ptr<ID3D11Texture2D> output;
      winrt::check_hresult(device1->OpenSharedResource1(shared.value,IID_PPV_ARGS(output.put())));
      InterlockedExchange(stage,4);
      auto mutex=output.as<IDXGIKeyedMutex>();D3D11_TEXTURE2D_DESC desc{};output->GetDesc(&desc);
      auto dxgi=device.as<IDXGIDevice>();winrt::com_ptr<IInspectable> inspectable;
      winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi.get(),inspectable.put()));
      auto uwp=inspectable.as<winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice>();
      InterlockedExchange(stage,5);
      using namespace winrt::Windows::Graphics;
      Capture::GraphicsCaptureItem item{nullptr};
      auto captureFactory=winrt::get_activation_factory<Capture::GraphicsCaptureItem,IGraphicsCaptureItemInterop>();
      winrt::check_hresult(captureFactory->CreateForWindow(target,winrt::guid_of<Capture::IGraphicsCaptureItem>(),winrt::put_abi(item)));
      InterlockedExchange(stage,6);
      auto size=item.Size();
      auto pool=Capture::Direct3D11CaptureFramePool::CreateFreeThreaded(uwp,static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(desc.Format),2,size);
      InterlockedExchange(stage,7);
      bool cursor=std::stoi(argv[6])!=0;
      auto capture=pool.CreateCaptureSession(item);capture.IsCursorCaptureEnabled(cursor);capture.StartCapture();
      InterlockedExchange(stage,8);
      while(IsWindow(target) && WaitForSingleObject(targetProcess.value,0)==WAIT_TIMEOUT) {
        const bool desiredCursor=(*cursorControl)!=0;
        if(desiredCursor!=cursor){capture.IsCursorCaptureEnabled(desiredCursor);cursor=desiredCursor;}
        DWORD owner=0;GetWindowThreadProcessId(target,&owner);if(owner!=targetPid)break;
        auto frame=pool.TryGetNextFrame();
        if(!frame){std::this_thread::sleep_for(std::chrono::milliseconds(5));continue;}
        auto content=frame.ContentSize();
        InterlockedExchange(stage,9);
        if(content.Width!=size.Width || content.Height!=size.Height) {
          frame.Close();if(content.Width>0 && content.Height>0){size=content;pool.Recreate(uwp,static_cast<winrt::Windows::Graphics::DirectX::DirectXPixelFormat>(desc.Format),2,size);}continue;
        }
        InterlockedExchange(stage,91);
        auto surface=frame.Surface();
        InterlockedExchange(stage,92);
        auto access=surface.as<winrt::IDirect3DDxgiInterfaceAccess>();
        InterlockedExchange(stage,93);
        winrt::com_ptr<ID3D11Texture2D> source;
        winrt::check_hresult(access->GetInterface(IID_PPV_ARGS(source.put())));
        InterlockedExchange(stage,10);
        D3D11_TEXTURE2D_DESC actual{};source->GetDesc(&actual);
        if(content.Width==static_cast<int>(desc.Width) && content.Height==static_cast<int>(desc.Height) &&
           actual.Width==desc.Width && actual.Height==desc.Height && actual.Format==desc.Format && !IsIconic(target) && IsWindowVisible(target)) {
          const auto status=mutex->AcquireSync(0,0);
          if(status==S_OK){context->CopyResource(output.get(),source.get());context->Flush();winrt::check_hresult(mutex->ReleaseSync(1));}
          else if(status!=WAIT_TIMEOUT){frame.Close();break;}
        }
        frame.Close();
      }
      capture.Close();pool.Close();return 0;
    } catch(winrt::hresult_error const &e) {return static_cast<int>(e.code().value);}
      catch(...) {return 6;}
  }
}
