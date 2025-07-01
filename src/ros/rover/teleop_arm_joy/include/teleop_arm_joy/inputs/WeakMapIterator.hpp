//
// Created by nova on 7/2/25.
//

#ifndef TELEOP_ARM_JOY_WEAKMAPITERATOR_HPP
#define TELEOP_ARM_JOY_WEAKMAPITERATOR_HPP

#include <utility>
#include <vector>
#include <memory>
#include <map>

namespace teleop_arm_joy {

template<typename T, bool is_const>
class WeakMapIterator {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = std::shared_ptr<T>;
  using difference_type = std::ptrdiff_t;
  using reference = typename std::conditional_t<is_const, const value_type&, value_type&>;
  using pointer = typename std::conditional_t<is_const, const value_type*, value_type*>;
  using map_type = std::map<std::string, std::weak_ptr<T>>;
  using map_pointer = typename std::conditional_t<is_const, const map_type*, map_type*>;
  using base_iterator = typename std::conditional_t<is_const,
    typename map_type::const_iterator,
    typename map_type::iterator>;

  explicit WeakMapIterator(base_iterator it, map_pointer map)
    : it_(it), map_(map), current_value_()
  {
    if (it_ != map_->end()) {
      skip_expired();
    }
  }

  // Convert non-const iterator to const iterator
  template<bool was_const, typename = std::enable_if_t<is_const && !was_const>>
  explicit WeakMapIterator(const WeakMapIterator<T, was_const>& other)
    : it_(other.it_), map_(other.map_), current_value_(other.current_value_) {}

  reference operator*() {
    ensure_current_value();
    return current_value_;
  }

  pointer operator->() {
    ensure_current_value();
    return &current_value_;
  }

  WeakMapIterator& operator++() {
    ++it_;
    skip_expired();
    return *this;
  }

  WeakMapIterator operator++(int) {
    WeakMapIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  friend bool operator==(const WeakMapIterator& a, const WeakMapIterator& b) {
    return a.it_ == b.it_;
  }

  friend bool operator!=(const WeakMapIterator& a, const WeakMapIterator& b) {
    return !(a == b);
  }

  base_iterator base() const { return it_; }

protected:
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

  template<typename U, bool> friend class WeakMapIterator;
};

} // namespace teleop_arm_joy

#endif //TELEOP_ARM_JOY_WEAKMAPITERATOR_HPP
