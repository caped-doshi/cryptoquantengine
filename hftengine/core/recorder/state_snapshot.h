/*
 * File: hftengine/core/recorder/state_snapshot.h
 * Description: Defines the StateSnapshot struct. Contains a timestamp,
 * equity, and position.
 * Author: Arvind Rathnashyam
 * Date: 2025-08-14
 * License: Proprietary
 */

#pragma once

#include "../types/aliases/usings.h"

struct StateSnapshot {
    Timestamp timestamp_;
    double equity_;
    double position_;
    double price_;
};