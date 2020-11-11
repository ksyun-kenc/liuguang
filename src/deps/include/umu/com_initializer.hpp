#pragma once

namespace umu {
class ComInitializer {
 public:
  ComInitializer() : hr_(CO_E_NOTINITIALIZED) {}
  ~ComInitializer() {
    if (SUCCEEDED(hr_)) {
      ::CoUninitialize();
    }
  }

  HRESULT Initialize(DWORD type = COINIT_APARTMENTTHREADED) {
    // https://docs.microsoft.com/en-us/windows/win32/learnwin32/initializing-the-com-library
    // it is a good idea to set the COINIT_DISABLE_OLE1DDE flag in the dwCoInit
    // parameter. Setting this flag avoids some overhead associated with Object
    // Linking and Embedding (OLE) 1.0, an obsolete technology.
    hr_ = CoInitializeEx(nullptr, type | COINIT_DISABLE_OLE1DDE);
    return hr_;
  }

 private:
  HRESULT hr_;
};
}  // end of namespace umu
