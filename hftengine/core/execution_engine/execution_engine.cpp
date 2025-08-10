/*
 * File: hftengine/core/execution_engine/execution_engine.cpp
 * Description: Class for execution engine to take orders, fill
 * orders.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

/* MULTIPLE gtx_book NEEDS UNIT TESTS*/

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include "../trading/fill.h"
#include "../trading/order_update.h"
#include "../types/book_side.h"
#include "../types/order_type.h"
#include "../types/time_in_force.h"
#include "../types/usings.h"
#include "execution_engine.h"

/**
 * @brief Default constructor for the ExecutionEngine.
 *
 * Initializes a new instance of the ExecutionEngine with default-initialized
 * internal data structures. The order books, order tracking, and asset-related
 * state will need to be set up through subsequent calls.
 */
ExecutionEngine::ExecutionEngine() {}

void ExecutionEngine::add_asset(int asset_id) {
    if (orderbooks_.find(asset_id) == orderbooks_.end()) {
        orderbooks_[asset_id] = OrderBook();
        active_orders_[asset_id] = std::vector<std::shared_ptr<Order>>();
    }
}

/**
 * @brief Registers a new asset in the execution engine.
 *
 * Initializes internal data structures (such as the order book and active
 * orders list) for the specified asset if it hasn't been added already.
 *
 * @param asset_id The unique identifier of the asset to be tracked.
 */
bool ExecutionEngine::clear_inactive_orders(int asset_id) { return true; }

/**
 * @brief Cancels an active order in the execution engine.
 *
 * This method attempts to cancel an existing order identified by its `orderId`
 * for a specific `asset_id`. It removes the order from all relevant internal
 * tracking structures, including the global order map, price-level order books,
 * and the active orders list.
 *
 * @param asset_id The ID of the asset associated with the order.
 * @param orderId The unique identifier of the order to cancel.
 * @return true if the order was found and successfully cancelled; false
 * otherwise.
 */
bool ExecutionEngine::cancel_order(int asset_id, const OrderId &orderId,
                                   const Timestamp &current_timestamp) {
    auto it = orders_.find(orderId);
    if (it == orders_.end()) return false;

    auto order = it->second;
    if (order->side_ == BookSide::Bid) {
        auto bid_it = gtx_books_[asset_id].bid_orders_.find(order->price_);
        if (bid_it != gtx_books_[asset_id].bid_orders_.end() &&
            bid_it->second->orderId_ == orderId) {
            gtx_books_[asset_id].bid_orders_.erase(bid_it);
        }
    } else {
        auto ask_it = gtx_books_[asset_id].ask_orders_.find(order->price_);
        if (ask_it != gtx_books_[asset_id].ask_orders_.end() &&
            ask_it->second->orderId_ == orderId) {
            gtx_books_[asset_id].ask_orders_.erase(ask_it);
        }
    }
    // Remove from active_orders_ vector for this asset
    auto &active_vec = active_orders_[asset_id];
    active_vec.erase(std::remove_if(active_vec.begin(), active_vec.end(),
                                    [&](const std::shared_ptr<Order> &o) {
                                        return o->orderId_ == orderId;
                                    }),
                     active_vec.end());
    // Remove from global orders map
    orders_.erase(it);
    Timestamp local_timestamp = current_timestamp + order_response_latency_us_;
    order_updates_.emplace_back(
        OrderUpdate{.exch_timestamp_ = current_timestamp,
                    .local_timestamp_ = local_timestamp,
                    .asset_id_ = asset_id,
                    .orderId_ = orderId,
                    .event_type_ = OrderEventType::Cancelled,
                    .order_ = std::nullopt});
    return true;
}

/**
 * @brief Checks if an order with the given ID exists in the execution engine.
 *
 * This method searches the internal order map to determine whether an order
 * with the specified OrderId has been submitted and is currently tracked by
 * the engine (e.g., active or pending execution).
 *
 * @param orderId The unique identifier of the order to check.
 * @return true if the order exists in the engine; false otherwise.
 */
bool ExecutionEngine::order_exists(const OrderId &orderId) const {
    return orders_.find(orderId) != orders_.end();
}

