#pragma once

#include <ShellAPI.h>

namespace umu {
namespace shellapi {
inline DWORD Execute(LPCTSTR file,
                     LPCTSTR parameters = nullptr,
                     LPCTSTR working_directory = nullptr,
                     LPCTSTR verb = nullptr,
                     ULONG mask = 0UL,
                     int show = SW_NORMAL,
                     HANDLE* process_handle = nullptr) {
  SHELLEXECUTEINFO sei = {sizeof(sei)};

  sei.fMask =
      (nullptr != process_handle ? mask | SEE_MASK_NOCLOSEPROCESS : mask);
  sei.lpVerb = verb;
  sei.lpFile = file;
  sei.lpParameters = parameters;
  sei.lpDirectory = working_directory;
  sei.nShow = show;
  if (::ShellExecuteEx(&sei)) {
    if (nullptr != process_handle) {
      *process_handle = sei.hProcess;
    }
    return ERROR_SUCCESS;
  }
  return ::GetLastError();
}

inline DWORD ExecuteWait(LPCTSTR file,
                         LPCTSTR parameters = nullptr,
                         LPCTSTR working_directory = nullptr,
                         LPCTSTR verb = nullptr,
                         ULONG mask = 0UL,
                         int show = SW_NORMAL,
                         LPDWORD exit_code = nullptr) {
  SHELLEXECUTEINFO sei = {sizeof(sei)};

  sei.fMask = mask | SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = verb;
  sei.lpFile = file;
  sei.lpParameters = parameters;
  sei.lpDirectory = working_directory;
  sei.nShow = show;
  if (::ShellExecuteEx(&sei)) {
    ::WaitForSingleObject(sei.hProcess, INFINITE);
    if (nullptr != exit_code) {
      ::GetExitCodeProcess(sei.hProcess, exit_code);
    }
    ::CloseHandle(sei.hProcess);
    return ERROR_SUCCESS;
  }
  return ::GetLastError();
}
}  // end of namespace shellapi
}  // end of namespace umu
