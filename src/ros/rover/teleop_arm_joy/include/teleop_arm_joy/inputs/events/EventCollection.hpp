//
// Created by nova on 6/29/25.
//

#ifndef TELEOP_ARM_JOY_EVENTCOLLECTION_H
#define TELEOP_ARM_JOY_EVENTCOLLECTION_H

#include <utility>
#include <vector>
#include <memory>
#include <map>
#include "Event.hpp"

namespace teleop_arm_joy {

/**
 * A container of Events, where events that don't yet exist are created when an attempt is made to retrieve them.
 */
class EventCollection {
public:
  EventCollection() = default;
  explicit EventCollection(std::weak_ptr<EventListenerQueue> listener_queue)
    : listener_queue_(std::move(listener_queue)) {}

  template<bool is_const>
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = Event::SharedPtr;
    using difference_type = std::ptrdiff_t;

    using reference = typename std::conditional_t<is_const,
      const value_type&,
      value_type&>;
    using pointer = typename std::conditional_t<is_const,
      const value_type*,
      value_type*>;
    using base_iterator = typename std::conditional_t<is_const,
      typename std::map<std::string, std::weak_ptr<Event>>::const_iterator,
      typename std::map<std::string, std::weak_ptr<Event>>::iterator>;

    using map_type = std::map<std::string, std::weak_ptr<Event>>;
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

    reference operator*() {
      ensure_current_value();
      return current_value_;
    }

    pointer operator->() {
      ensure_current_value();
      return &current_value_;
    }

    Iterator& operator++() {
      ++it_;
      skip_expired();
      return *this;
    }

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

  private:
    base_iterator it_;
    map_pointer map_;
    value_type current_value_;

    void skip_expired() {
      while (it_ != map_->end()) {
        if (auto ptr = it_->second.lock()) {
          current_value_ = ptr;
          return;
        }

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

      if constexpr (!is_const) {
        it_ = map_->erase(it_);
      } else {
        ++it_;
      }
      skip_expired();
    }
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  Event::SharedPtr operator[](const std::string& index) {
    auto it = items_.find(index);
    Event::SharedPtr ptr;

    if (it != items_.end()) {
      ptr = it->second.lock();
      if (!ptr) {
        items_.erase(it);
      }
    }

    if (!ptr) {
      ptr = std::make_shared<Event>(index, listener_queue_);
      items_[index] = ptr;
    }

    return ptr;
  }

  iterator begin() { return iterator(items_.begin(), &items_); }
  iterator end() { return iterator(items_.end(), &items_); }
  const_iterator begin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator end() const { return const_iterator(items_.end(), &items_); }
  const_iterator cbegin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator cend() const { return const_iterator(items_.end(), &items_); }

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
  std::map<std::string, std::weak_ptr<Event>> items_{};
  std::weak_ptr<EventListenerQueue> listener_queue_;
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_EVENTCOLLECTION_H
