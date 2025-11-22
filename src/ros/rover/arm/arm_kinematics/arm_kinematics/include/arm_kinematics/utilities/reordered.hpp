//
// Created by nova on 22/11/25.
//

#ifndef ARM_KINEMATICS_REORDERED_HPP
#define ARM_KINEMATICS_REORDERED_HPP
#include <vector>

#include "order.hpp"

namespace arm_kinematics {

template<typename TCollection, typename TKey = std::size_t, bool FixedSize = true>
struct Reordered
{
  using key_type = TKey;
  using size_type = std::size_t;

  TCollection & collection;
  Order<FixedSize, TKey> & order;

  /**
   * Get the value at the key for this id in the order
   */
  [[nodiscard]] const auto & operator[](const size_type & id) const noexcept {
    return collection[order[id]];
  }

  /**
   * Get the value at the key for this id in the order
   */
  [[nodiscard]] auto & operator[](const size_type & id) noexcept {
    return collection[order[id]];
  }

  // iterator types

  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference         = decltype(std::declval<TCollection&>()[std::declval<key_type>()]);

    Reordered* parent = nullptr;
    size_type index   = 0;   // index into `order`

    reference operator*() const noexcept {
      return (*parent)[index];
    }

    iterator& operator++() noexcept {
      ++index;
      return *this;
    }

    iterator operator++(int) noexcept {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const iterator& a, const iterator& b) noexcept {
      return a.parent == b.parent && a.index == b.index;
    }

    friend bool operator!=(const iterator& a, const iterator& b) noexcept {
      return !(a == b);
    }
  };

  struct const_iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference         = const decltype(std::declval<TCollection&>()[std::declval<key_type>()]);

    const Reordered* parent = nullptr;
    size_type index         = 0;

    reference operator*() const noexcept {
      return (*parent)[index];
    }

    const_iterator& operator++() noexcept {
      ++index;
      return *this;
    }

    const_iterator operator++(int) noexcept {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const const_iterator& a, const const_iterator& b) noexcept {
      return a.parent == b.parent && a.index == b.index;
    }

    friend bool operator!=(const const_iterator& a, const const_iterator& b) noexcept {
      return !(a == b);
    }
  };

  // begin/end
  iterator begin() noexcept {
    return iterator{this, 0};
  }

  iterator end() noexcept {
    return iterator{this, order.size()};
  }

  const_iterator begin() const noexcept {
    return cbegin();
  }

  const_iterator end() const noexcept {
    return cend();
  }

  const_iterator cbegin() const noexcept {
    return const_iterator{this, 0};
  }

  const_iterator cend() const noexcept {
    return const_iterator{this, order.size()};
  }
};

} // namespace arm_kinematics

#endif //ARM_KINEMATICS_REORDERED_HPP