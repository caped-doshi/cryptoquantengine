/*
 * File: hftengine/core/recorder/equity_snapshot.h
 * Description: Defines the EquitySnapshot struct. Contains a timestamp 
 * and an equity. 
 * Author: Arvind Rathnashyam 
 * Date: 2025-07-08 
 * License: Proprietary
 */

#pragma once

#include "../types/usings.h"

struct EquitySnapshot {
    Timestamp timestamp_;
    double equity_;
};