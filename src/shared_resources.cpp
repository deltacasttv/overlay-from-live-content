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

#include "shared_resources.hpp"

bool Deltacast::SharedResources::Synchronization::wait_until_ready_to_process()
{
    using namespace std::chrono_literals;
    std::unique_lock lock(mutex);
    return condition_variable.wait_for(lock, 100ms, [&]{ return to_process; });
}

void Deltacast::SharedResources::Synchronization::notify_processing_finished()
{
    {
        std::lock_guard lock(mutex);
        processed = true;
        to_process = false;
    }
    condition_variable.notify_one();
}

bool Deltacast::SharedResources::Synchronization::wait_until_processed()
{
    using namespace std::chrono_literals;
    std::unique_lock lock(mutex);
    return condition_variable.wait_for(lock, 100ms, [&]{ return processed; });
}

void Deltacast::SharedResources::Synchronization::notify_ready_to_process()
{
    {
        std::lock_guard lock(mutex);
        processed = false;
        to_process = true;
    }
    condition_variable.notify_one();
}

std::unique_lock<std::mutex> Deltacast::SharedResources::Synchronization::lock()
{
    return std::unique_lock(mutex);
}

void Deltacast::SharedResources::reset()
{
    synchronization.stop_is_requested = false;
    synchronization.incoming_signal_changed = false;
    buffer = nullptr;
    buffer_size = 0;
}
