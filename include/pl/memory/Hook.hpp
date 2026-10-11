#pragma once

/**
 * @file Hook.hpp
 * @brief Memory hook API.
 */

#include <string_view>
#include <utility>

#include "pl/Export.hpp"

namespace pl::memory {

using FuncPtr = void *;

/**
 * @brief Hook chain priority. Lower values run earlier.
 */
enum class HookPriority : int {
  Highest = 0,
  High = 100,
  Normal = 200,
  Low = 300,
  Lowest = 400,
};

/**
 * @brief Installs a detour for a target function.
 *
 * @param name Diagnostic label used in log lines (e.g. "PauseMenuOpen"). May be empty.
 * @return 0 on success, -1 on a failed/refused install.
 *
 * Three overloads are exported rather than one function with default arguments. A default
 * argument is resolved at the call site and does not emit a distinct symbol, so a library built
 * against the old 4-argument signature (e.g. libinbuiltmods.so) imports
 * `_ZN2pl6memory4hookEPvS1_PS1_NS0_12HookPriorityE`, which a 5-argument definition never
 * exports; the loader then fails with UnsatisfiedLinkError and every inbuilt mod silently dies.
 * The 4- and 3-argument overloads exist solely to keep those old binaries loadable. All three
 * forward to the one implementation below.
 */
PL_EXPORT int hook(FuncPtr target, FuncPtr detour, FuncPtr *originalFunc,
                   HookPriority priority, std::string_view name);

/** @copydoc hook(FuncPtr, FuncPtr, FuncPtr *, HookPriority, std::string_view) */
PL_EXPORT int hook(FuncPtr target, FuncPtr detour, FuncPtr *originalFunc,
                   HookPriority priority);

/** @copydoc hook(FuncPtr, FuncPtr, FuncPtr *, HookPriority, std::string_view) */
PL_EXPORT int hook(FuncPtr target, FuncPtr detour, FuncPtr *originalFunc);

/**
 * @brief Removes a detour from a target function.
 */
PL_EXPORT bool unhook(FuncPtr target, FuncPtr detour);

/**
 * @brief RAII owner for an installed hook.
 */
class HookHandle {
public:
  HookHandle() = default;

  HookHandle(FuncPtr target, FuncPtr detour, FuncPtr *originalFunc,
             HookPriority priority = HookPriority::Normal)
      : mTarget(target), mDetour(detour),
        mInstalled(hook(target, detour, originalFunc, priority) == 0) {}

  HookHandle(const HookHandle &) = delete;
  HookHandle &operator=(const HookHandle &) = delete;

  HookHandle(HookHandle &&other) noexcept { swap(other); }

  HookHandle &operator=(HookHandle &&other) noexcept {
    if (this != &other) {
      reset();
      swap(other);
    }
    return *this;
  }

  ~HookHandle() { reset(); }

  [[nodiscard]] bool installed() const noexcept { return mInstalled; }

  void reset() {
    if (mInstalled) {
      unhook(mTarget, mDetour);
      mInstalled = false;
    }
    mTarget = nullptr;
    mDetour = nullptr;
  }

  void swap(HookHandle &other) noexcept {
    std::swap(mTarget, other.mTarget);
    std::swap(mDetour, other.mDetour);
    std::swap(mInstalled, other.mInstalled);
  }

private:
  FuncPtr mTarget{};
  FuncPtr mDetour{};
  bool mInstalled{};
};

} // namespace pl::memory

