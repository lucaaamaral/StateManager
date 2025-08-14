#ifndef REDIS_STORAGE_H
#define REDIS_STORAGE_H

#include <memory>
#include <string>

#include "logging/LoggerIface.h"

namespace StateManager {

class RedisStorage {
private:
  std::shared_ptr<logging::LoggerIface> logger_;

  void log(logging::LogLevel level, const std::string &message) const;

public:
  RedisStorage();
  ~RedisStorage();

  // Locking methods
  bool tryLock(const std::string &key) const;
  void unlock(const std::string &key) const;
  bool isLocked(const std::string &key) const;

  // Data manipulation methods
  bool write(const std::string &key, const std::string &value) const;
  bool erase(const std::string &key) const;
  bool update(const std::string &key, const std::string &newValue) const;
  std::string read(const std::string &key) const;
};

} // namespace StateManager

#endif // REDIS_STORAGE_H