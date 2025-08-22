/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

#include "../types/aliases/usings.h"

struct StateSnapshot {
    Timestamp timestamp_;
    double equity_;
    double position_;
    double price_;
};