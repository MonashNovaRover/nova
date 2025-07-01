#include "../../../../../../../../../../nix/store/wl7xs26116sswgw18pnc3yw9r5gxr6hx-gcc-13.3.0/include/c++/13.3.0/utility"

//
// Created by nova on 7/1/25.
//

#ifndef TELEOP_ARM_JOY_INPUTDECLARATION_HPP
#define TELEOP_ARM_JOY_INPUTDECLARATION_HPP



namespace teleop_arm_joy {

/**
 * The struct used by input sources to export their inputs. Fundamentally, an std::reference_wrapper<T> of the actual
 * data we care about, tied with a name, and any other additional information we might include in the future, like a
 * description.
 */
template<typename T>
struct InputDeclaration {
  std::string name;
  std::reference_wrapper<T> reference;

  InputDeclaration(std::string name, T& reference) : name(std::move(name)), reference(std::ref(reference)) {
    static_assert(!std::is_rvalue_reference<decltype(reference)>::value,
      "The given reference must be memory held by your input source. An InputDeclaration simply tells teleop where the "
      "value of the input is stored, and doesn't own the memory.\n\n"
      "Incorrect:\n\tdeclarations.emplace_back(\"joe\", 0.0);\n"
      "Correct:\n\tdeclarations.emplace_back(\"joe\", this->joe);"
      );
  }
};

}

#endif //TELEOP_ARM_JOY_INPUTDECLARATION_HPP
