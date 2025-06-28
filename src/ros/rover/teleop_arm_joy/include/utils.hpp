//
// Created by nova on 6/13/25.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <string_view>


inline std::string snake_to_title(const std::string_view in)
{
  std::string out;
  out.reserve(in.size());               // 1‑for‑1 replacement, no re‑allocs

  bool new_word = true;                 // first char should be capitalised
  for (const char ch : in) {
    if (ch == '_') {                  // underscore → space, next char starts a word
      out.push_back(' ');
      new_word = true;
    } else {
      if (new_word && std::isalpha(static_cast<unsigned char>(ch)))
        out.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
      else
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
      new_word = false;
    }
  }
  return out;
}

#endif //UTILS_HPP
