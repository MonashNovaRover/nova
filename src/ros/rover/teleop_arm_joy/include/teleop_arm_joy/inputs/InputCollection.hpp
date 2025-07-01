//
// Created by nova on 7/1/25.
//

#ifndef TELEOP_ARM_JOY_INPUTCOLLECTION_HPP
#define TELEOP_ARM_JOY_INPUTCOLLECTION_HPP

#include "Input.hpp"

namespace teleop_arm_joy {

template<typename T>
class InputCollection {
public:

  // Container type aliases
  using value_type = std::shared_ptr<Input<T>>;
  using reference = value_type&;
  using const_reference = const value_type&;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;

  std::shared_ptr<Input<T>> operator[](const std::string& key) {
    // Find the element
    const auto& it = items_.find(key);

    // Create a new event if it isn't in the collection
    if (it == items_.end()) {
      const auto new_item = std::make_shared<Input<T>>(key);

      items_[key] = new_item;
      return new_item;
    }

    return it->second;
  }

  template<bool is_const>
  class Iterator {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = std::shared_ptr<Input<T>>;
    using difference_type = std::ptrdiff_t;

    // Conditional types based on const-ness
    using reference = typename std::conditional_t<is_const,
      const value_type&,
      value_type&>;
    using pointer = typename std::conditional_t<is_const,
      const value_type*,
      value_type*>;
    using base_iterator = typename std::conditional_t<is_const,
      typename std::map<std::string, std::shared_ptr<Input<T>>>::const_iterator,
      typename std::map<std::string, std::shared_ptr<Input<T>>>::iterator>;

    explicit Iterator(base_iterator it) : it_(it) {}

    // Convert non-const iterator to const iterator
    template<bool was_const, typename = std::enable_if_t<is_const && !was_const>>
    explicit Iterator(const Iterator<was_const>& other) : it_(other.base()) {}

    reference operator*() const {
      return it_->second;
    }
    pointer operator->() {
      return &(it_->second);
    }

    // Prefix increment
    Iterator& operator++() {
      ++it_;
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

    // Allow access to base iterator for conversion
    base_iterator base() const { return it_; }

  private:
    base_iterator it_;
    // Grant access to other iterator specializations
    template<bool> friend class IteratorImpl;
  };

  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;

  iterator begin() { return iterator(items_.begin()); }
  iterator end() { return iterator(items_.end()); }
  const_iterator begin() const { return const_iterator(items_.begin()); }
  const_iterator end() const { return const_iterator(items_.end()); }
  const_iterator cbegin() const { return const_iterator(items_.begin()); }
  const_iterator cend() const { return const_iterator(items_.end()); }

private:
  std::map<std::string, typename std::shared_ptr<Input<T>>> items_{};
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_INPUTCOLLECTION_HPP
