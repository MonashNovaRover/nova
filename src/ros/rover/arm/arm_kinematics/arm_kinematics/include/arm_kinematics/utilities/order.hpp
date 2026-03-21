//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_ORDERING_HPP
#define ARM_KINEMATICS_ORDERING_HPP

#include <vector>

namespace arm_kinematics {

/**
 * Represents an ordering/permutation relationship between two indexed collections.
 *
 * The most important mental model is:
 *
 * - `Order` does not primarily describe a semantic relationship between items
 * - `Order` describes where items from one indexed layout appear in another indexed layout
 *
 * In other words, use `Order` when you care about:
 *
 * - reordering a collection
 * - converting between "old index" and "new index"
 * - crossing a boundary where named items are assigned stable internal ids
 * - composing multiple index-layout transformations
 *
 * Do not use `Order` as a general-purpose lookup table when plain ids or arrays are enough.
 *
 * Let the original sequence be indexed 0..N-1.
 *
 * - `order[i]` is the original index of the element that is now at new index `i`
 * - `order.inverse[j]` is the new index of the element that originally was at index `j`
 *
 * That means:
 *
 * - use `order[...]` when you have a new index and need to know which original element lives there
 * - use `order.inverse[...]` when you have an original index and need to know where it moved
 *
 * To create a reordered copy of a vector:
 * \code
 *   std::vector<std::string> names = { ... };
 *   Order<> order(names.size(), names.size());
 *   // ...
 *
 *   auto reordered = order.reorder(names);
 *   // reordered[i] == names[ order[i] ]
 * \endcode
 *
 * `Order` is also useful at representation boundaries.
 * For example, if named items are assigned stable internal ids:
 * \code
 *   Order<std::string, size_t> ids;
 *   // ids[new_id] gives the original named key stored at that id
 *   // ids.inverse[original_name_index] gives the assigned internal id
 * \endcode
 *
 * If you don't want to modify the original collection, the \c Reordered helper lets you index through the
 * permutation without copying the original container:
 * \code
 *   std::vector<std::string> names = { ... };
 *   Order<> order(names.size(), names.size());
 *   // ...
 *
 *   auto view = Reordered(names, order);
 *   // view[i] gives names[ order[i] ] without copying
 * \endcode
 *
 * Orders can also be composed. If `r` maps A -> B and `l` maps B -> C, then `l * r` maps A -> C.
 * This is useful when several layout changes happen in sequence and you want one combined transform.
 *
 * \tparam TKey the type used to index into the underlying collection
 * \tparam TValue the type stored in the collection for indices
 * \tparam StoresInverse
 *   - When true, this specialization owns the forward mapping and stores the inverse mapping internally.
 *   - When false, this specialization represents the inverse view, referring back to the owning forward specialization.
 */
template<typename TKey = std::size_t, typename TValue = std::size_t, bool StoresInverse = true>
class Order {
public:
  using Container        = std::vector<TValue>;
  using InverseContainer = std::vector<TKey>;

  using InverseType      = Order<TValue, TKey, !StoresInverse>;
  /// When StoresInverse=true, this stores the actual inverse directly, otherwise it is a reference to its owning parent
  using InverseRef       = std::conditional_t<
    StoresInverse,
    Order<TValue, TKey, false>,
    Order<TValue, TKey, true> &
  >;

  using value_type      = TValue;
  using size_type       = typename Container::size_type;
  using const_reference = typename Container::const_reference;
  using const_iterator  = typename Container::const_iterator;
  using const_reverse_iterator = typename Container::const_reverse_iterator;

  /**
   * This is the type returned when indexing into the forward order.
   *
   * Assigning through this proxy updates both sides of the permutation:
   * `order[i] = v;` also performs `order.inverse[v] = i;`.
   */
  struct Proxy {
    Order & parent;
    TKey idx;

    // Assignment from value_type
    Proxy & operator=(const value_type& value) {
      parent.set(idx, value);   //< side-effect -- update inverse map
      return *this;
    }

    // Support chained assignments: a[i] = a[j];
    Proxy & operator=(const Proxy& other) {
      auto v = static_cast<value_type>(other);
      parent.set(idx, v);
      return *this;
    }

    // Conversion to value_type so it behaves like a reference
    operator value_type() const {
      return parent.data_[idx];
    }

  };
  using reference = Proxy;

