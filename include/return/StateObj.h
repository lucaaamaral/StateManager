#pragma once

#include <nlohmann/json.hpp>

namespace StateManager {

using json = nlohmann::json;

// Abstract base class that enforces JSON serialization contract
class StateObj {
public:
    virtual ~StateObj() = default;
    
    // Pure virtual methods that child classes must implement
    virtual json to_json() const = 0;
    virtual void from_json(const json& j) = 0;
    
    // Clone method for polymorphic copying
    virtual std::unique_ptr<StateObj> clone() const = 0;
    
protected:
    StateObj() = default;
    StateObj(const StateObj&) = default;
    StateObj& operator=(const StateObj&) = default;
    StateObj(StateObj&&) = default;
    StateObj& operator=(StateObj&&) = default;
};


} // namespace StateManager

// Macro to enforce StateObj inheritance and generate both nlohmann and StateObj methods
#define STATE_OBJ_DEFINE_TYPE(Type, ...)  \
    static_assert(std::is_base_of_v<::StateManager::StateObj, Type>, #Type " must inherit from StateObj"); \
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Type, __VA_ARGS__) \
    nlohmann::json Type::to_json() const { \
        nlohmann::json j; \
        nlohmann::to_json(j, *this); \
        return j; \
    } \
    void Type::from_json(const nlohmann::json& j) { \
        nlohmann::from_json(j, *this); \
    } \
    std::unique_ptr<::StateManager::StateObj> Type::clone() const { \
        return std::make_unique<Type>(*this); \
    }
