//
// Created by nova on 6/10/25.
//

#ifndef EVENTCOLLECTION_HPP
#define EVENTCOLLECTION_HPP
#include <map>
#include <memory>
#include <string>

#include "../Collection.hpp"
#include "../Event.hpp"

namespace teleop_arm_joy {

/**
 * Base class given to InputSources to allow them to get event objects to invoke.
 */
template <typename T, typename TCollated>
class CollatedCollection : public Collection<T> {
public:
  ~CollatedCollection() override = default;

  std::shared_ptr<T> operator[](const std::string& index) override {
    // Try find the element
    const auto& it = items_.find(index);

    // Create a new collated event if it isn't in the collection
    if (it == items_.end()) {
      const auto new_item = std::make_shared<TCollated>(index);
      
      items_[index] = new_item;
      const std::shared_ptr<T> weak_item = new_item;
      return weak_item;
    }

    // Otherwise make a weak pointer from the existing shared pointer
    std::shared_ptr<T> weak_item = std::static_pointer_cast<T>(it->second);
    return weak_item;
  }

  void add(const std::string& key, const std::shared_ptr<T>& value) override {
    // Try find the element
    const auto& it = items_.find(key);

    // Create a new collated event if it isn't in the collection
    if (it == items_.end()) {
      const auto new_item = std::make_shared<TCollated>(key);
      new_item->add(value);
      items_.insert({key, new_item});
      return;
    }

    std::static_pointer_cast<TCollated>(it->second)->add(value);
  }


  // void add(const std::shared_ptr<T>& value) override {
  //   // Try find the element
  //   const auto& it = items_.find(value->get_name());
  //
  //   // Create a new collated event if it isn't in the collection
  //   if (it == items_.end()) {
  //     const auto new_item = std::make_shared<TCollated>(value->get_name());
  //     new_item->add(value);
  //     items_.insert({value->get_name(), new_item});
  //     return;
  //   }
  //
  //   it->second->add(value);
  // }

  typename Collection<T>::iterator begin() override {
    return items_.begin();
  }

  typename Collection<T>::iterator end() override {
    return items_.end();
  }

  typename Collection<T>::const_iterator begin() const override {
    return items_.begin();
  }

  typename Collection<T>::const_iterator end() const override {
    return items_.end();
  }

private:
  std::map<std::string, std::shared_ptr<T>> items_{};
};

} // teleop_arm_joy

#endif //EVENTCOLLECTION_HPP
