//
// Created by nova on 22/11/25.
//

#ifndef ARM_KINEMATICS_ANALYSIS_SUBTREE_H
#define ARM_KINEMATICS_ANALYSIS_SUBTREE_H

#include "analysis_tree.hpp"
#include <arm_kinematics/utilities/order.hpp>
#include <arm_kinematics/utilities/reordered.hpp>

namespace arm_kinematics
{

/**
 * A subtree of an AnalysisTree with a unique root
 */
class AnalysisSubtree
{
public:
  AnalysisSubtree(
    const AnalysisTree & tree,
    const std::string & root_name,
    const FrameDefinitions & definitions);

  const AnalysisTree & tree;
  // Provides order to index into tree
  Order<> topological{0};

  // Reversed mask, in subtree indices
  [[nodiscard]] constexpr Reordered<const std::vector<bool>> get_reversed_mask() const noexcept
  {
    return Reordered{
      static_cast<const std::vector<bool> &>(reversed_mask_old_),
      topological
    };
  }

  // Subtree sizes, in subtree indices
  [[nodiscard]] constexpr Reordered<const std::vector<size_t>> get_subtree_sizes() const noexcept
  {
    return Reordered{
      static_cast<const std::vector<size_t> &>(subtree_sizes_old_),
      topological
    };
  }

  // Reversed mask, in subtree indices
  [[nodiscard]] constexpr Reordered<std::vector<bool>> get_reversed_mask() noexcept
  {
    return Reordered{
      reversed_mask_old_,
      topological
    };
  }

  // Subtree sizes, in subtree indices
  [[nodiscard]] constexpr Reordered<std::vector<size_t>> get_subtree_sizes() noexcept
  {
    return Reordered{
      subtree_sizes_old_,
      topological
    };
  }

  [[nodiscard]] constexpr Order<> & get_inverse_topological() const noexcept {
    return inverse_topological_.size() == 0 ? inverse_topological_ = topological.inverse() : inverse_topological_;
  }


  /**
   * Reversed path, indexed by path index, outputs subtree index
   * \warning This value will immediately segfault if you so much as look at reversed_path_old_ in a non-const way.
   */
  [[nodiscard]] constexpr Reordered<Order<>> get_reversed_path() const noexcept
  {
    return Reordered{
      get_inverse_topological(),
      reversed_path_old_
    };
  }


private:
  explicit AnalysisSubtree(const AnalysisTree & tree) : tree(tree) {}

  std::vector<size_t> frame_parents;
  Isometry3dVector frame_origins;

  size_t root_joint_id_ = 0;
  size_t reversed_path_end_ = 0;

  std::vector<bool> reversed_mask_old_{};
  std::vector<size_t> subtree_sizes_old_{};

  Order<> reversed_path_old_{0};

  [[nodiscard]] constexpr Order<> & get_inverse_reversed_path_old() const noexcept {
    return inverse_reversed_path_old_.size() == 0
      ? inverse_reversed_path_old_ = reversed_path_old_.inverse()
      : inverse_reversed_path_old_;
  }
  mutable Order<> inverse_reversed_path_old_{0};

  // Set by get_reversed_path()
  mutable Order<> inverse_topological_{0};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_ANALYSIS_SUBTREE_H