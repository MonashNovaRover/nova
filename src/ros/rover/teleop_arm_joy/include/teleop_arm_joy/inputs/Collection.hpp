//
// Created by nova on 6/10/25.
//

#ifndef COLLECTION_HPP
#define COLLECTION_HPP
#include <memory>

namespace teleop_arm_joy {

/**
 * An abstract base class for a collection of things
 * @tparam T The type of objects in the collection
 */
template<typename T>
class Collection {
public:
  virtual ~Collection() = default;

private:
  virtual std::weak_ptr<T> operator[](const std::string& index) = 0;

  /**
   * Adds a given value to the store with the given key.
   * @param key The key to store the value under.
   * @param value The value to put in the store at a key.
   */
  virtual void add(const std::string& key, const std::shared_ptr<T>& value) = 0;
};

} // teleop_arm_joy

#endif //COLLECTION_HPP
