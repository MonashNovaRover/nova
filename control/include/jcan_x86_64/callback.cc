#include "callback.h"
#include "jcan/src/lib.rs.h"
#include <stdio.h>

namespace org::jcan
{
   Bus::Bus() {
      this->jBus = new_jbus().into_raw();
   }

   std::unique_ptr<Bus> new_bus() {
      return std::unique_ptr<Bus>(new Bus());
   }

   void Bus::open(const char *name) {
      this->jBus->open(name);
   }

   void Bus::set_id_filter(std::vector<uint32_t> allowed_ids) {
      // Convert to a Rust Vec by pushing back each element
      auto rust_vec = rust::Vec<uint32_t>();
      for (auto id : allowed_ids) {
         rust_vec.push_back(id);
      }
      this->jBus->set_id_filter(rust_vec);
   }

   void Bus::send(Frame frame) {
      this->jBus->send(frame);
   }

   void Bus::receive() {
      this->jBus->receive();
   }

   std::vector<Frame> Bus::receive_from_thread_buffer() {
      std::vector<Frame> stdv;
      auto frames = this->jBus->receive_from_thread_buffer();
      std::copy(frames.begin(), frames.end(), std::back_inserter(stdv));
      return stdv;
   }

  void Bus::add_callback(int id, void (*callback)(Frame)) {
    this->callbacks_[id] = callback;
  }

  void Bus::spin() {
    // Get a vector of frames from the bus (receive_from_thread_buffer)
    auto frames = this->receive_from_thread_buffer();

    // For each frame, call the callback function associated with the frame's ID
    for (auto frame : frames) {
      auto it = this->callbacks_.find(frame.id);
      // If the ID is not in the map, do nothing
      if (it != this->callbacks_.end())
      {
        // Call the callback function
        it->second(frame);
      }
    }
  }
}