/**
 * @brief Executes a market order against the order book.
 *
 * This function attempts to fill a market buy or sell order by consuming
 * liquidity from the opposing side of the order book (i.e., sell orders consume
 * bids, and buy orders consume asks). It traverses the price levels from best
 * price onward, filling as much of the order as available at each level.
 *
 * At each level, the function compares the available depth with the remaining
 * unfilled quantity of the order. If sufficient liquidity is available, it
 * performs a full fill for the remaining quantity. Otherwise, it partially
 * fills and proceeds to the next price level.
 *
 * All generated fills are recorded in the internal `fills_` vector, and the
 * order’s `filled_quantity_` is updated accordingly. The order is passed as a
 * shared pointer to maintain shared state across components (e.g., strategy,
 * book, execution engine).
 *
 * @param asset_id Identifier of the asset being traded.
 * @param side Direction of the trade (TradeSide::Buy or TradeSide::Sell).
 * @param order Shared pointer to the market order to be executed.
 *              The order's fill state will be updated in-place.
 */
void ExecutionEngine::execute_market_order(int asset_id, TradeSide side,
                                           std::shared_ptr<Order> order) {
    int level = 0;
    int levels = (side == TradeSide::Buy) ? orderbooks_[asset_id].ask_levels()
                                          : orderbooks_[asset_id].bid_levels();
    while (order->filled_quantity_ < order->quantity_ && level < levels) {
        Quantity level_depth =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].depth_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].depth_at_level(BookSide::Bid, level);
        Price level_price =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].price_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].price_at_level(BookSide::Bid, level);
        if (level_depth > (order->quantity_ - order->filled_quantity_)) {
            fills_.emplace_back(
                Fill{.asset_id_ = asset_id,
                     .exch_timestamp_ = order->exch_timestamp_,
                     .local_timestamp_ =
                         order->exch_timestamp_ + order_response_latency_us_,
                     .orderId_ = order->orderId_,
                     .side_ = side,
                     .price_ = level_price,
                     .quantity_ = order->quantity_ - order->filled_quantity_,
                     .is_maker = false});
            order->filled_quantity_ = order->quantity_;
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});
        } else {
            fills_.emplace_back(
                Fill{.asset_id_ = asset_id,
                     .exch_timestamp_ = order->exch_timestamp_,
                     .local_timestamp_ =
                         order->exch_timestamp_ + order_response_latency_us_,
                     .orderId_ = order->orderId_,
                     .side_ = side,
                     .price_ = level_price,
                     .quantity_ = level_depth,
                     .is_maker = false});
            order->filled_quantity_ += level_depth;
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});
        }
        level++;
    }
}

/**
 * @brief Executes a Limit Fill-Or-Kill (FOK) order against the order book.
 *
 * A Fill-Or-Kill (FOK) order must be fully executed immediately at the
 * specified limit price or better. If insufficient liquidity is available at
 * acceptable price levels at the time of submission, the order is rejected and
 * no fills occur.
 *
 * This function first scans the relevant side of the order book (asks for buys,
 * bids for sells) to determine whether enough volume is available at prices
 * equal to or better than the order's limit price.
 *
 * If sufficient liquidity exists, the order is executed in full using
 * aggressive taker fills at the available price levels. If not, the function
 * exits early without modifying the order or generating any fills.
 *
 * All successful fills are appended to the internal `fills_` vector, and the
 * order's `filled_quantity_` is updated in-place. No partial fills are allowed.
 *
 * @param asset_id Identifier of the asset being traded.
 * @param side Trade direction: TradeSide::Buy or TradeSide::Sell.
 * @param order Shared pointer to the FOK order to be executed.
 *              The order's `filled_quantity_` will be updated if filled.
 * @return true if the order was fully filled; false if rejected with no fills.
 */
