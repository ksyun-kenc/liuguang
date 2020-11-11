#pragma once

#include <psapi.h>


namespace umu {
namespace apppath_t {
inline CString GetProgramPath(HMODULE module_handle = nullptr) {
  CString program_path;

  for (DWORD buffer_size = MAX_PATH;; buffer_size += MAX_PATH) {
    DWORD size = ::GetModuleFileName(
        module_handle, program_path.GetBuffer(buffer_size), buffer_size);
    if (0 == size) {
      return _T("");
    }

    if (size < buffer_size) {
      program_path.ReleaseBuffer(size);
      return program_path;
    }

    if (ERROR_INSUFFICIENT_BUFFER != ::GetLastError()) {
      return _T("");
    }
  }
}

// 返回的路径最后有带 '\'
inline CString GetProgramDirectory(HMODULE module_handle = nullptr) {
  CString program_path(GetProgramPath(module_handle));
  program_path.Truncate(program_path.ReverseFind(_T('\\')) + 1);
  return program_path;
}

inline CString GetProgramBaseName(HMODULE module_handle = nullptr) {
  CString program_path(GetProgramPath(module_handle));
  return program_path.Mid(program_path.ReverseFind(_T('\\')) + 1);
}

inline CString GetProductDirectory(HMODULE module_handle = nullptr) {
  CString path(GetProgramDirectory(module_handle));
  // bin 不区分大小写
  LPTSTR found(StrRStrI(path, nullptr, _T("\\bin\\")));
  if (nullptr != found) {
    path.Truncate(static_cast<int>(found - path) + 1);
  } else {
    int backslash_count(0);
    for (int pos = path.GetLength() - 1; pos > 0; --pos) {
      if (_T('\\') == path.GetAt(pos)) {
        path.Truncate(pos + 1);
        if (2 == ++backslash_count) {
          // 倒数第二个 \\ 截断
          break;
        }
      }
    }
  }
  return path;
}

// To retrieve the base name of a module in the current process, use the
// GetModuleFileName function to retrieve the full module name and then use a
// function call such as strrchr(szmodulename, '\') to scan to the beginning of
// the base name within the module name string. This is more efficient and more
// reliable than calling GetModuleBaseName with a handle to the current process.
inline CString GetModuleBaseName(HANDLE process, HMODULE module = nullptr) {
  CString name;

  for (DWORD buffer_size = 64;; buffer_size += 64) {
    DWORD size = ::GetModuleBaseName(process, module,
                                     name.GetBuffer(buffer_size), buffer_size);
    if (0 == size) {
      return _T("");
    }

    if (size < buffer_size) {
      name.ReleaseBuffer(size);
      return name;
    }

    if (ERROR_INSUFFICIENT_BUFFER != ::GetLastError()) {
      return _T("");
    }
  }
}
}  // end of namespace apppath
}  // end of namespace umu
