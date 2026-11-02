#pragma once
#include <winrt/base.h>
#include <Unknwn.h>

namespace winrt {
  struct
#if WINRT_IMPL_HAS_DECLSPEC_UUID
    __declspec(uuid("A9B3D012-3DF2-4EE3-B8D1-8695F457D3C1"))
#endif
    IDirect3DDxgiInterfaceAccess : ::IUnknown {
    virtual HRESULT __stdcall GetInterface(REFIID id, void **object) = 0;
  };
}
#if !WINRT_IMPL_HAS_DECLSPEC_UUID
template<>
constexpr auto __mingw_uuidof<winrt::IDirect3DDxgiInterfaceAccess>() -> GUID const & {
  static constexpr GUID id{0xA9B3D012,0x3DF2,0x4EE3,{0xB8,0xD1,0x86,0x95,0xF4,0x57,0xD3,0xC1}};
  return id;
}
#endif