bool ExecutionEngine::execute_fok_order(int asset_id, TradeSide side,
                                        std::shared_ptr<Order> order) {
    int level = -1;
    int levels = (side == TradeSide::Buy) ? orderbooks_[asset_id].ask_levels()
                                          : orderbooks_[asset_id].bid_levels();
    Quantity available_qty;
    while (++level < levels && available_qty < order->quantity_) {
        Price level_price =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].price_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].price_at_level(BookSide::Bid, level);
        if (side == TradeSide::Buy && level_price > order->price_) break;
        if (side == TradeSide::Sell && level_price < order->price_) break;
        available_qty +=
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].depth_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].depth_at_level(BookSide::Bid, level);
    }
    if (available_qty < order->quantity_) return false;
    level = -1;
    while (++level < levels && order->filled_quantity_ < order->quantity_) {
        Quantity level_depth =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].depth_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].depth_at_level(BookSide::Bid, level);
        Price level_price =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].price_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].price_at_level(BookSide::Bid, level);
        if (side == TradeSide::Buy && level_price > order->price_) break;
        if (side == TradeSide::Sell && level_price < order->price_) break;
        if (level_depth > (order->quantity_ - order->filled_quantity_)) {
            fills_.emplace_back(
                Fill{.asset_id_ = asset_id,
                     .exch_timestamp_ = order->exch_timestamp_,
                     .local_timestamp_ =
                         order->exch_timestamp_ + order_response_latency_us_,
                     .orderId_ = order->orderId_,
                     .side_ = TradeSide::Buy,
                     .price_ = level_price,
                     .quantity_ = order->quantity_ - order->filled_quantity_,
                     .is_maker = false});
            order->filled_quantity_ = order->quantity_;
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});
        } else {
            fills_.emplace_back(
                Fill{.asset_id_ = asset_id,
                     .exch_timestamp_ = order->exch_timestamp_,
                     .local_timestamp_ =
                         order->exch_timestamp_ + order_response_latency_us_,
                     .orderId_ = order->orderId_,
                     .side_ = TradeSide::Buy,
                     .price_ = level_price,
                     .quantity_ = level_depth,
                     .is_maker = false});
            order->filled_quantity_ += level_depth;
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});
        }
    }
    return true;
}

/**
 * @brief Executes a Limit Immediate-Or-Cancel (IOC) order against the order
 * book.
 *
 * An Immediate-Or-Cancel (IOC) order attempts to fill as much of the specified
 * quantity as possible immediately, at prices equal to or better than the given
 * limit price. Any remaining unfilled portion is automatically canceled — the
 * order does not rest on the book.
 *
 * This function scans the order book starting from the best opposing price
 * level (asks for buy orders, bids for sell orders), and aggregates as many
 * fills as possible without exceeding the order’s limit price or remaining
 * quantity.
 *
 * Fills are recorded in the internal `fills_` vector, and the order’s
 * `filled_quantity_` is updated in-place. The order object is passed as a
 * shared pointer so that state is shared across engine components.
 *
 * @param asset_id Identifier of the asset being traded.
 * @param side Direction of the trade: TradeSide::Buy or TradeSide::Sell.
 * @param order Shared pointer to the IOC order being executed.
 *              The `filled_quantity_` field will be updated accordingly.
 * @return true if the order was partially or fully filled; false if no fills
 * occurred.
 */
bool ExecutionEngine::execute_ioc_order(int asset_id, TradeSide side,
                                        std::shared_ptr<Order> order) {
    int level = 0;
    int levels = (side == TradeSide::Buy) ? orderbooks_[asset_id].ask_levels()
                                          : orderbooks_[asset_id].bid_levels();
    while (level < levels && order->filled_quantity_ < order->quantity_) {
        Price level_price =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].price_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].price_at_level(BookSide::Bid, level);
        if (side == TradeSide::Buy && level_price > order->price_) break;
        if (side == TradeSide::Sell && level_price < order->price_) break;
        Quantity level_depth =
            (side == TradeSide::Buy)
                ? orderbooks_[asset_id].depth_at_level(BookSide::Ask, level)
                : orderbooks_[asset_id].depth_at_level(BookSide::Bid, level);
        if (level_depth > (order->quantity_ - order->filled_quantity_)) {
            Fill fill = {.asset_id_ = asset_id,
                         .exch_timestamp_ = order->exch_timestamp_,
                         .local_timestamp_ =
                             order->exch_timestamp_ + order_response_latency_us_,
                         .orderId_ = order->orderId_,
                         .side_ = side,
                         .price_ = level_price,
                         .quantity_ =
                             order->quantity_ - order->filled_quantity_,
                         .is_maker = false};
            fills_.emplace_back(fill);
            order->filled_quantity_ = order->quantity_;
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});

        } else {
            order->filled_quantity_ += level_depth;
            fills_.emplace_back(
                Fill{.asset_id_ = asset_id,
                     .exch_timestamp_ = order->exch_timestamp_,
                     .local_timestamp_ =
                         order->exch_timestamp_ + order_response_latency_us_,
                     .orderId_ = order->orderId_,
                     .side_ = side,
                     .price_ = level_price,
                     .quantity_ = level_depth,
                     .is_maker = false});
            order_updates_.emplace_back(
                OrderUpdate{.exch_timestamp_ = order->exch_timestamp_,
                            .local_timestamp_ = order->exch_timestamp_ +
                                                order_response_latency_us_,
                            .asset_id_ = asset_id,
                            .orderId_ = order->orderId_,
                            .event_type_ = OrderEventType::Fill,
                            .order_ = *order});
        }
        level++;
    }
    return (order->filled_quantity_ > 0) ? true : false;
}

