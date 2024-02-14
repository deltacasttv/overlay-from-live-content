/*
 * SPDX-FileCopyrightText: Copyright (c) DELTACAST.TV. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at * * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <condition_variable>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <atomic>

#include "VideoMasterHD_Core.h"
#include "VideoMasterAPIHelper/VideoInformation/core.hpp"

#include "signal_information.hpp"

namespace Deltacast
{
    struct SharedResources
    {
        class Synchronization
        {
        public:
            std::atomic_bool stop_is_requested = false;
            std::atomic_bool signal_has_changed = false;

            bool wait_until_ready_to_process();
            void notify_processing_finished();
            bool wait_until_processed();
            void notify_ready_to_process();

            std::unique_lock<std::mutex> lock();

        private:
            std::mutex mutex;
            std::condition_variable condition_variable;

            bool to_process = false;
            bool processed = false;
        } synchronization;

        UBYTE* buffer = nullptr;
        ULONG buffer_size = 0;

        std::unique_ptr<VideoMasterVideoInformation> rx_video_info;
        std::unique_ptr<VideoMasterVideoInformation> tx_video_info;

        unsigned int maximum_latency;

        void reset();
    };
}