  friend void swap(Proxy a, Proxy b) noexcept {
    // Read values
    auto old_a = static_cast<value_type>(a);
    auto old_b = static_cast<value_type>(b);
    a.parent.set(a.idx, old_b);
    b.parent.set(b.idx, old_a);
  }

  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference         = Proxy;

    Order & parent;
    size_type index = 0;

    reference operator*() const noexcept {
      return parent[index];
    }

    iterator & operator++() noexcept {
      ++index;
      return *this;
    }

    iterator operator++(int) noexcept {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(const iterator & a, const iterator & b) noexcept {
      return a.index == b.index && &a.parent == &b.parent;
    }
    friend bool operator!=(const iterator & a, const iterator & b) noexcept { return !(a == b); }
  };

  struct reverse_iterator {
    using iterator_category = std::bidirectional_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using reference         = Proxy;

    Order & parent;
    size_type index   = 0;  // index into order, counting backwards

    reference operator*() const noexcept {
      return parent[index];
    }

    reverse_iterator & operator++() noexcept {
      --index;
      return *this;
    }

    reverse_iterator operator++(int) noexcept {
      reverse_iterator tmp = *this;
      --index;
      return tmp;
    }

    friend bool operator==(const reverse_iterator & a, const reverse_iterator & b) noexcept {
      return &a.parent == &b.parent && a.index == b.index;
    }
    friend bool operator!=(const reverse_iterator& a, const reverse_iterator& b) noexcept { return !(a == b); }
  };

  // Forward constructor -- only enabled if StoresInverse
  template<bool B = StoresInverse, std::enable_if_t<B, int> = 0>
  explicit Order(size_t in_size, size_t out_size)
    : inverse(out_size, *this), data_(in_size) {}

  // Forward default constructor -- only enabled if StoresInverse
  template<bool B = StoresInverse, std::enable_if_t<B, int> = 0>
  explicit Order()
    : inverse(0, *this), data_(0) {}

  // Forward copy constructor -- only enabled if StoresInverse
  template<bool OtherStoresInverse, bool B = StoresInverse, std::enable_if_t<B, int> = 0>
  explicit Order(const Order<TKey, TValue, OtherStoresInverse> & other)
    : inverse(other.inverse.data_, *this), data_(other.data_) {}

  // Forward move constructor -- only enabled if StoresInverse
  template<bool OtherStoresInverse, bool B = StoresInverse, std::enable_if_t<B, int> = 0>
  explicit Order(Order<TKey, TValue, OtherStoresInverse> && other) noexcept
    : inverse(std::move(other.inverse.data_), *this), data_(std::move(other.data_)) {}

  // Forward copy assignment -- only enabled if StoresInverse
  template<bool OtherStoresInverse>
  Order & operator=(const Order<TKey, TValue, OtherStoresInverse> & other) {
    data_ = other.data_;
    inverse.data_ = other.data_;
    return *this;
  }

  // Forward move assignment -- only enabled if StoresInverse
  template<bool OtherStoresInverse>
  Order & operator=(Order<TKey, TValue, OtherStoresInverse> && other) noexcept {
    data_ = std::move(other.data_);
    inverse.data_ = std::move(other.data_);
    return *this;
  }

  void set(size_type new_id, value_type old_id) {
    data_[new_id] = old_id;
    inverse.data_[old_id] = new_id;
  }

  // Indexing into the order
  // `order[new_index] -> old_index`
  reference operator[](size_type idx) noexcept {
    return Proxy{*this, idx};
  }
  // `order[new_index] -> old_index`
  const_reference operator[](size_type idx) const noexcept {
    return data_[idx];
  }

  // Iteration over the indices
  iterator begin() noexcept { return {*this, 0}; }
  const_iterator begin() const noexcept { return data_.begin(); }
  const_iterator cbegin() const noexcept { return data_.cbegin(); }

  iterator end() noexcept { return {*this, size()}; }
  const_iterator end() const noexcept { return data_.end(); }
  const_iterator cend() const noexcept { return data_.cend(); }

  reverse_iterator rbegin() noexcept { return {*this, size() - 1}; }
  const_reverse_iterator rbegin() const noexcept { return data_.rbegin(); }
  const_reverse_iterator crbegin() const noexcept { return data_.crbegin(); }

  reverse_iterator rend() noexcept { return {*this, static_cast<size_type>(-1)}; }
  const_reverse_iterator rend() const noexcept { return data_.rend(); }
  const_reverse_iterator crend() const noexcept { return data_.crend(); }

  size_type size() const noexcept { return data_.size(); }

