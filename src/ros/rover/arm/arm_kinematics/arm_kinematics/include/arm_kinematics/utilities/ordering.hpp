//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_ORDERING_HPP
#define ARM_KINEMATICS_ORDERING_HPP
#include <deque>
#include <vector>

#include "arm_kinematics/span.hpp"

class Order {
public:
  void push_back(const size_t idx) noexcept {
    map.emplace_back(idx);
  }

  void push_back_deletions(const arm_kinematics::span<bool> deletions) {
    for (size_t i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i);
  }

  void push_back_deletions(const size_t begin_idx, const arm_kinematics::span<bool> deletions) {
    for (size_t i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i + begin_idx);
  }

  std::deque<size_t> map{};
};

#endif //ARM_KINEMATICS_ORDERING_HPP