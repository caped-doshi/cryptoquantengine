/*
 * File: hftengine/core/data/readers/market_data_feed.cpp
 * Description: Class to stream trade and book_update data chronologically.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../market_data/book_update.h"
#include "../../market_data/trade.h"
#include "../../orderbook/orderbook.h"
#include "../../types/event_type.h"
#include "../../types/usings.h"
#include "book_stream_reader.h"
#include "market_data_feed.h"
#include "trade_stream_reader.h"

/**
 * @brief Constructs a MarketDataFeed for multiple assets from CSV files.
 *
 * This constructor initializes internal stream state for each asset by creating
 * a BookStreamReader and TradeStreamReader, opening the corresponding input
 * files. The asset ID is used as a key to associate each book/trade file pair
 * with a unique asset.
 *
 * @param book_files A map from asset ID to the path of the CSV file containing
 * book updates. Each file is opened and parsed by a BookStreamReader.
 * @param trade_files A map from asset ID to the path of the CSV file containing
 * trade data. Each file is opened and parsed by a TradeStreamReader, if
 * provided.
 *
 * @note If a trade file is not found for a given asset ID, only book updates
 * will be used. Files are expected to be in a compatible CSV format with
 * correct headers.
 */
MarketDataFeed::MarketDataFeed(
    const std::unordered_map<int, std::string> &book_files,
    const std::unordered_map<int, std::string> &trade_files) {
    for (const auto &[asset_id, book_file] : book_files) {
        StreamState state;
        state.book_reader = std::make_unique<BookStreamReader>();
        state.book_reader->open(book_file);

        auto trade_it = trade_files.find(asset_id);
        if (trade_it != trade_files.end()) {
            state.trade_reader = std::make_unique<TradeStreamReader>();
            state.trade_reader->open(trade_it->second);
        }

        asset_streams_[asset_id] = std::move(state);
    }
}

/**
 * @brief Retrieves the next market data event (either book update or trade)
 * across all assets.
 *
 * This function scans all asset streams and identifies the earliest event based
 * on timestamp. The event can be either a `BookUpdate` or a `Trade`, and is
 * selected in global chronological order from all assets being tracked. The
 * corresponding event is returned via reference parameters, and internal stream
 * pointers are advanced accordingly.
 *
 * @param[out] asset_id The asset ID associated with the next event.
 * @param[out] event_type The type of the next event (`EventType::BookUpdate` or
 * `EventType::Trade`).
 * @param[out] book_update The book update event data (valid only if event_type
 * is BookUpdate).
 * @param[out] trade The trade event data (valid only if event_type is Trade).
 * @return true if a new event was found and returned, false if all streams are
 * exhausted.
 */
bool MarketDataFeed::next_event(int &asset_id, EventType &event_type,
                                BookUpdate &book_update, Trade &trade) {
    bool found = false;
    Timestamp min_time = std::numeric_limits<Timestamp>::max();

    for (auto &[id, stream] : asset_streams_) {
        // Ensure both streams are preloaded
        if (!stream.next_book_update.has_value()) stream.advance_book();
        if (!stream.next_trade.has_value()) stream.advance_trade();

        if (stream.next_book_update.has_value() &&
            stream.next_book_update->timestamp_ < min_time) {
            min_time = stream.next_book_update->timestamp_;
            asset_id = id;
            event_type = EventType::BookUpdate;
            found = true;
        }

        if (stream.next_trade.has_value() &&
            stream.next_trade->timestamp_ < min_time) {
            min_time = stream.next_trade->timestamp_;
            asset_id = id;
            event_type = EventType::Trade;
            found = true;
        }
    }

    if (!found) return false;

    auto &stream = asset_streams_.at(asset_id);
    if (event_type == EventType::BookUpdate) {
        book_update = *stream.next_book_update;
        stream.advance_book();
    } else {
        trade = *stream.next_trade;
        stream.advance_trade();
    }

    return true;
}

/**
 * @brief Retrieves the earliest upcoming timestamp across all market data
 * streams.
 *
 * This function examines the next available `BookUpdate` and `Trade` events for
 * all assets and returns the minimum timestamp among them, without advancing
 * the internal stream state. It can be used to peek at the timestamp of the
 * next event that would be returned by `next_event()`.
 *
 * @return The earliest upcoming event timestamp across all assets, or
 * `std::numeric_limits<Timestamp>::max()` if no events remain.
 */
std::optional<Timestamp> MarketDataFeed::peek_timestamp() {
    std::optional<Timestamp> earliest;

    for (auto &[asset_id, stream] : asset_streams_) {
        if (!stream.next_book_update.has_value()) stream.advance_book();
        if (!stream.next_trade.has_value()) stream.advance_trade();

        if (stream.next_book_update.has_value()) {
            Timestamp ts = stream.next_book_update->timestamp_;
            if (!earliest.has_value() || ts < *earliest) {
                earliest = ts;
            }
        }

        if (stream.next_trade.has_value()) {
            Timestamp ts = stream.next_trade->timestamp_;
            if (!earliest.has_value() || ts < *earliest) {
                earliest = ts;
            }
        }
    }

    return earliest;
}

/**
 * @brief Advances the book update stream to the next available update.
 *
 * Attempts to parse the next `BookUpdate` from the associated book reader.
 * If successful, the parsed update is stored in `next_book_update`.
 * If the stream has ended or parsing fails, `next_book_update` is reset to
 * `std::nullopt`.
 *
 * @return true if a new book update was successfully parsed and stored, false
 * otherwise.
 */
bool MarketDataFeed::StreamState::advance_book() {
    BookUpdate update;
    if (book_reader->parse_next(update)) {
        next_book_update = update;
        return true;
    }
    next_book_update.reset();
    return false;
}

/**
 * @brief Advances the trade stream to the next available trade.
 *
 * Attempts to parse the next `Trade` from the associated trade reader.
 * If successful, the parsed trade is stored in `next_trade`.
 * If the stream has ended or parsing fails, `next_trade` is reset to
 * `std::nullopt`.
 *
 * @return true if a new trade was successfully parsed and stored, false
 * otherwise.
 */
bool MarketDataFeed::StreamState::advance_trade() {
    Trade t;
    if (trade_reader->parse_next(t)) {
        next_trade = t;
        return true;
    }
    next_trade.reset();
    return false;
}