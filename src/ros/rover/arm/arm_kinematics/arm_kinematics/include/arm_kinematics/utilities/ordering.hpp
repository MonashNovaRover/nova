//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_ORDERING_HPP
#define ARM_KINEMATICS_ORDERING_HPP

#include <deque>
#include "arm_kinematics/span.hpp"

namespace arm_kinematics {

/**
 * std::vector based structure for looking up indices
 */
class IndexMap {
public:
  explicit IndexMap(const size_t N) : map(N) {}

  std::vector<size_t> map{};
};

/**
 * std::deque based structure for looking up indices
 */
class Order {
public:
  void push_back(const size_t idx) noexcept {
    map.emplace_back(idx);
  }

  /**
   * Adds indices that are false from a deletions vector, in order, to this order
   * \param deletions any element that is true will have the index added to this order
   */
  void push_back_deletions(const std::vector<bool> & deletions) {
    for (size_t i = 0; i < deletions.size(); ++i)
      if (!deletions[i])
        push_back(i);
  }

  /**
   * Adds indices that are false from a deletions vector, in order, to this order
   * \param deletions any element that is true will have the index added to this order
   * \param begin_idx inclusive index to begin at
   * \param end_idx exclusive index to end at
   */
  void push_back_deletions(const std::vector<bool> & deletions, const size_t begin_idx, const size_t end_idx) {
    assert(deletions.size() <= end_idx || begin_idx >= end_idx);
    for (size_t i = begin_idx; i < end_idx; ++i)
      if (!deletions[i])
        push_back(i);
  }

  // TODO construct inverse
  /**
   * Constructs an IndexMap that converts from old indices to new indicies.
   * \param capacity The size of the new IndexMap
   */
  [[nodiscard]] IndexMap inverse(const size_t capacity) const {
    // Maps old to new
    IndexMap inverse_map(capacity);

    size_t new_idx = 0;
    for (const auto & old_idx : map) {
      inverse_map.map[old_idx] = new_idx;
      new_idx++;
    }

    return inverse_map;
  }

  /**
   * Constructs an IndexMap that converts from old indices to new indicies.
   * Assumes the largest index in this Order's map to be the capacity of the IndexMap
   */
  [[nodiscard]] IndexMap inverse() const {
    size_t max_value = map.front();

    for (const auto & value : map)
      if (value > max_value)
        max_value = value;

    return inverse(max_value);
  }

  std::deque<size_t> map{};
};


}

#endif //ARM_KINEMATICS_ORDERING_HPP