/**
 * @brief Places a Limit GTX (Post-Only) order without taking liquidity.
 *
 * A GTX (Good-Till-Cancel Post-Only) order is rejected if it would cross the
 * spread and match with any existing resting orders. This ensures the order
 * only adds liquidity to the order book and does not take liquidity.
 *
 * The function first checks whether the given order would cross the opposing
 * side of the book. If so, it is rejected (i.e., not inserted into the book).
 * Otherwise, the order is inserted at its price level and its estimated queue
 * position is recorded based on the current depth at that price.
 *
 * Both bid-side and ask-side GTX orders are supported. For example:
 * - A buy GTX order is rejected if its price is >= best ask.
 * - A sell GTX order is rejected if its price is <= best bid.
 *
 * All accepted orders are stored in the appropriate side's price map and
 * tracked in the global `orders_` map.
 *
 * @param asset_id Identifier of the traded asset.
 * @param order Shared pointer to the order to be placed. `queueEst_` will be
 * initialized.
 * @return true if the order was accepted and added to the book; false if it
 * would take liquidity.
 *
 * @note This function does not attempt to match or execute the order.
 *       It only handles enforcement and insertion of passive orders.
 */
bool ExecutionEngine::place_gtx_order(int asset_id,
                                      std::shared_ptr<Order> order) {
    Price best_ask = orderbooks_[asset_id].best_ask();
    Price best_bid = orderbooks_[asset_id].best_bid();
    if ((order->side_ == BookSide::Bid && best_ask > 0.0 &&
         order->price_ >= best_ask) ||
        (order->side_ == BookSide::Ask && best_bid > 0.0 &&
         order->price_ <= best_bid)) {
        std::cout << "[ExecutionEngine] - gtx order failed\n";
        return false;
    }

    order->queueEst_ =
        orderbooks_[asset_id].depth_at(order->side_, order->price_);
    if (order->side_ == BookSide::Bid)
        gtx_books_[asset_id].bid_orders_[order->price_] = order;
    else
        gtx_books_[asset_id].ask_orders_[order->price_] = order;
    orders_[order->orderId_] = order;
    active_orders_[asset_id].push_back(order);
    order_updates_.emplace_back(OrderUpdate{
        .exch_timestamp_ = order->exch_timestamp_,
        .local_timestamp_ = order->exch_timestamp_ + order_response_latency_us_,
        .asset_id_ = asset_id,
        .orderId_ = order->orderId_,
        .event_type_ = OrderEventType::Acknowledged});
    std::cout << "[ExecutionEngine] - gtx order (" << order->orderId_
              << ") placed, now " << orders_.size() << " orders\n";
    return true;
}

/**
 * @brief Submits a new order to the execution engine.
 *
 * This function routes the submitted order to the appropriate execution path
 * based on its order type (e.g., MARKET, LIMIT) and time-in-force (TIF)
 * directive (e.g., FOK, IOC, GTX). Market orders are executed immediately,
 * while limit orders are processed according to their TIF policy.
 *
 * @param asset_id The unique identifier for the asset this order applies to.
 * @param side The side of the trade (Buy or Sell).
 * @param order The order object containing price, quantity, TIF, and type.
 *
 * @throws std::invalid_argument if the TimeInForce value is unsupported.
 */
