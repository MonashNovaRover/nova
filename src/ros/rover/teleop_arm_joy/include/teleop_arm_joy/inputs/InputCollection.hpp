//
// Created by nova on 7/1/25.
//

#ifndef TELEOP_ARM_JOY_INPUTCOLLECTION_HPP
#define TELEOP_ARM_JOY_INPUTCOLLECTION_HPP

#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"
#include "InputCommon.hpp"
#include <functional>

namespace teleop_arm_joy {

template<typename InputT>
class InputCollection {
public:
  explicit InputCollection(EventCollection& events) : events_(events) {}

  // Add move constructor
  InputCollection(InputCollection&& other) noexcept
    : items_(std::move(other.items_))
    , events_(other.events_) {}

  // Add move assignment
  InputCollection& operator=(InputCollection&& other) noexcept {
    if (this != &other) {
      items_ = std::move(other.items_);
      events_ = other.events_;
    }
    return *this;
  }

  // Delete copy constructor and assignment
  InputCollection(const InputCollection&) = delete;
  InputCollection& operator=(const InputCollection&) = delete;

  // Container type aliases
  using iterator = WeakMapIterator<InputT, false>;
  using const_iterator = WeakMapIterator<InputT, true>;

  std::shared_ptr<InputT> operator[](const std::string& key) {
    // Find the element
    auto it = items_.find(key);
    std::shared_ptr<InputT> ptr;

    if (it != items_.end()) {
      ptr = it->second.lock();
      if (!ptr) {
        // If the weak_ptr expired, remove it from the map
        items_.erase(it);
      }
    }

    if (!ptr) {
      // Create new input if we don't have a valid one
      ptr = std::make_shared<InputT>(key);
      setup_new_item(ptr);
      items_[key] = ptr;
    }

    return ptr;
  }

  iterator begin() { return iterator(items_.begin(), &items_); }
  iterator end() { return iterator(items_.end(), &items_); }
  const_iterator begin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator end() const { return const_iterator(items_.end(), &items_); }
  const_iterator cbegin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator cend() const { return const_iterator(items_.end(), &items_); }

  /**
   * Utility method to delete expired weak pointers from the internal map
   */
  void clean_up() {
    for (auto it = items_.begin(); it != items_.end();) {
      if (it->second.expired()) {
        it = items_.erase(it);
      } else {
        ++it;
      }
    }
  }

  /**
   * Gets the number of non-expired inputs
   */
  [[nodiscard]] size_t size() const {
    size_t count = 0;
    for (const auto& item : items_) {
      if (!item.second.expired()) {
        ++count;
      }
    }
    return count;
  }

private:
  void setup_new_item(const std::shared_ptr<InputT>& item);
  std::map<std::string, std::weak_ptr<InputT>> items_{};
//  std::reference_wrapper<EventCollection> events_;
  EventCollection& events_;
};

template<>
void InputCollection<Button>::setup_new_item(const std::shared_ptr<Button> &item);

template<>
void InputCollection<Axis>::setup_new_item(const std::shared_ptr<Axis> &item);

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_INPUTCOLLECTION_HPP
