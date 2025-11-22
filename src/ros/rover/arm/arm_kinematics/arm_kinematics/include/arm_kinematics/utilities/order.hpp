//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_ORDERING_HPP
#define ARM_KINEMATICS_ORDERING_HPP

#include <deque>
#include <vector>

#include "arm_kinematics/span.hpp"

namespace arm_kinematics {

template<bool FixedSize = true, typename T = std::size_t>
class Order {
public:
  using value_type      = T;
  using Container       = std::conditional_t<
    FixedSize,
    std::vector<T>,
    std::deque<T>
  >;
  using size_type       = typename Container::size_type;
  using reference       = typename Container::reference;
  using const_reference = typename Container::const_reference;
  using iterator        = typename Container::iterator;
  using const_iterator  = typename Container::const_iterator;
  using reverse_iterator       = typename Container::reverse_iterator;
  using const_reverse_iterator = typename Container::const_reverse_iterator;

  // Enabled only if FixedSize
  template<bool B = FixedSize, std::enable_if_t<B, int> = 0>
  explicit Order(size_type n)
    : map(n) {}

  // Provide a default ctor only for the non-fixed case
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  Order() : map() {}

  // Enabled only if not FixedSize
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  void push_back(size_type idx) noexcept {
    reset_inverse();
    map.emplace_back(idx);
  }

  // Enabled only if not FixedSize
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  void push_back_deletions(const span<bool> deletions) {
    reset_inverse();
    for (size_type i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i);
  }

  // Enabled only if not FixedSize
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  void push_back_deletions(const size_type begin_idx, const span<bool> deletions) {
    reset_inverse();
    for (size_type i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i + begin_idx);
  }

  // Indexing into the order
  reference operator[](size_type idx) noexcept {
    reset_inverse();
    return map[idx];
  }

  const_reference operator[](size_type idx) const noexcept {
    return map[idx];
  }

  [[nodiscard]] constexpr const Order<true, T> & inverse(const size_type capacity) const
  {
    if (inverse_)
      return *inverse_;

    inverse_ = std::make_unique<Order<true, T>>(capacity);
    for (size_type i = 0; i < map.size(); ++i)
      (*inverse_)[map[i]] = i;

    return *inverse_;
  }

  [[nodiscard]] constexpr const Order<true, T> & inverse() const
  {
    if (inverse_)
      return *inverse_;

    T max = 0;
    for (const auto & idx : map)
      if (idx > max)
        max = idx;

    return inverse(max);
  }

  // Iteration over the indices
  iterator begin() noexcept
  {
    reset_inverse();
    return map.begin();
  }
  const_iterator begin() const noexcept { return map.begin(); }
  const_iterator cbegin() const noexcept { return map.cbegin(); }

  iterator end() noexcept
  {
    reset_inverse();
    return map.end();
  }
  const_iterator end() const noexcept { return map.end(); }
  const_iterator cend() const noexcept { return map.cend(); }

  reverse_iterator rbegin() noexcept
  {
    reset_inverse();
    return map.rbegin();
  }
  const_reverse_iterator rbegin() const noexcept { return map.rbegin(); }
  const_reverse_iterator crbegin() const noexcept { return map.crbegin(); }

  reverse_iterator rend() noexcept { return map.rend(); }
  const_reverse_iterator rend() const noexcept { return map.rend(); }
  const_reverse_iterator crend() const noexcept { return map.crend(); }

  size_type size() const noexcept { return map.size(); }

  Container map{};

private:
  void reset_inverse() noexcept
  {
    inverse_ = nullptr;
  }

  // Lazily evaluated reverse order
  mutable std::unique_ptr<Order<true, T>> inverse_;
};

} // namespace arm_kinematics

#endif //ARM_KINEMATICS_ORDERING_HPP