bool ExecutionEngine::execute_order(int asset_id, TradeSide side,
                                    const Order &order) {
    auto order_ptr = std::make_shared<Order>(order);
    if (order.orderType_ == OrderType::MARKET) {
        std::cout << "[ExecutionEngine] - execute market order\n";
        execute_market_order(asset_id, side, order_ptr);
    } else if (order.orderType_ == OrderType::LIMIT) {
        switch (order.tif_) {
        case TimeInForce::FOK:
            return execute_fok_order(asset_id, side, order_ptr);
            break;
        case TimeInForce::IOC:
            return execute_ioc_order(asset_id, side, order_ptr);
            break;
        case TimeInForce::GTX:
            std::cout << "[ExecutionEngine] - execute limit gtx order\n";
            return place_gtx_order(asset_id, order_ptr);
            break;
        default:
            throw std::invalid_argument("Unsupported TimeInForce");
            return false;
        }
    }
    return true;
}

/*
 * @brief Handles a book update and updates internal queue position estimates.
 *
 * This function processes a BookUpdate for a given asset by:
 * - Retrieving the current quantity at the updated price level.
 * - Computing the quantity change (delta).
 * - If it's a bid-side reduction and our order exists at that price,
 *   updating our queue position estimate based on a probabilistic fill model.
 * - Applying the update to the order book.
 *
 * The fill probability is estimated using the formula:
 * \f[
 * p_n = \frac{f(V_n)}{f(V_n) + f(\max(Q_n - S - V_n, 0))}
 * \f]
 * where:
 * - \(V_n\) is our previous queue estimate,
 * - \(S\) is the remaining quantity in our order,
 * - \(Q_n\) is the total depth at that price before the update,
 * - \(\delta Q_n\) is the change in depth,
 * - and \(f(x) = \log(1 + x)\).
 *
 * @param asset_id The ID of the asset this update pertains to.
 * @param book_update The update event (side, price, new quantity).
 */
void ExecutionEngine::handle_book_update(int asset_id,
                                         const BookUpdate &book_update) {
    // update queue position estimations
    Quantity Q_n =
        orderbooks_[asset_id].depth_at(book_update.side_, book_update.price_);
    Quantity deltaQ_n = book_update.quantity_ - Q_n;
    if (deltaQ_n < 0) {
        if (book_update.side_ == BookSide::Bid) {
            auto it = gtx_books_[asset_id].bid_orders_.find(book_update.price_);
            if (it != gtx_books_[asset_id].bid_orders_.end()) {
                Quantity S =
                    it->second->quantity_ - it->second->filled_quantity_;
                Quantity V_n = it->second->queueEst_;
                double p_n =
                    (f(V_n) > 0.0)
                        ? (f(V_n) / (f(V_n) + f(std::max(Q_n - S - V_n, 0.0))))
                        : 0.0;
                Quantity V_nplus1 = std::max(V_n + p_n * deltaQ_n, 0.0);
                it->second->queueEst_ = V_nplus1;
            }
        } else if (book_update.side_ == BookSide::Ask) {
            auto it = gtx_books_[asset_id].ask_orders_.find(book_update.price_);
            if (it != gtx_books_[asset_id].ask_orders_.end()) {
                Quantity S =
                    it->second->quantity_ - it->second->filled_quantity_;
                Quantity V_n = it->second->queueEst_;
                double p_n =
                    (f(V_n) > 0.0)
                        ? (f(V_n) / (f(V_n) + f(std::max(Q_n - S - V_n, 0.0))))
                        : 0.0;
                Quantity V_nplus1 = std::max(V_n + p_n * deltaQ_n, 0.0);
                it->second->queueEst_ = V_nplus1;
            }
        }
    }
    // update orderbook
    orderbooks_[asset_id].apply_book_update(book_update);
}

/**
 * @brief Processes an incoming trade and fills a matching resting order if
 * eligible.
 *
 * This function simulates passive order fills by checking if there is a resting
 * order at the trade price on the opposite side of the trade. If a matching
 * order exists at the same price, has queue position zero (i.e., is at the
 * front), and was placed before the trade timestamp, it will be filled.
 *
 * Specifically:
 * - For a Sell trade: attempts to fill the top Buy order at the trade price.
 * - For a Buy trade: attempts to fill the top Sell order at the trade price.
 * - A fill is only allowed if the resting order has `queueEst_ == 0.0`, meaning
 *   it is first in line to be filled.
 * - The filled quantity is the minimum of the trade quantity and the unfilled
 *   portion of the resting order.
 *
 * If all conditions are met, a fill is created and stored in the internal
 * `fills_` list.
 *
 * @param asset_id Identifier for the asset being traded.
 * @param trade Incoming trade information (price, quantity, side, timestamp,
 * etc.).
 */
