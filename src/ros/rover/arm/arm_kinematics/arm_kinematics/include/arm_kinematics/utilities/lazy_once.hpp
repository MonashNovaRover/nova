//
// Created by Bailey Chessum on 9/4/26.
//

#ifndef ARM_KINEMATICS_LAZY_ONCE_HPP
#define ARM_KINEMATICS_LAZY_ONCE_HPP

#include <memory>
#include <mutex>
#include <utility>

namespace arm_kinematics {

/**
 * Thread-safe lazy-initialized cell.
 *
 * Stores a `std::unique_ptr<T>` that is constructed exactly once on the first call to `get()`,
 * via a caller-supplied factory closure. Subsequent calls return the cached instance directly.
 * `std::call_once` provides the cross-thread synchronization.
 *
 * Designed for the "RobotModel-style" lazy-init pattern where multiple cached fields each have
 * their own construction logic. Each call site supplies its own factory inline; the cell does
 * not store the factory, so there's no per-cell capture overhead.
 *
 * **Lifetime:** because the underlying storage is `std::unique_ptr<T>`, `T` may be a
 * forward-declared type at the point where `LazyOnce<T>` is *used as a member*, as long as `T`
 * is complete at the point where the enclosing class's destructor is instantiated. In practice
 * this means the enclosing class needs an out-of-line destructor in a translation unit that
 * has the full `T` definition in scope.
 *
 * **Thread safety:** `get()` is safe to call concurrently from multiple threads. `is_initialized()`
 * is racy with concurrent first-time `get()` calls; treat it as a debug/diagnostic accessor only.
 *
 * **Move-only.** Copying a lazy cell would require either copying `T` (often unwanted or
 * impossible) or sharing storage (semantically confusing). Move support exists for cases where
 * the enclosing class needs to be movable.
 */
template <class T>
class LazyOnce {
public:
  LazyOnce() = default;
  LazyOnce(const LazyOnce &) = delete;
  LazyOnce & operator=(const LazyOnce &) = delete;
  LazyOnce(LazyOnce &&) noexcept = default;
  LazyOnce & operator=(LazyOnce &&) noexcept = default;

  /**
   * Returns the cached value, constructing it via `factory()` on the first call.
   *
   * `factory` must be invocable with no arguments and return `std::unique_ptr<T>`. It is
   * invoked exactly once across all threads, even if multiple threads call `get()` concurrently
   * for the first time. Subsequent calls do not invoke the factory and return the cached value
   * directly.
   *
   * \pre `factory()` must return a non-null `std::unique_ptr<T>`. A null return is a contract
   *      violation; the resulting dereference is undefined.
   */
  template <class Factory>
  T & get(Factory && factory) const
  {
    std::call_once(flag_, [this, &factory]() {
      value_ = std::forward<Factory>(factory)();
    });
    return *value_;
  }

  /**
   * Diagnostic: true iff the cell has been initialized via a successful `get()` call.
   *
   * \warning This is a racy read with respect to concurrent first-time `get()` calls. Use only
   * for debug logging or single-threaded contexts.
   */
  [[nodiscard]] bool is_initialized() const noexcept { return value_ != nullptr; }

private:
  mutable std::unique_ptr<T> value_{};
  mutable std::once_flag flag_{};
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_LAZY_ONCE_HPP
