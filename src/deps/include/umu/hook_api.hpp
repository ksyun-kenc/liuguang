#pragma once

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

#ifndef STATUS_PROCEDURE_NOT_FOUND
#define STATUS_PROCEDURE_NOT_FOUND ((NTSTATUS)0xC000007A)
#endif

namespace umu {
class HookApi {
 public:
  HookApi() {}

  ~HookApi() noexcept {}

  NTSTATUS Hook(void* proc, void* hook_proc) noexcept {
    NTSTATUS status = LhInstallHook(proc, hook_proc, (PVOID) nullptr, &hook_);
    if (!NT_SUCCESS(status)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": LhInstallHook(`%p') failed, 0x%08X\n", proc,
                status);
    }
    return status;
  }

  NTSTATUS Hook(HMODULE dll, LPCSTR proc_name, void* hook_proc) noexcept {
    void* proc = GetProcAddress(dll, proc_name);
    if (nullptr == proc) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": GetProcAddress(`%p!%s') failed, #%d!\n", dll,
                proc, GetLastError());
      return STATUS_PROCEDURE_NOT_FOUND;
    }
    return Hook(proc, hook_proc);
  }

  NTSTATUS Hook(LPCWSTR dll_name, LPCSTR proc, void* hook_proc) noexcept {
    HMODULE dll = GetModuleHandle(dll_name);
    if (nullptr == dll) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": GetModuleHandle(`%S!%s') failed, #%d!\n", dll,
                proc, GetLastError());
      return STATUS_DLL_NOT_FOUND;
    }
    return Hook(dll, proc, hook_proc);
  }

  NTSTATUS Unhook() noexcept {
    NTSTATUS status = LhUninstallHook(&hook_);
    if (NT_SUCCESS(status)) {
      LhWaitForPendingRemovals();
    } else {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": LhUninstallHook() failed, 0x%08X\n", status);
    }
    return status;
  }

  NTSTATUS SetExclusiveNone() noexcept {
    ULONG ACLEntries[1] = {0};
    NTSTATUS status = LhSetExclusiveACL(ACLEntries, 0, &hook_);
    if (!NT_SUCCESS(status)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": LhSetExclusiveACL() failed, 0x%08X\n", status);
    }
    return status;
  }

  NTSTATUS SetExclusive(ULONG* thread_id_list, ULONG thread_count) noexcept {
    NTSTATUS status = LhSetExclusiveACL(thread_id_list, thread_count, &hook_);
    if (!NT_SUCCESS(status)) {
      ATLTRACE2(atlTraceException, 0,
                __FUNCTION__ ": LhSetExclusiveACL() failed, 0x%08X\n", status);
    }
    return status;
  }

 private:
  HOOK_TRACE_INFO hook_{};
};

inline NTSTATUS HookAllThread(HookApi& hook, void* proc, void* hook_proc) {
  NTSTATUS status = hook.Hook(proc, hook_proc);
  if (NT_SUCCESS(status)) {
    status = hook.SetExclusiveNone();
  }
  return status;
}

inline NTSTATUS HookAllThread(HookApi& hook,
                              HMODULE dll,
                              LPCSTR proc,
                              void* hook_proc) {
  NTSTATUS status = hook.Hook(dll, proc, hook_proc);
  if (NT_SUCCESS(status)) {
    status = hook.SetExclusiveNone();
  }
  return status;
}
inline NTSTATUS HookAllThread(HookApi& hook,
                              LPCWSTR dll_name,
                              LPCSTR proc,
                              void* hook_proc) {
  NTSTATUS status = hook.Hook(dll_name, proc, hook_proc);
  if (NT_SUCCESS(status)) {
    status = hook.SetExclusiveNone();
  }
  return status;
}

template <typename T>
inline NTSTATUS HookProc(HMODULE dll,
                         LPCSTR proc_name,
                         T& proc,
                         HookApi& hook,
                         T hook_proc) {
  NTSTATUS status = STATUS_SUCCESS;
  proc = reinterpret_cast<T>(GetProcAddress(dll, proc_name));
  ATLTRACE2(atlTraceUtil, 0, __FUNCTION__ ": GetProcAddress(%p, %s) = %p\n",
            dll, proc_name, proc);
  ATLASSERT(nullptr != proc);
  status = HookAllThread(hook, proc, hook_proc);

  return status;
}
}  // end of namespace umu
