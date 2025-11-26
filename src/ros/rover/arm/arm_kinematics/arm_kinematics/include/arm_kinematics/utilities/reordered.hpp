//
// Created by nova on 22/11/25.
//

#ifndef ARM_KINEMATICS_REORDERED_HPP
#define ARM_KINEMATICS_REORDERED_HPP

#include <vector>
#include "order.hpp"

namespace arm_kinematics {

// Please only use this for temporary values
template<typename TCollection, typename TKey = std::size_t, bool FixedSize = true>
struct Reordered
{
  using key_type = TKey;
  using size_type = std::size_t;

  TCollection & collection{};
  const Order<FixedSize, TKey> & order{0};

  constexpr Reordered(TCollection & collection, const Order<FixedSize, TKey> & order) : collection(collection), order(order) {}

  // Don't allow const reorders to be copied
  // Enable only if T != Forbidden
  template<typename U=TCollection,
           std::enable_if_t<!std::is_same_v<U, const Order<>>, int> = 0>
  Reordered(const Reordered &) = default;

  // Enable only if T == Forbidden
  template<typename U=TCollection,
           std::enable_if_t<std::is_same_v<U, const Order<>>, int> = 0>
  Reordered(const Reordered &) = delete;


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

  struct reverse_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference =
      decltype(std::declval<TCollection&>()[std::declval<TKey>()]);

    Reordered* parent = nullptr;
    size_type index   = 0;  // index into order, counting backwards

    reference operator*() const noexcept {
      return (*parent)[index];
    }

    reverse_iterator& operator++() noexcept {
      --index;
      return *this;
    }

    reverse_iterator operator++(int) noexcept {
      reverse_iterator tmp = *this;
      --index;
      return tmp;
    }

    friend bool operator==(const reverse_iterator& a,
                           const reverse_iterator& b) noexcept {
      return a.parent == b.parent && a.index == b.index;
    }

    friend bool operator!=(const reverse_iterator& a,
                           const reverse_iterator& b) noexcept {
      return !(a == b);
    }
  };

  struct const_reverse_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference =
      const decltype(std::declval<TCollection&>()[std::declval<TKey>()]);

    const Reordered* parent = nullptr;
    size_type index         = 0;

    reference operator*() const noexcept {
      return (*parent)[index];
    }

    const_reverse_iterator& operator++() noexcept {
      --index;
      return *this;
    }

    const_reverse_iterator operator++(int) noexcept {
      const_reverse_iterator tmp = *this;
      --index;
      return tmp;
    }

    friend bool operator==(const const_reverse_iterator& a,
                           const const_reverse_iterator& b) noexcept {
      return a.parent == b.parent && a.index == b.index;
    }

    friend bool operator!=(const const_reverse_iterator& a,
                           const const_reverse_iterator& b) noexcept {
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

  reverse_iterator rbegin() noexcept {
    if (order.size() == 0) return reverse_iterator{this, 0};
    return reverse_iterator{this, order.size() - 1};
  }

  reverse_iterator rend() noexcept {
    // rend() is one past the end of reverse traversal -> index = -1
    return reverse_iterator{this, static_cast<size_type>(-1)};
  }

  const_reverse_iterator rbegin() const noexcept {
    if (order.size() == 0) return const_reverse_iterator{this, 0};
    return const_reverse_iterator{this, order.size() - 1};
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator{this, static_cast<size_type>(-1)};
  }

  const_reverse_iterator crbegin() const noexcept { return rbegin(); }
  const_reverse_iterator crend() const noexcept { return rend(); }

};

} // namespace arm_kinematics

#endif //ARM_KINEMATICS_REORDERED_HPP