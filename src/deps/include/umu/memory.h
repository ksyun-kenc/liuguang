#pragma once

namespace umu {
namespace memory {
static inline void* GetVTableFunctionAddress(void* class_ptr, size_t offset) {
  void** vtable = *(void***)class_ptr;
  return vtable[offset];
}
}  // namespace memory
}  // end of namespace umu
