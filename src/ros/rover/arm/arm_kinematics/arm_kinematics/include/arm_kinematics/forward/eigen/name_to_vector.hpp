//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_NAME_TO_VECTOR_HPP
#define ARM_KINEMATICS_NAME_TO_VECTOR_HPP

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "arm_kinematics/utilities/order.hpp"

namespace arm_kinematics {

template<typename TValue, typename TKey = std::string, typename TAlloc = std::allocator<TValue>>
class NameToVector {
public:
  /**
   * Add an item.
   * \param name The name the value can be looked up by
   * \param value The value to std::move to the collection
   * \returns The id of that added item
   */
  size_t add(const TKey & name, TValue value) {
    const auto idx = size();
    ids[name] = idx;
    names.emplace_back(name);
    data.emplace_back(std::move(value));
    return idx;
  }

  /**
   * Get the index of a name in the well-ordering_
   */
  size_t operator[](const TKey & name) {
    const auto it = ids.find(name);

    if (it == ids.end())
      throw std::out_of_range(name + " is not in the controller ordering_.");

    return it->second;
  }

  /**
   * Get the index of a name in the well-ordering_
   */
  size_t operator[](const TKey & name) const {
    const auto it = ids.find(name);

    if (it == ids.end())
      throw std::out_of_range(name + " is not in the controller ordering_.");

    return it->second;
  }

  /**
   * Get the index of a name in the well-ordering_
   */
  std::optional<size_t> get(const TKey & name) {
    const auto it = ids.find(name);

    if (it == ids.end())
      throw std::out_of_range(name + " is not in the collection.");

    return it->second;
  }

  /**
   * Get the index of a name in the well-ordering_
   */
  std::optional<size_t> get(const TKey & name) const {
    const auto it = ids.find(name);

    if (it == ids.end())
      throw std::out_of_range(name + " is not in the collection.");

    return it->second;
  }

  /**
   * Returns true if the map defines an id for the given name
   */
  [[nodiscard]] bool contains(const std::string & name) const noexcept {
    const auto it = ids.find(name);
    return it != ids.end();
  }

  /**
   * Get the name in the well-ordering_ at the given index
   */
  TValue & operator[](const size_t id) {
    return data[id];
  }

  TValue & by_name(const std::string & name) const {
    const auto it = ids.find(name);

    if (it == ids.end())
      throw std::out_of_range(name + " is not in the collection.");

    return data[it->second];
  }

  /**
   * Get the name in the well-ordering_ at the given index
   */
  const TValue & operator[](const size_t id) const {
    return data[id];
  }

  [[nodiscard]] size_t size() const noexcept {
    return data.size();
  }

  void reserve(const size_t N) noexcept {
    data.reserve(N);
    names.reserve(N);
  }

  void swap(const size_t a, const size_t b) {
    // Swap the actual data
    std::swap(data[a], data[b]);

    // Fix names lookup
    std::swap(ids[names[a]], ids[names[b]]);
    std::swap(names[a], names[b]);
  }

  void sort(const Order<> & order) noexcept {
    data = order.map(data);
    names = order.map(names);

    for (auto & [key, id] : ids) {
      id = order.inverse[id];
    }
  }

  std::vector<TValue, TAlloc> data;
  std::vector<TKey, std::allocator<TKey>> names;
  std::map<TKey, size_t> ids;
};

}


#endif //ARM_KINEMATICS_NAME_TO_VECTOR_HPP