#pragma once

#include <functional>

// C++11

namespace umu {
class ScopeGuard {
 public:
  explicit ScopeGuard(std::function<void()> on_exit_scope)
      : on_exit_scope_(on_exit_scope), dismissed_(false) {}

  ~ScopeGuard() noexcept {
    if (!dismissed_) {
      on_exit_scope_();
    }
  }

  void Dismiss() { dismissed_ = true; }

 private:
  std::function<void()> on_exit_scope_;
  bool dismissed_;

  // noncopyable
  ScopeGuard(ScopeGuard const&) = delete;
  ScopeGuard& operator=(ScopeGuard const&) = delete;
};
}  // end of namespace umu

#define SCOPEGUARD_LINENAME_CAT(name, line) name##line
#define SCOPEGUARD_LINENAME(name, line) SCOPEGUARD_LINENAME_CAT(name, line)
#define ON_SCOPE_EXIT(callback) \
  umu::ScopeGuard SCOPEGUARD_LINENAME(EXIT, __LINE__)(callback)
