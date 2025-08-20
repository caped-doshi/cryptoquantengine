/*
 * File: hft_bt_engine/core/types/enums/action_type.h
 * Description: Enum class defining the two possible action types for the
 * backtest engine: submit buy order, submit sell order, cancel order.
 * Author: Arvind Rathnashyam
 * Date: 2025-07-03
 * License: Proprietary
 */

#pragma once
enum class ActionType {
    SubmitBuy,
    SubmitSell,
    Cancel,
    LocalProcessFill,
    LocalBookUpdate,
    LocalOrderUpdate
};