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
    map.emplace_back(idx);
  }

  // Enabled only if not FixedSize
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  void push_back_deletions(const span<bool> deletions) {
    for (size_type i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i);
  }

  // Enabled only if not FixedSize
  template<bool B = FixedSize, std::enable_if_t<!B, int> = 0>
  void push_back_deletions(const size_type begin_idx, const span<bool> deletions) {
    for (size_type i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i + begin_idx);
  }

  // Indexing into the order
  reference operator[](size_type idx) noexcept {
    return map[idx];
  }

  const_reference operator[](size_type idx) const noexcept {
    return map[idx];
  }

  // Iteration over the indices
  iterator begin() noexcept { return map.begin(); }
  const_iterator begin() const noexcept { return map.begin(); }
  const_iterator cbegin() const noexcept { return map.cbegin(); }

  iterator end() noexcept { return map.end(); }
  const_iterator end() const noexcept { return map.end(); }
  const_iterator cend() const noexcept { return map.cend(); }

  size_type size() const noexcept { return map.size(); }

  Container map{};
};

} // namespace arm_kinematics

#endif //ARM_KINEMATICS_ORDERING_HPP