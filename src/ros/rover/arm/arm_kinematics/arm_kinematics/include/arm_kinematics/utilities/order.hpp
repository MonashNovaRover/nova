//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_ORDERING_HPP
#define ARM_KINEMATICS_ORDERING_HPP

#include <deque>
#include <vector>

#include "arm_kinematics/span.hpp"

namespace arm_kinematics {

template<typename T = std::size_t>
class Order {
public:
  using value_type      = T;
  using Container       = std::vector<T>;
  using size_type       = typename Container::size_type;
  using const_reference = typename Container::const_reference;
  using iterator        = typename Container::iterator;
  using const_iterator  = typename Container::const_iterator;
  using reverse_iterator       = typename Container::reverse_iterator;
  using const_reverse_iterator = typename Container::const_reverse_iterator;

  struct InverseProxy {
    Order* parent;
    value_type v;

    // Assignment from value_type
    InverseProxy& operator=(const T& idx) {
      parent->set(idx, v);        // side-effect: update inverse map
      return *this;
    }

    // Support chained assignments: a[i] = a[j];
    InverseProxy& operator=(const InverseProxy& other) {
      auto idx = static_cast<T>(other);
      parent->set(idx, v);
      return *this;
    }

    // Conversion to value_type so it behaves like a reference
    operator value_type() const {
      return parent->inverse_map[v];
    }
  };

  struct Proxy {
    Order* parent;
    T idx;

    // Assignment from value_type
    Proxy& operator=(const value_type& v) {
      parent->set(idx, v);        // side-effect: update inverse map
      return *this;
    }

    // Support chained assignments: a[i] = a[j];
    Proxy& operator=(const Proxy& other) {
      auto v = static_cast<value_type>(other);
      parent->set(idx, v);
      return *this;
    }

    // Conversion to value_type so it behaves like a reference
    operator value_type() const {
      return parent->map[idx];
    }
  };

  struct Inverse {
    Order& inverse;

    using reference = InverseProxy;
    using const_reference = const T &;

    reference operator[](T value) noexcept {
      return InverseProxy{&inverse, value};
    }

    const_reference operator[](T value) const noexcept {
      return inverse.inverse_map[value];
    }
  };

  using reference = Proxy;

  explicit Order(size_type in_size, size_type out_size)
    : map(in_size), inverse_map(out_size) {}

  void set(size_type new_id, size_type old_id) {
    map[new_id] = old_id;
    inverse_map[old_id] = new_id;
  }

  // Indexing into the order
  reference operator[](size_type idx) noexcept {
    return Proxy{this, idx};
  }

  const_reference operator[](size_type idx) const noexcept {
    return map[idx];
  }

  // Iteration over the indices
  iterator begin() noexcept
  {
    return map.begin();
  }
  const_iterator begin() const noexcept { return map.begin(); }
  const_iterator cbegin() const noexcept { return map.cbegin(); }

  iterator end() noexcept
  {
    return map.end();
  }
  const_iterator end() const noexcept { return map.end(); }
  const_iterator cend() const noexcept { return map.cend(); }

  reverse_iterator rbegin() noexcept
  {
    return map.rbegin();
  }
  const_reverse_iterator rbegin() const noexcept { return map.rbegin(); }
  const_reverse_iterator crbegin() const noexcept { return map.crbegin(); }

  reverse_iterator rend() noexcept { return map.rend(); }
  const_reverse_iterator rend() const noexcept { return map.rend(); }
  const_reverse_iterator crend() const noexcept { return map.crend(); }

  size_type size() const noexcept { return map.size(); }

  Inverse inverse{this};

private:

  Container map{};
  Container inverse_map{};


  // Lazily evaluated reverse order
  // mutable std::unique_ptr<Order<true, T>> inverse_;
};

} // namespace arm_kinematics

#endif //ARM_KINEMATICS_ORDERING_HPP