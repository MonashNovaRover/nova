// Copyright (c) 2021 Samsung Research America
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

#ifndef NOVA_BEHAVIOR_TREE__UTILS_HPP
#define NOVA_BEHAVIOR_TREE__UTILS_HPP

#include <vector>
#include <string>
#include <sstream>

namespace nova_behavior_tree::utils
{
  
    template <typename T>
    std::string vectorToString(const std::vector<T> &vec)
    {
      std::ostringstream oss;
      oss << "[";
      if (!vec.empty())
      {
        for (size_t i = 0; i < vec.size() - 1; ++i)
        {
          oss << vec[i] << ", ";
        }
        oss << vec.back();
      }
      oss << "]";
      return oss.str();
    }

}

#endif // NOVA_BEHAVIOR_TREE__UTILS_HPP_