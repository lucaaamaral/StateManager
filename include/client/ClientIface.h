#pragma once

#include <string>

namespace StateManager {

// Interface for connection to external resources
class ClientIface {
public:
    virtual ~ClientIface() = default;
    
    virtual bool isConnected() const = 0;
    virtual void connect() = 0;
};

} // namespace StateManager