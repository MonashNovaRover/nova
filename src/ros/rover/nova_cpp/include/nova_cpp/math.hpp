// Copyright (c) 2025 Monash Nova Rover
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NOVA_CPP__MATH_HPP_
#define NOVA_CPP__MATH_HPP_

namespace nova_cpp
{
    /**
     * @brief Returns the sign of a value.
     * @param val The value to check.
     * @return 1 if val is positive, -1 if val is negative, 0 if val is zero.
     */
    template <typename T>
    int sign(T val)
    {
        return (T(0) < val) - (val < T(0));
    }
} // namespace nova_cpp

#endif // NOVA_CPP__MATH_HPP_