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

#include "rx_stream.hpp"

#include <thread>
#include <functional>

#include "VideoMasterAPIHelper/handle_manager.hpp"

using Deltacast::Helper::ApiSuccess;

const std::unordered_map<uint32_t, VHD_STREAMTYPE> id_to_stream_type = {
    {0, VHD_ST_RX0},
    {1, VHD_ST_RX1},
    {2, VHD_ST_RX2},
    {3, VHD_ST_RX3},
    {4, VHD_ST_RX4},
    {5, VHD_ST_RX5},
    {6, VHD_ST_RX6},
    {7, VHD_ST_RX7},
    {8, VHD_ST_RX8},
    {9, VHD_ST_RX9},
    {10, VHD_ST_RX10},
    {11, VHD_ST_RX11},
};

std::unique_ptr<Deltacast::RxStream> Deltacast::RxStream::create(Device& device, int channel_index, BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct)
{
    if (id_to_stream_type.find(channel_index) == id_to_stream_type.end())
        return nullptr;
        
    auto stream_handle = get_stream_handle(device.handle(), id_to_stream_type.at(channel_index), VHD_SDI_STPROC_DISJOINED_VIDEO);
    if (!stream_handle)
        return nullptr;
    
    return std::unique_ptr<RxStream>(new RxStream(device, channel_index, std::move(stream_handle), buffer_allocation_fct, buffer_deallocation_fct));
}

bool Deltacast::RxStream::configure(SignalInformation signal_info, bool /*overlay_enabled*/)
{
    ApiSuccess api_success;
    if (!(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_TRANSFER_SCHEME, VHD_TRANSFER_UNCONSTRAINED))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_VIDEO_STANDARD, signal_info.video_standard))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_INTERFACE, signal_info.interface))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFER_PACKING, VHD_BUFPACK_VIDEO_RGB_24))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_DEPTH, _buffer_queue_depth)))
    {
        std::cout << "ERROR for " << _name << ": Cannot configure stream (" << api_success << ")" << std::endl;
        return false;
    }
    
    return true;
}

bool Deltacast::RxStream::on_start()
{
    bool all_slots_pushed = true;
    for (auto& slot : slots())
        all_slots_pushed = all_slots_pushed && push_slot(slot);
    
    return all_slots_pushed;
}

bool Deltacast::RxStream::loop_iteration(SharedResources& shared_resources)
{
    HANDLE slot_handle = nullptr;
    
    ULONG filling = 0;
    do
    {
        push_slot(slot_handle);

        auto [ _handle, api_success ] = pop_slot();
        if (!api_success && api_success.error_code() == VHDERR_TIMEOUT)
            return true;
        else if (!api_success)
            return false;
        slot_handle = _handle;

        VHD_GetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_FILLING, &filling);
    } while (filling > 0);

    // Wraps the slot handle so that it is pushed back to the queue when it goes out of scope
    // regardless of the way the function returns (normal return, exception, etc.)
    Helper::ResourceManager<HANDLE> slot_wrapper(slot_handle,
        [this, &shared_resources](HANDLE slot_handle)
        {
            push_slot(slot_handle);
        }
    );

    auto signal_info = _device.get_incoming_signal_information(_channel_index);
    if (signal_info.video_standard != shared_resources.signal_info.video_standard
        || signal_info.clock_divisor != shared_resources.signal_info.clock_divisor
        || signal_info.interface != shared_resources.signal_info.interface)
    {
        shared_resources.synchronization.signal_has_changed = true;
        return false;
    }

    VHD_GetSlotBuffer(slot_handle, VHD_SDI_BT_VIDEO, &shared_resources.buffer, &shared_resources.buffer_size);
    
    shared_resources.synchronization.notify_ready_to_process();
    
    while (!_should_stop 
        && !shared_resources.synchronization.wait_until_processed()) {}

    // slot_wrapper goes out of scope "normally" which pushes it back to the queue
    // thanks to the lambda function passed to the ResourceManager

    return true;
}