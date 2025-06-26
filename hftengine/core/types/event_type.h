/*
 * File: hft_bt_engine/core/market_data/core/update_type.h
 * Description: Enum class defining the two possible L2Update types: Incremental / Snapshot.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-24
 * License: Proprietary
 */

#pragma once

enum class EventType
{
    None,
    Trade,
    L2Update
};