void ExecutionEngine::handle_trade(int asset_id, const Trade &trade) {
    auto it = (trade.side_ == TradeSide::Sell)
                  ? gtx_books_[asset_id].bid_orders_.find(trade.price_)
                  : gtx_books_[asset_id].ask_orders_.find(trade.price_);
    auto end = (trade.side_ == TradeSide::Sell)
                   ? gtx_books_[asset_id].bid_orders_.end()
                   : gtx_books_[asset_id].ask_orders_.end();
    if (it == end) return;
    auto order = it->second;

    if (order->exch_timestamp_ >= trade.exch_timestamp_) return;
    std::cout << "[ExecutionEngine] - order found at trade price and time\n";

    if (order->queueEst_ == 0.0 && order->filled_quantity_ < order->quantity_) {
        Quantity fill_qty = std::min(
            trade.quantity_, order->quantity_ - order->filled_quantity_);
        std::cout << "[ExecutionEngine] - fill signaled for " << fill_qty
                  << " quantity\n";
        order->filled_quantity_ += fill_qty;
        order_updates_.emplace_back(
            OrderUpdate{.exch_timestamp_ = trade.exch_timestamp_,
                        .local_timestamp_ =
                            trade.exch_timestamp_ + order_response_latency_us_,
                        .asset_id_ = asset_id,
                        .orderId_ = order->orderId_,
                        .event_type_ = OrderEventType::Fill,
                        .order_ = *order});
        fills_.emplace_back(
            Fill{.asset_id_ = asset_id,
                 .exch_timestamp_ = trade.exch_timestamp_,
                 .local_timestamp_ =
                     trade.exch_timestamp_ + order_response_latency_us_,
                 .orderId_ = order->orderId_,
                 .side_ = (order->side_ == BookSide::Bid) ? TradeSide::Buy
                                                          : TradeSide::Sell,
                 .price_ = trade.price_,
                 .quantity_ = fill_qty,
                 .is_maker = true});
        std::cout << "[ExecutionEngine] - " << fills_.size() << " fills\n";
    }
}

/**
 * @brief Returns a read-only reference to the list of order updates.
 *
 * Provides access to all order updates that have occured during
 * execution on the exchange.
 *
 * @return A const reference to the vector of OrderUpdate objects.
 */
const std::vector<OrderUpdate> &ExecutionEngine::order_updates() const {
    return order_updates_;
}

/**
 * @brief Clears all recorded order updates from the execution engine.
 *
 * This method empties the internal 'order_updates_' vector, removing
 * all previously recorded trad updates.
 */
void ExecutionEngine::clear_order_updates() { order_updates_.clear(); }

/**
 * @brief Returns a read-only reference to the list of generated fills.
 *
 * Provides access to all fills that have occurred during order execution.
 *
 * @return A const reference to the vector of Fill objects.
 */
const std::vector<Fill> &ExecutionEngine::fills() const { return fills_; }

/**
 * @brief Clears all recorded fills from the execution engine.
 *
 * This method empties the internal `fills_` vector, removing all previously
 * recorded trade fills. Useful for resetting state between test cases or
 * simulations.
 */
void ExecutionEngine::clear_fills() { fills_.clear(); }

/**
 * @brief Computes the natural logarithm of (1 + quantity).
 *
 * This function uses std::log1p to accurately compute log(1 + qty),
 * which is especially useful for small values of qty to avoid precision loss.
 *
 * @param qty The quantity (x) for which to compute log(1 + x).
 * @return The natural logarithm of (1 + qty).
 */
double ExecutionEngine::f(const double x) { return std::log1p(x); }

/**
 * @brief Sets the order entry latency in microseconds
 * @param latency in microseconds
 */
void ExecutionEngine::set_order_entry_latency_us(
    const Microseconds latency_us) {
    order_entry_latency_us_ = latency_us;
}

/**
 * @brief Sets the order response latency in microseconds
 * @param latency in microseconds
 */
void ExecutionEngine::set_order_response_latency_us(
    const Microseconds latency_us) {
    order_response_latency_us_ = latency_us;
}