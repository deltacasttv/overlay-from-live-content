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

#include "stream.hpp"

#include <thread>

using Deltacast::Helper::ApiSuccess;

Deltacast::Stream::~Stream()
{
    stop();
}

bool Deltacast::Stream::start(SharedResources& shared_resources)
{
    if (!configure_application_buffers())
        return false;
    
    if (!on_start(shared_resources))
        return false;

    ApiSuccess api_success{VHD_StartStream(*handle())};
    if (!api_success)
    {
        std::cout << "ERROR for " << _name << ": Cannot start stream (" << api_success << ")" << std::endl;
        return false;
    }

    _loop_thread = std::thread(&Stream::loop, this, std::ref(shared_resources));
    
    return true;
}

bool Deltacast::Stream::stop()
{
    if (_should_stop)
        return true;

    _should_stop = true;
    if (_loop_thread.joinable())
        _loop_thread.join();

    on_stop();

    VHD_StopStream(*handle());

    if (!uninit_application_buffers())
        return false;

    return true;
}

std::pair<HANDLE, ApiSuccess> Deltacast::Stream::pop_slot()
{
    if (_number_of_pushed_slots < _buffer_queue_depth)
        return std::make_pair(_slots[_number_of_pushed_slots++], ApiSuccess{});
    
    HANDLE slot_handle = nullptr;
    ApiSuccess api_success{_api_pop_slot_fct(*handle(), &slot_handle, _slot_timeout_ms)};
    if (!api_success && api_success.error_code() != VHDERR_TIMEOUT)
    {
        std::cout << "ERROR for " << _name << ": Cannot pop slot (" << api_success << ")" << std::endl;
        return std::make_pair(nullptr, api_success);
    }

    return std::make_pair(slot_handle, api_success);
}

ApiSuccess Deltacast::Stream::push_slot(HANDLE slot_handle)
{
    if (!slot_handle)
        return ApiSuccess{};

    ApiSuccess api_success{_api_push_slot_fct(slot_handle)};
    if (!api_success)
    {
        std::cout << "ERROR for " << _name << ": Cannot push slot (" << api_success << ")" << std::endl;
        return api_success;
    }

    ++_number_of_pushed_slots;

    return ApiSuccess{};
}

bool Deltacast::Stream::loop(SharedResources& shared_resources)
{
    while (!_should_stop)
    {
        if (!loop_iteration(shared_resources))
            return false;

        if (!_should_stop)
        {
            ULONG slots_count = 0, slots_dropped = 0;
            VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_COUNT, &slots_count);
            VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_DROPPED, &slots_dropped);

            if (!_slots_dropped.has_value())
                _slots_dropped = slots_dropped;

            if (_slots_dropped != slots_dropped)
            {
                std::cout << "INFO for " << _name << ": Dropped occurred: slots count=" << slots_count << ", slots dropped=" << slots_dropped << std::endl;
                _slots_dropped = slots_dropped;
            }
        }
    }
    
    return true;
}

bool Deltacast::Stream::configure_application_buffers()
{
    ApiSuccess api_success{VHD_InitApplicationBuffers(*handle())};
    if (!api_success)
    {
        std::cout << "ERROR for " << _name << ": Cannot initialize application buffers (" << api_success << ")" << std::endl;
        return false;
    }

    ULONG buffer_size = 0;
    api_success = VHD_GetApplicationBuffersSize(*handle(), VHD_SDI_BT_VIDEO, &buffer_size);
    if (!api_success || !buffer_size)
    {
        std::cout << "ERROR for " << _name << ": Cannot get application buffers size (" << api_success << ")" << std::endl;
        return false;
    }

    for (auto i = 0; i < _buffer_queue_depth; ++i)
    {
        _application_buffers[i][VHD_SDI_BT_VIDEO] = _buffer_allocation_fct(buffer_size);
        api_success = VHD_CreateSlotEx(*handle(), _application_buffers[i].data(), &_slots[i]);
        if (!api_success)
        {
            std::cout << "ERROR for " << _name << ": Cannot create slot (" << api_success << ")" << std::endl;
            return false;
        }
    }

    return true;
}

bool Deltacast::Stream::uninit_application_buffers()
{
    for (auto i = 0; i < _buffer_queue_depth; ++i)
        _buffer_deallocation_fct(_application_buffers[i][VHD_SDI_BT_VIDEO]);
    
    return true;
}