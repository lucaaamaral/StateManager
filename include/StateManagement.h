#pragma once

// Common types and utilities
#include "common/StateManagerTypes.h"

// Core state management
#include "core/IStateManager.h"
#include "core/StateManager.h"

// Storage components
#include "storage/RedisKeyManager.h"
#include "storage/RedisStateStorage.h"
#include "storage/StateStorageIface.h"

// Notification components
#include "notification/IStateNotifier.h"
#include "notification/RedisChannelManager.h"
#include "notification/RedisStateNotifier.h"

namespace StateManager {
// This header includes all components of the state management system
}