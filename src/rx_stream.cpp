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

#include "api_helper/handle_manager.hpp"

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

std::unique_ptr<Deltacast::RxStream> Deltacast::RxStream::create(Device& device, int channel_index
                                                                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct
                                                                , Processor process_fct)
{
    if (id_to_stream_type.find(channel_index) == id_to_stream_type.end())
        return nullptr;
        
    auto stream_handle = get_stream_handle(device.handle(), id_to_stream_type.at(channel_index), VHD_SDI_STPROC_DISJOINED_VIDEO);
    if (!stream_handle)
        return nullptr;
    
    return std::unique_ptr<RxStream>(new RxStream(device, channel_index, std::move(stream_handle), buffer_allocation_fct, buffer_deallocation_fct, process_fct));
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

bool Deltacast::RxStream::on_start(SharedResources& /*shared_resources*/)
{
    _currently_active_processing_threads = 0;

    bool all_slots_pushed = true;
    for (auto& slot : slots())
        all_slots_pushed = all_slots_pushed && push_slot(slot);
    
    return all_slots_pushed;
}

void Deltacast::RxStream::on_stop()
{
    while (_currently_active_processing_threads.load() > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
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

    auto signal_info = _device.get_incoming_signal_information(_channel_index);
    if (signal_info.video_standard != shared_resources.signal_info.video_standard
        || signal_info.clock_divisor != shared_resources.signal_info.clock_divisor
        || signal_info.interface != shared_resources.signal_info.interface)
    {
        shared_resources.synchronization.signal_has_changed = true;
        push_slot(slot_handle);
        return false;
    }

    UBYTE* buffer = nullptr;
    ULONG buffer_size = 0;
    VHD_GetSlotBuffer(slot_handle, VHD_SDI_BT_VIDEO, &buffer, &buffer_size);

    HANDLE tx_slot_handle = shared_resources.synchronization.pop_buffer_for_processing();
    if (!tx_slot_handle)
    {
        std::cout << "ERROR for " << _name << ": No slot available for processing" << std::endl;
        return false;
    }

    UBYTE* tx_buffer = nullptr;
    ULONG tx_buffer_size = 0;
    VHD_GetSlotBuffer(tx_slot_handle, VHD_SDI_BT_VIDEO, &tx_buffer, &tx_buffer_size);

    _currently_active_processing_threads.fetch_add(1);
    std::thread([=, &shared_resources]()
    {
        if (shared_resources.rx_renderer.has_value())
            shared_resources.rx_renderer.value().update_buffer(buffer, buffer_size);

        _process_fct(buffer, buffer_size, tx_buffer, tx_buffer_size);
        shared_resources.synchronization.set_buffer_to_transfer(tx_slot_handle);
        shared_resources.synchronization.notify_ready_to_process();
        push_slot(slot_handle);
        _currently_active_processing_threads.fetch_sub(1);
    }).detach();
    
    return true;
}