/*
 * File: hftengine/core/types/queue_model.h
 * Description: Enum class defining the different queue models.
 * Author: Arvind Rathnashyam
 * Date: 2025-06-29
 * License: Proprietary
 */

#pragma once

enum class QueueModel {
    RiskAdverse,
    Prob,
    PowerProb,
    PowerProb2,
    PowerProb3,
    LogProb,
    LobProb2
};