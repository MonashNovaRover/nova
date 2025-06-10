//
// Created by nova on 6/10/25.
//

#ifndef EVENTCOLLECTION_HPP
#define EVENTCOLLECTION_HPP
#include <map>
#include <memory>
#include <string>

#include "Collection.hpp"
#include "Event.hpp"

namespace teleop_arm_joy {

/**
 * Base class given to InputSources to allow them to get event objects to invoke.
 */
template <typename T, typename TCollated>
class CollatedCollection final : public Collection<T> {
public:
  ~CollatedCollection() override = default;

  std::weak_ptr<T> operator[](const std::string& index) override {
    // Try find the element
    const auto& it = items_.find(index);

    // Create a new collated event if it isn't in the collection
    if (it == items_.end()) {
      const auto new_item = std::make_shared<TCollated>();
      
      items_[index] = new_item;
      const std::weak_ptr<T> weak_item = new_item;
      return weak_item;
    }

    // Otherwise make a weak pointer from the existing shared pointer
    std::weak_ptr<T> weak_item = std::static_pointer_cast<T>(it->second);
    return weak_item;
  }

  void add(const std::string& key, const std::shared_ptr<T>& value) override {
    // Try find the element
    const auto& it = items_.find(key);

    // Create a new collated event if it isn't in the collection
    if (it == items_.end()) {
      const auto new_item = std::make_shared<TCollated>();
      new_item->add(value);
      items_.insert({key, new_item});
    }

    it->second->add(value);
  }

private:
  std::map<std::string, std::shared_ptr<TCollated>> items_{};
};

} // teleop_arm_joy

#endif //EVENTCOLLECTION_HPP
