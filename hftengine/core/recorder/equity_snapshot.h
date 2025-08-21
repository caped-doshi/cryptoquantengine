/*
 * File: hftengine/core/recorder/equity_snapshot.h
 * Description: Defines the EquitySnapshot struct. Contains a timestamp
 * and an equity. Used for testing/debugging purposes.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-08
 * License: Proprietary
 */

#pragma once

#include "../types/aliases/usings.h"

struct EquitySnapshot {
    Timestamp timestamp_;
    double equity_;
};