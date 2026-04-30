//
// Taken from teleop_modular
//
// Copyright 2025 Bailey Chessum
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
//
// Created by Bailey Chessum on 7/6/25.
//

#ifndef ARM_KINEMATICS_INPUT_SOURCE_SPAN_HPP
#define ARM_KINEMATICS_INPUT_SOURCE_SPAN_HPP

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <vector>

namespace arm_kinematics
{

/**
 * Substitute for std::span to support more C++ versions
 *
 * \warning This will not work for std::vector<bool>
 */
template<typename T>
struct span
{
  using iterator = T *;
  using element_type = std::remove_const_t<T>;

  T * data_;
  std::size_t size_;

  constexpr span() noexcept
  : data_(nullptr), size_(0)
  {
  }
  constexpr span(T * data, std::size_t size) noexcept
  : data_(data), size_(size)
  {
  }

  template<typename TAlloc>
  constexpr span(std::vector<element_type, TAlloc> & vector) noexcept
  : data_(vector.data()), size_(vector.size())
  {
  }

  template<typename TAlloc, typename U = T, std::enable_if_t<std::is_const_v<U>, int> = 0>
  constexpr span(const std::vector<element_type, TAlloc> & vector) noexcept
  : data_(vector.data()), size_(vector.size())
  {
  }

  template<typename U = T, std::enable_if_t<std::is_const_v<U>, int> = 0>
  constexpr span(span<element_type> other) noexcept
  : data_(other.data_), size_(other.size_)
  {
  }

  constexpr T * data() const noexcept { return data_; }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    return size_;
  }
  [[nodiscard]] constexpr bool empty() const noexcept
  {
    return size_ == 0;
  }

  constexpr T & operator[](std::size_t idx) const
  {
    assert(idx < size_);
    return data_[idx];
  }

  constexpr T & front() const
  {
    assert(size_ > 0);
    return data_[0];
  }

  constexpr T & back() const
  {
    assert(size_ > 0);
    return data_[size_ - 1];
  }

  constexpr iterator begin() const noexcept
  {
    return data_;
  }
  constexpr iterator end() const noexcept
  {
    return data_ + size_;
  }
};

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_INPUT_SOURCE_SPAN_HPP
