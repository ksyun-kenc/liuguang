#pragma once

namespace umu {
class Module {
 public:
  template <typename T>
  static T GetProcAddress(HMODULE module, LPCSTR proc_name) noexcept {
    ATLASSERT(nullptr != module);
    return reinterpret_cast<T>(::GetProcAddress(module, proc_name));
  }

 public:
  Module() {}

  ~Module() { Free(); }

  DWORD Get(LPCTSTR name) noexcept {
    ATLASSERT(nullptr == module_);
    module_ = GetModuleHandle(name);
    if (nullptr == module_) {
      return GetLastError();
    }
    ATLTRACE2(atlTraceUtil, 0, "GetModuleHandle(%s) = %p\n", (LPCSTR)CT2A(name),
              module_);
    return NO_ERROR;
  }

  DWORD GetOrLoad(LPCTSTR name) noexcept {
    ATLASSERT(nullptr == module_);
    module_ = GetModuleHandle(name);
    if (nullptr == module_) {
      module_ = LoadLibrary(name);
      if (nullptr == module_) {
        return GetLastError();
      }
      load_ = true;
      ATLTRACE2(atlTraceUtil, 0, "LoadLibrary(%s) = %p\n", (LPCSTR)CT2A(name),
                module_);
    } else {
      ATLTRACE2(atlTraceUtil, 0, "GetModuleHandle(%s) = %p\n",
                (LPCSTR)CT2A(name), module_);
    }
    return NO_ERROR;
  }

  DWORD Load(LPCTSTR name) noexcept {
    ATLASSERT(nullptr == module_);
    module_ = LoadLibrary(name);
    if (nullptr == module_) {
      return GetLastError();
    }
    load_ = true;
    ATLTRACE2(atlTraceUtil, 0, "LoadLibrary(%s) = %p\n", (LPCSTR)CT2A(name),
              module_);
    return NO_ERROR;
  }

  void Free() noexcept {
    if (nullptr != module_) {
      if (load_) {
        FreeLibrary(module_);
        load_ = false;
        ATLTRACE2(atlTraceUtil, 0, "FreeLibrary(%p)\n", module_);
      }
      module_ = nullptr;
    }
  }

  HMODULE GetHandle() const noexcept {
    ATLASSERT(nullptr != module_);
    return module_;
  }

  operator HMODULE() const noexcept {
    ATLASSERT(nullptr != module_);
    return module_;
  }

  operator HANDLE() const noexcept {
    ATLASSERT(nullptr != module_);
    return module_;
  }

  bool IsLoad() const noexcept {
    ATLASSERT(nullptr != module_);
    return load_;
  }

  template <typename T>
  T GetProcAddress(LPCSTR proc_name) const noexcept {
    ATLASSERT(nullptr != module_);
    return reinterpret_cast<T>(::GetProcAddress(module_, proc_name));
  }

 private:
  bool load_ = false;
  HMODULE module_ = nullptr;
};
}  // namespace umu