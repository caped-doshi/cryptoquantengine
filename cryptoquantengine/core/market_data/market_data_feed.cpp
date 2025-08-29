/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <future>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../market_data/book_update.h"
#include "../market_data/trade.h"
#include "../orderbook/orderbook.h"
#include "../types/aliases/usings.h"
#include "../types/enums/event_type.h"
#include "market_data_feed.h"
#include "readers/book_stream_reader.h"
#include "readers/trade_stream_reader.h"

namespace core::market_data {
MarketDataFeed::MarketDataFeed() {}

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
    using namespace core::market_data;

    for (const auto &[asset_id, book_file] : book_files) {
        auto trade_it = trade_files.find(asset_id);
        std::string trade_file =
            (trade_it != trade_files.end()) ? trade_it->second : "";
        add_stream(asset_id, book_file, trade_file);
    }
}

/**
 * @brief Adds a new asset data stream to the MarketDataFeed.
 *
 * This method initializes and registers a new stream for the given asset ID
 * using the provided Level 2 book update file and trade file. The function
 * creates new `BookStreamReader` and `TradeStreamReader` instances, opens the
 * corresponding CSV files, and stores the stream state internally for later
 * event processing.
 *
 * @param asset_id The unique identifier of the asset.
 * @param book_file Path to the CSV file containing Level 2 book update data.
 * @param trade_file Path to the CSV file containing trade data.
 *
 * @note This method assumes that the given file paths are valid and readable.
 *       It will replace any existing stream associated with the same asset ID.
 */
void MarketDataFeed::add_stream(int asset_id, const std::string &book_file,
                                const std::string &trade_file) {
    using namespace core::market_data;
    StreamState stream;

    auto book_future = std::async(std::launch::async, [book_file]() {
        auto reader = std::make_unique<BookStreamReader>();
        reader->open(book_file);
        return reader;
    });
    auto trade_future = std::async(std::launch::async, [trade_file]() {
        auto reader = std::make_unique<TradeStreamReader>();
        reader->open(trade_file);
        return reader;
    });
    stream.book_reader = book_future.get();
    stream.trade_reader = trade_future.get();
    stream.book_reader->set_market_feed_latency_us(market_feed_latency_us_);
    stream.trade_reader->set_market_feed_latency_us(market_feed_latency_us_);
    asset_streams_[asset_id] = std::move(stream);
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
                                core::market_data::BookUpdate &book_update,
                                core::market_data::Trade &trade) {
    using namespace core::market_data;
    bool found = false;
    Timestamp min_time = std::numeric_limits<Timestamp>::max();

    for (auto &[id, stream] : asset_streams_) {
        // Ensure both streams are preloaded
        if (!stream.next_book_update.has_value()) stream.advance_book();
        if (!stream.next_trade.has_value()) stream.advance_trade();

        if (stream.next_book_update.has_value() &&
            stream.next_book_update->exch_timestamp_ < min_time) {
            min_time = stream.next_book_update->exch_timestamp_;
            asset_id = id;
            event_type = EventType::BookUpdate;
            found = true;
        }

        if (stream.next_trade.has_value() &&
            stream.next_trade->exch_timestamp_ < min_time) {
            min_time = stream.next_trade->exch_timestamp_;
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
    using namespace core::market_data;
    std::optional<Timestamp> earliest;

    for (auto &[asset_id, stream] : asset_streams_) {
        if (!stream.next_book_update.has_value()) stream.advance_book();
        if (!stream.next_trade.has_value()) stream.advance_trade();

        if (stream.next_book_update.has_value()) {
            Timestamp ts = stream.next_book_update->exch_timestamp_;
            if (!earliest.has_value() || ts < *earliest) {
                earliest = ts;
            }
        }

        if (stream.next_trade.has_value()) {
            Timestamp ts = stream.next_trade->exch_timestamp_;
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
    using namespace core::market_data;
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
    using namespace core::market_data;
    Trade trade;
    if (trade_reader->parse_next(trade)) {
        next_trade = trade;
        return true;
    }
    next_trade.reset();
    return false;
}

void MarketDataFeed::set_market_feed_latency(Microseconds latency_us) {
    market_feed_latency_us_ = latency_us;
    for (auto &[_, stream] : asset_streams_) {
        stream.book_reader->set_market_feed_latency_us(latency_us);
        stream.trade_reader->set_market_feed_latency_us(latency_us);
    }
}
} // namespace core::market_data