#pragma once
#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <memory>
#include <mutex>
#include <optional>
#include <functional>

#include <nlohmann/json.hpp>

#include "client/RedisConfig.h"
#include "logging/LoggerIface.h"
#include "return/StateObj.h"

namespace StateManager {

using error = std::optional<std::string>;
using json = nlohmann::json;

class StateManager {
private:
    std::shared_ptr<logging::LoggerIface> logger_;
    void log(logging::LogLevel level, const std::string& message) const;

    RedisConfig config_;
public:
    explicit StateManager(const RedisConfig& config);
    ~StateManager();

    // Configuration methods
    static void setConfig(const RedisConfig& config);

    // Data manipulation methods
    bool write(const std::string& key, const StateObj& value);
    bool erase(const std::string& key);
    std::pair<error, std::unique_ptr<StateObj>> read(const std::string& key, const StateObj& template_obj);

    // Channel manipulation methods
    void subscribe(const std::string& channel, std::function<void(std::unique_ptr<StateObj>)> callback, const StateObj& template_obj);
    void unsubscribe(const std::string& channel);
    bool publish(const std::string& channel, const StateObj& data);
};

} // namespace StateManager

#endif // STATE_MANAGER_H

