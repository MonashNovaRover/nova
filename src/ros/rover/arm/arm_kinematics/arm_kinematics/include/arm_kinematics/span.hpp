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

#include <vector>
#include <cstddef>

namespace arm_kinematics
{

/**
 * Substitute for std::span to support more C++ versions
 */
template<typename T>
struct span
{
  using iterator = T *;

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
  constexpr explicit span(std::vector<T, TAlloc> & vector) noexcept
  : data_(vector.data()), size_(vector.size())
  {
  }
  template<typename TAlloc>
  constexpr explicit span(const std::vector<T, TAlloc> & vector) noexcept
  : data_(vector.data()), size_(vector.size())
  {
  }

  [[nodiscard]] constexpr std::size_t size() const noexcept
  {
    return size_;
  }
  [[nodiscard]] constexpr bool empty() const noexcept
  {
    return size_ == 0;
  }

  constexpr T & operator[](std::size_t idx) const noexcept
  {
    assert(idx < size_);
    return data_[idx];
  }

  constexpr T & front() const noexcept
  {
    assert(size_ > 0);
    return data_[0];
  }

  constexpr T & back() const noexcept
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