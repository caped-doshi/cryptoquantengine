/*
 * Copyright (c) 2025 arvindkrv@protonmail.com
 *
 * Please see the LICENSE file for the terms and conditions
 * associated with this software.
 */

#include <atomic>
#include <thread>

#include "../types/aliases/usings.h"

namespace core::trading {
class OrderIdGenerator {
  public:
    OrderIdGenerator() : current_id_(0) {}

    OrderId nextId() { return ++current_id_; }

  private:
    std::atomic<OrderId> current_id_;
};
}