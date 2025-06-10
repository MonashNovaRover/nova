//
// Created by Bailey Chessum on 6/9/25.
//

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP
s
#include <map>
#include <string>

#include "CollatedCollection.hpp"
#include "CollatedEvent.hpp"
#include "CollatedInput.hpp"
#include "Input.hpp"

namespace teleop_arm_joy {
/**
 * Class responsible for owning the various maps between input name and input object.
 */
class InputManager {

public:
  // Accessors
  [[nodiscard]] Collection<Input<double>, CollatedInput<double>>& get_axes() {
    return axes_;
  }
  [[nodiscard]] Collection<Input<bool>, CollatedInput<bool>>& get_booleans() {
    return booleans_;
  }
  [[nodiscard]] CollatedCollection<Event, CollatedEvent>& get_events() {
    return events_;
  }

protected:
  CollatedCollection<Input<double>, CollatedInput<double>> axes_{};
  CollatedCollection<Input<bool>, CollatedInput<bool>> booleans_{};

  CollatedCollection<Event, CollatedEvent> events_{};
};

} // teleop_arm_joy

#endif //INPUTMANAGER_HPP
