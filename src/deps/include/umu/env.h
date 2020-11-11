#pragma once

#include <string>

namespace umu {
namespace env {
inline bool GetEnvironmentVariableA(_In_opt_ const std::string& name,
                                    _Out_opt_ std::string* value) {
  DWORD length = ::GetEnvironmentVariableA(name.data(), nullptr, 0);
  // if length == 1, it's ""
  if (length > 0) {
    // length including terminating null character
    if (nullptr != value) {
      value->resize(length - 1);
      length = ::GetEnvironmentVariableA(name.data(), value->data(), length);
      if (length == 0) {
        return false;
      }
    }
    return true;
  }
  // GetLastError returns ERROR_ENVVAR_NOT_FOUND
  return false;
}
inline bool GetEnvironmentVariableW(_In_opt_ const std::wstring& name,
                                    _Out_opt_ std::wstring* value) {
  DWORD length = ::GetEnvironmentVariableW(name.data(), nullptr, 0);
  // if length == 1, it's ""
  if (length > 0) {
    // length including terminating null character
    if (nullptr != value) {
      value->resize(length - 1);
      length = ::GetEnvironmentVariableW(name.data(), value->data(), length);
      if (length == 0) {
        return false;
      }
    }
    return true;
  }
  // GetLastError returns ERROR_ENVVAR_NOT_FOUND
  return false;
}

inline std::string ExpandEnvironmentStringA(const std::string& original) {
  if (std::string::npos != original.find('%')) {
    // max = 32 * 1024
    const DWORD c_size = 32 * 1024;
    std::string expanded;
    expanded.resize(c_size);
    int length = (int)ExpandEnvironmentStringsA(original.data(),
                                                expanded.data(), c_size);
    if (0 != length) {
      expanded.resize(length - 1);
      return expanded;
    }
  }

  return original;
}
inline std::wstring ExpandEnvironmentStringW(const std::wstring& original) {
  if (std::wstring::npos != original.find(L'%')) {
    // max = 32 * 1024
    const DWORD c_size = 32 * 1024;
    std::wstring expanded;
    expanded.resize(c_size);
    int length = (int)ExpandEnvironmentStringsW(original.data(),
                                                expanded.data(), c_size);
    if (0 != length) {
      expanded.resize(length - 1);
      return expanded;
    }
  }

  return original;
}

// no \ tail, unless it's root
inline std::string GetCurrentDirectoryA() {
  DWORD length = ::GetCurrentDirectoryA(0, nullptr);
  // if length == 1, it's ""
  if (length > 0) {
    std::string dir;
    dir.resize(length - 1);
    // length including terminating null character
    length = ::GetCurrentDirectoryA(length, dir.data());
    if (length > 0) {
      return dir;
    }
  }
  return "";
}
inline std::wstring GetCurrentDirectoryW() {
  DWORD length = ::GetCurrentDirectoryW(0, nullptr);
  // if length == 1, it's ""
  if (length > 0) {
    std::wstring dir;
    dir.resize(length - 1);
    // length including terminating null character
    length = ::GetCurrentDirectoryW(length, dir.data());
    if (length > 0) {
      return dir;
    }
  }
  return L"";
}

inline std::string GetTempDirectoryA() {
  std::string temp_dir;

  // The maximum possible return value is MAX_PATH+1 (261)
  for (DWORD buffer_size = MAX_PATH + 1;;) {
    temp_dir.resize(buffer_size);
    DWORD size = ::GetTempPathA(buffer_size, temp_dir.data());
    if (0 == size) {
      ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": #%d\n", ::GetLastError());
      return "";
    }
    if (size < buffer_size) {
      temp_dir.resize(size);
      return temp_dir;
    }
    ATLTRACE2(atlTraceUtil, 0, "%d -> %d\n", buffer_size, size);
    buffer_size = size;
  }
}
inline std::wstring GetTempDirectoryW() {
  std::wstring temp_dir;

  // The maximum possible return value is MAX_PATH+1 (261)
  for (DWORD buffer_size = MAX_PATH + 1;;) {
    temp_dir.resize(buffer_size);
    DWORD size = ::GetTempPathW(buffer_size, temp_dir.data());
    if (0 == size) {
      ATLTRACE2(atlTraceException, 0, __FUNCTION__ ": #%d\n", ::GetLastError());
      return L"";
    }
    if (size < buffer_size) {
      temp_dir.resize(size);
      return temp_dir;
    }
    ATLTRACE2(atlTraceUtil, 0, "%d -> %d\n", buffer_size, size);
    buffer_size = size;
  }
}
}  // end of namespace env
}  // end of namespace umu