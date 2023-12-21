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
    bool ready_to_process = condition_variable.wait_for(lock, 100ms, [&]{ return to_process; });
    if (ready_to_process)
        to_process = false;
    return ready_to_process;
}

void Deltacast::SharedResources::Synchronization::notify_ready_to_process()
{
    {
        std::lock_guard lock(mutex);
        to_process = true;
    }
    condition_variable.notify_one();
}

HANDLE Deltacast::SharedResources::Synchronization::pop_buffer_for_processing()
{
    std::lock_guard lock(mutex);
    if (buffers_ready_for_processing.empty())
    return nullptr;

    auto slot_handle = buffers_ready_for_processing.front();
    buffers_ready_for_processing.pop_front();

    return slot_handle;
}

void Deltacast::SharedResources::Synchronization::push_buffer_for_processing(HANDLE buffer)
{
    std::lock_guard lock(mutex);
    buffers_ready_for_processing.push_back(buffer);
}

HANDLE Deltacast::SharedResources::Synchronization::get_buffer_to_transfer()
{
    std::lock_guard lock(mutex);

    if (buffers_ready_for_transfer.empty())
        return nullptr;

    auto slot_handle = buffers_ready_for_transfer.front();
    buffers_ready_for_transfer.pop_front();

    return slot_handle;
}

void Deltacast::SharedResources::Synchronization::set_buffer_to_transfer(HANDLE buffer)
{
    std::lock_guard lock(mutex);
    buffers_ready_for_transfer.clear();
    buffers_ready_for_transfer.push_back(buffer);
}

void Deltacast::SharedResources::reset()
{
    synchronization.stop_is_requested = false;
    synchronization.signal_has_changed = false;
    signal_info = {};

    synchronization.reset();
}

void Deltacast::SharedResources::Synchronization::reset()
{
    buffers_ready_for_processing.clear();
    buffers_ready_for_transfer.clear();
}
