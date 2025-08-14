#pragma once
#ifndef STATE_MANAGER_IFACE_H
#define STATE_MANAGER_IFACE_H

#include <string>

using error = std::optional<std::string>;

namespace StateManager {

// Main state manager interface
class StateManagerIface {
public:
  virtual ~StateManagerIface() = default;

  // Data manipulation
  virtual bool write(const std::string &key, const std::string &value) = 0;
  virtual bool erase(const std::string &key) = 0;
  virtual std::pair<error, std::string> read(const std::string &key) = 0;

  // Notification management
  virtual void subscribe(const std::string &key,
                         std::function<void(const std::string &)> callback) = 0;
  virtual void unsubscribe(const std::string &key) = 0;
  virtual void publish(const std::string &key, const std::string &value) = 0;
};

} // namespace StateManager

#endif // STATE_MANAGER_IFACE_H
