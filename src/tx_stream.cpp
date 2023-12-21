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

#include "tx_stream.hpp"

#include <thread>

#include "api_helper/handle_manager.hpp"

using Deltacast::Helper::ApiSuccess;

const std::unordered_map<uint32_t, VHD_STREAMTYPE> id_to_stream_type = {
    {0, VHD_ST_TX0},
    {1, VHD_ST_TX1},
    {2, VHD_ST_TX2},
    {3, VHD_ST_TX3},
    {4, VHD_ST_TX4},
    {5, VHD_ST_TX5},
    {6, VHD_ST_TX6},
    {7, VHD_ST_TX7},
    {8, VHD_ST_TX8},
    {9, VHD_ST_TX9},
    {10, VHD_ST_TX10},
    {11, VHD_ST_TX11},
};

std::unique_ptr<Deltacast::TxStream> Deltacast::TxStream::create(Device& device, int channel_index
                                                                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct)
{
    if (id_to_stream_type.find(channel_index) == id_to_stream_type.end())
        return nullptr;

    auto stream_handle = get_stream_handle(device.handle(), id_to_stream_type.at(channel_index), VHD_SDI_STPROC_DISJOINED_VIDEO);
    if (!stream_handle)
        return nullptr;
    
    return std::unique_ptr<TxStream>(new Deltacast::TxStream(device, channel_index, std::move(stream_handle), buffer_allocation_fct, buffer_deallocation_fct));
}

bool Deltacast::TxStream::configure(SignalInformation signal_info, bool overlay_enabled)
{
    ApiSuccess api_success;
    if (!(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_VIDEO_STANDARD, signal_info.video_standard))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_INTERFACE, signal_info.interface))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_TX_GENLOCK, TRUE))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFER_PACKING, (overlay_enabled ? VHD_BUFPACK_VIDEO_RGBA_32 : VHD_BUFPACK_VIDEO_RGB_24)))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_DEPTH, _buffer_queue_depth))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_PRELOAD, 0)))
    {
        std::cout << "ERROR for " << _name << ": Cannot configure stream (" << api_success << ")" << std::endl;
        return false;
    }

    return true;
}

bool Deltacast::TxStream::on_start(SharedResources& shared_resources)
{
    for (auto i = 0; i < _buffer_queue_depth; ++i)
        shared_resources.synchronization.push_buffer_for_processing(std::get<HANDLE>(pop_slot()));

    return true;
}

bool Deltacast::TxStream::loop_iteration(SharedResources& shared_resources)
{
    while (!_should_stop && !shared_resources.synchronization.wait_until_ready_to_process()) {}
    if (_should_stop)
        return false;

    HANDLE slot_handle = shared_resources.synchronization.get_buffer_to_transfer();
    if (!slot_handle)
    {
        std::cout << "ERROR for " << _name << ": No slot available to transfer" << std::endl;
        return false;
    }

    ApiSuccess api_success;
    ULONG on_board_filling = 0, buffer_queue_filling = 0;
    if (!(api_success = VHD_GetStreamProperty(*handle(), VHD_CORE_SP_ONBOARDBUFFER_FILLING, &on_board_filling))
        || !(api_success = VHD_GetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_FILLING, &buffer_queue_filling)))
    {
        std::cout << "ERROR for " << _name << ": Cannot get stream property (" << api_success << ")" << std::endl;
        return false;
    }

    if ((on_board_filling + buffer_queue_filling) < shared_resources.maximum_latency)
    {
        push_slot(slot_handle);

        {
            auto [ handle, api_success ] = pop_slot();
            if (!api_success)
            {
                std::cout << "ERROR for " << _name << ": Cannot pop slot (" << api_success << ")" << std::endl;
                return false;
            }
            slot_handle = handle;
        }
    }

    shared_resources.synchronization.push_buffer_for_processing(slot_handle);

    return true;
}
