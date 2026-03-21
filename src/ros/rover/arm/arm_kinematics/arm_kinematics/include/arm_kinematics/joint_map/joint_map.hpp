//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_HPP
#define ARM_KINEMATICS_JOINT_MAP_HPP

#include <memory>
#include <utility>
#include <vector>

#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Helper class to map between one parameterized joint space and another related joint space.
 *
 * This is the runtime wrapper around a concrete mapping implementation. The default concrete implementation is
 * AffineJointMap, which handles reordering, duplication, and mimic-joint-style affine remapping.
 *
 * Future implementations may support more general propagation of related joint values, such as transmissions or other
 * plugin-specific mapping strategies.
 *
 * \see JointMapBuilder
 */
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

  /**
   * \brief Maps all the given input joint positions to output joint positions.
   * \note The values in inputs and outputs should correspond to the input and output name spaces used to build this map.
   *
   * \param[in] inputs The position values for each input element in the source joint space.
   * \param[out] outputs The position values for each output element in the target joint space.
   *
   * \warning inputs and outputs must be pre-allocated to the correct size!
   * \warning inputs and outputs must not point to the same memory, or be any internal storage from the implementation.
   */
  void map(span<const double> inputs, span<float> outputs) const;

  /**
   * std::vector helper overload for map(span<const double>, span<float>).
   */
  void map(const std::vector<double> & inputs, std::vector<float> & outputs) const;

  /// The number of elements expected in the input joint space for this map.
  [[nodiscard]] size_t input_count() const noexcept;
  /// The number of elements produced in the output joint space for this map.
  [[nodiscard]] size_t output_count() const noexcept;
  /// Returns false if this instance is default constructed and has no mapping implementation.
  [[nodiscard]] bool valid() const noexcept { return impl_ != nullptr; }

private:
  /**
   * Runtime polymorphic concept for concrete mapping implementations.
   *
   * This interface is intentionally minimal. It only describes runtime mapping behavior and size queries, and does not
   * include any builder- or planning-facing operations.
   */
  struct Concept {
    virtual ~Concept() = default;
    virtual void map(span<const double> inputs, span<float> outputs) const = 0;
    [[nodiscard]] virtual size_t input_count() const noexcept = 0;
    [[nodiscard]] virtual size_t output_count() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<Concept> clone() const = 0;
  };

  /**
   * Adapts a concrete implementation type to the Concept interface.
   */
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
