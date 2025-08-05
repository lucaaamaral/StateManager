#pragma once

// Common types and utilities
#include "common/StateManagerTypes.h"

// Core state management
#include "core/IStateManager.h"
#include "core/StateManager.h"

// Storage components
#include "storage/StateStorageIface.h"
#include "storage/RedisStateStorage.h"
#include "storage/RedisKeyManager.h"

// Notification components
#include "notification/IStateNotifier.h"
#include "notification/RedisStateNotifier.h"
#include "notification/RedisChannelManager.h"

namespace StateManager {
    // This header includes all components of the state management system
}