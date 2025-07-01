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
  using value_type = std::shared_ptr<InputT>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  template<bool is_const>
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::shared_ptr<InputT>;
    using difference_type = std::ptrdiff_t;

    // Conditional types based on const-ness
    using reference = typename std::conditional_t<is_const,
      const value_type&,
      value_type&>;
    using pointer = typename std::conditional_t<is_const,
      const value_type*,
      value_type*>;
    using base_iterator = typename std::conditional_t<is_const,
      typename std::map<std::string, std::weak_ptr<InputT>>::const_iterator,
      typename std::map<std::string, std::weak_ptr<InputT>>::iterator>;

    using map_type = std::map<std::string, std::weak_ptr<InputT>>;
    using map_pointer = typename std::conditional_t<is_const,
      const map_type*,
      map_type*>;

    explicit Iterator(base_iterator it, map_pointer map)
      : it_(it), map_(map), current_value_()
    {
      if (it_ != map_->end()) {
        skip_expired();
      }
    }


    // Convert non-const iterator to const iterator
    template<bool was_const, typename = std::enable_if_t<is_const && !was_const>>
    explicit Iterator(const Iterator<was_const>& other)
      : it_(other.it_), map_(other.map_), current_value_(other.current_value_) {}

    reference operator*() {
      ensure_current_value();
      return current_value_;
    }

    pointer operator->() {
      ensure_current_value();
      return &current_value_;
    }

    // Prefix increment
    Iterator& operator++() {
      ++it_;
      skip_expired();
      return *this;
    }

    // Postfix increment
    Iterator operator++(int) {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const Iterator& a, const Iterator& b) {
      return a.it_ == b.it_;
    }

    friend bool operator!=(const Iterator& a, const Iterator& b) {
      return !(a == b);
    }

    /// Allows access to the base iterator for conversion
    base_iterator base() const { return it_; }

  private:
    base_iterator it_;
    map_pointer map_;
    value_type current_value_;

    /// Skips over any weak pointers that have expired
    void skip_expired() {
      while (it_ != map_->end()) {
        if (auto ptr = it_->second.lock()) {
          current_value_ = ptr;
          return;
        }

        // Remove any expired entry before continuing to the next item
        if constexpr (!is_const) {
          it_ = map_->erase(it_);
        } else {
          ++it_;
        }
      }

      current_value_.reset();
    }

    void ensure_current_value() {
      if (current_value_)
        return;

      if (auto ptr = it_->second.lock()) {
        current_value_ = ptr;
        return;
      }

      // If current value expired, skip to next valid one
      if constexpr (!is_const) {
        it_ = map_->erase(it_);
      } else {
        ++it_;
      }
      skip_expired();
    }

    // Grant access to other iterator specializations
    template<bool> friend class Iterator;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

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