  /**
   * Creates a copy of a given vector with elements from indices in original for each element in the order.
   * @tparam U The type contained in the vector
   * @param original The vector to take elements from
   * @return A copy of the original vector, with the items sorted to match the order
   */
  template<typename U, typename UAlloc>
  [[nodiscard]] std::vector<U, UAlloc> reorder(const std::vector<U, UAlloc> & original) const noexcept {
    if (original.empty())
      return {};

    std::vector<U, UAlloc> mapped{};
    mapped.reserve(data_.size());

    for (const auto & i : data_) {
      mapped.emplace_back(std::move(original[i]));
    }

    return mapped;
  }

  /**
   * Creates a copy of a given vector with elements from indices in original for each element in the order.
   * @tparam U The type contained in the vector
   * @param original The vector to take elements from
   * @return A copy of the original vector, with the items sorted to match the order
   */
  template<typename U, typename UAlloc>
  [[nodiscard]] std::vector<U, UAlloc> reorder(std::vector<U, UAlloc> && original) const noexcept {
    if (original.empty())
      return {};

    std::vector<U, UAlloc> mapped{};
    mapped.reserve(data_.size());

    for (const auto & i : data_) {
      mapped.emplace_back(std::move(original[i]));
    }

    original.clear();
    return mapped;
  }

  [[nodiscard]] std::string to_string() const {
    std::stringstream ss;
    ss << "Order{";

    auto first = true;

    for (const auto value : data_) {
      if (!first)
        ss << ",";
      ss << std::to_string(value);
      first = false;
    }
    ss << "}";
    return ss.str();
  }

  /**
   * Stores the inverse mapping for this order:
   * `inverse[old_index] -> new_index`.
   */
  InverseRef inverse{};

private:
  // Allow all Order<*,*,*> specializations to access private members
  template<typename, typename, bool>
  friend class Order;

  // Forward constructor for data directly -- only enabled if StoresInverse
  template<bool B = StoresInverse, std::enable_if_t<B, int> = 0>
  explicit Order(Container forward_data, typename InverseType::Container inverse_data)
    : inverse(std::move(inverse_data), *this), data_(std::move(forward_data)) {}

  // Inverse constructor -- only enabled if not StoresInverse
  template<bool B = StoresInverse, std::enable_if_t<!B, int> = 0>
  explicit Order(size_type out_size, Order<TValue, TKey, true> & parent)
    : inverse(parent), data_(out_size) {}

  // Inverse constructor -- only enabled if not StoresInverse
  template<bool B = StoresInverse, std::enable_if_t<!B, int> = 0>
  explicit Order(Container data, Order<TValue, TKey, true> & parent)
    : inverse(parent), data_(std::move(data)) {}

  Container data_{};
};

/// Function style composition, where the resulting order applies `r` first, then `l`.
/// If `r` maps A -> B and `l` maps B -> C, then `l * r` maps A -> C.
template<typename TA, typename TB, typename TC, bool StoresInverseL, bool StoresInverseR>
Order<TA, TB, true>
operator*(
  const Order<TB, TC, StoresInverseL>& l,
  const Order<TA, TB, StoresInverseR>& r)
{
  Order<TA, TC, true> order(r.size(), l.inverse.size());

  // TODO: Generalize to TA other than size_t
  for (size_t i = 0; i < r.size(); i++) {
    order[i] = l[r[i]];
  }

  return order;
}

/// Function style composition with a vector
template<typename TKey, typename TValue, typename TKeyAlloc, typename TValueAlloc = std::allocator<TKey>, bool StoresInverse>
std::vector<TValue, TValueAlloc> operator*(
  const Order<TKey, TValue, StoresInverse>& order,
  const std::vector<TKey, TKeyAlloc>& indices)
{
  std::vector<TValue> out;
  out.reserve(indices.size());

  for (const auto i : indices) {
    out.push_back(order[i]);
  }

  return out;
}

/// Function style composition with a vector
template<typename TValue, typename TValueAlloc, bool StoresInverse>
std::vector<TValue> operator*(
  const std::vector<TValue, TValueAlloc>& indices,
  const Order<size_t, size_t, StoresInverse>& order)
{
  std::vector<TValue, TValueAlloc> out;
  out.reserve(order.size());

  for (size_t i : order) {
    out.push_back(order[i]);
  }

  return out;
}


} // namespace arm_kinematics

#endif //ARM_KINEMATICS_ORDERING_HPP
