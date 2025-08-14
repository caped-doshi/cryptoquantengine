/*
 * File: hftengine/core/trading/orderId_generator.h
 * Description: Class to generate unique orderIds.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-26
 * License: Proprietary
 */

#include <atomic>
#include <thread>

#include "../types/usings.h"

class OrderIdGenerator {
  public:
    OrderIdGenerator() : current_id_(0) {}

    OrderId nextId() { return ++current_id_; }

  private:
    std::atomic<OrderId> current_id_;
};