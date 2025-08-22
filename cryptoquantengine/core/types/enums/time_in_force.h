/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#pragma once

enum class TimeInForce
{
    GTC, // good till cancel
    GTX, // good till crossing
    FOK, // fill or kill
    IOC  // immediate or cancel
};