#pragma once

#include <queue>
#include "properties.h"

namespace esphome {
namespace danfoss_eco {

enum class CommandType { READ, WRITE };

class Command {
 public:
  Command(CommandType type, std::shared_ptr<DeviceProperty> property) : type(type), property(property) {}
  bool execute(ble_client::BLEClient *client) {
    if (type == CommandType::READ) return property->read_request(client);
    auto writable = std::static_pointer_cast<WritableProperty>(property);
    return writable->write_request(client);
  }

 protected:
  CommandType type;
  std::shared_ptr<DeviceProperty> property;
};

class CommandQueue {
 public:
  void push(Command *cmd) { queue_.push(cmd); }
  Command *pop() {
    if (queue_.empty()) return nullptr;
    Command *cmd = queue_.front();
    queue_.pop();
    return cmd;
  }
  bool empty() const { return queue_.empty(); }

 protected:
  std::queue<Command *> queue_;
};

} // namespace danfoss_eco
} // namespace esphome