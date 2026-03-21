//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_HPP
#define ARM_KINEMATICS_JOINT_MAP_HPP

#include <memory>
#include <utility>
#include <vector>

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC JointMap {
public:
  JointMap() = default;
  JointMap(const JointMap & other);
  JointMap(JointMap && other) noexcept = default;
  JointMap & operator=(const JointMap & other);
  JointMap & operator=(JointMap && other) noexcept = default;

  template<class Impl>
  explicit JointMap(Impl impl)
  : impl_(std::make_unique<Model<Impl>>(std::move(impl)))
  {
  }

  void map(span<const double> inputs, span<float> outputs) const;
  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const;

  [[nodiscard]] size_t input_count() const noexcept;
  [[nodiscard]] size_t output_count() const noexcept;
  [[nodiscard]] bool valid() const noexcept { return impl_ != nullptr; }

private:
  struct Concept {
    virtual ~Concept() = default;
    virtual void map(span<const double> inputs, span<float> outputs) const = 0;
    [[nodiscard]] virtual size_t input_count() const noexcept = 0;
    [[nodiscard]] virtual size_t output_count() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<Concept> clone() const = 0;
  };

  template<class Impl>
  struct Model final : Concept {
    explicit Model(Impl impl) : impl(std::move(impl)) {}

    void map(span<const double> inputs, span<float> outputs) const override { impl.map(inputs, outputs); }
    [[nodiscard]] size_t input_count() const noexcept override { return impl.input_count(); }
    [[nodiscard]] size_t output_count() const noexcept override { return impl.output_count(); }
    [[nodiscard]] std::unique_ptr<Concept> clone() const override {
      return std::make_unique<Model<Impl>>(impl);
    }

    Impl impl;
  };

  std::unique_ptr<Concept> impl_ = nullptr;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_HPP
