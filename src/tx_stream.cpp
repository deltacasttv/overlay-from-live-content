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
                                            , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct
                                            , Processor process_fct)
{
    if (id_to_stream_type.find(channel_index) == id_to_stream_type.end())
        return nullptr;

    auto stream_handle = get_stream_handle(device.handle(), id_to_stream_type.at(channel_index), VHD_SDI_STPROC_DISJOINED_VIDEO);
    if (!stream_handle)
        return nullptr;
    
    return std::unique_ptr<TxStream>(new Deltacast::TxStream(device, channel_index, std::move(stream_handle), buffer_allocation_fct, buffer_deallocation_fct, process_fct));
}

bool Deltacast::TxStream::configure(SignalInformation signal_info, bool overlay_enabled)
{
    ApiSuccess api_success;
    if (!(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_VIDEO_STANDARD, signal_info.video_standard))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_INTERFACE, signal_info.interface))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_SDI_SP_TX_GENLOCK, TRUE))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFER_PACKING, (overlay_enabled ? VHD_BUFPACK_VIDEO_RGBA_32 : VHD_BUFPACK_VIDEO_RGB_24)))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_DEPTH, 2))
        || !(api_success = VHD_SetStreamProperty(*handle(), VHD_CORE_SP_BUFFERQUEUE_PRELOAD, 0)))
    {
        std::cout << "ERROR for " << _name << ": Cannot configure stream (" << api_success << ")" << std::endl;
        return false;
    }

    return true;
}

bool Deltacast::TxStream::loop_iteration(SharedResources& shared_resources)
{
    auto [ slot_handle, api_success ] = pop_slot();
    if (!api_success && api_success.error_code() == VHDERR_TIMEOUT)
        return true;
    else if (!api_success)
        return false;

    // Wraps the slot handle so that it is pushed back to the queue when it goes out of scope
    // regardless of the way the function returns (normal return, exception, etc.)
    Helper::ResourceManager<HANDLE> slot_wrapper(slot_handle,
        [this, &shared_resources](HANDLE slot_handle)
        {
            shared_resources.synchronization.notify_processing_finished();
            push_slot(slot_handle);
        }
    );

    while (!_should_stop && !shared_resources.synchronization.wait_until_ready_to_process()) {}
    if (_should_stop)
        return false;

    ULONG on_board_filling = 0;
    if (!(api_success = VHD_GetStreamProperty(*handle(), VHD_CORE_SP_ONBOARDBUFFER_FILLING, &on_board_filling)))
    {
        std::cout << "ERROR for " << _name << ": Cannot get stream property (" << api_success << ")" << std::endl;
        return false;
    }

    for (auto i = 1; i < on_board_filling; ++i)
    {
        shared_resources.synchronization.notify_processing_finished();
        while (!_should_stop && !shared_resources.synchronization.wait_until_ready_to_process()) {}
    }

    UBYTE* buffer = nullptr;
    ULONG buffer_size = 0;
    if (!(api_success = VHD_GetSlotBuffer(slot_handle, VHD_SDI_BT_VIDEO, &buffer, &buffer_size)))
    {
        std::cout << "ERROR for " << _name << ": Cannot get slot buffer (" << api_success << ")" << std::endl;
        return false;
    }

    _process_fct(shared_resources.buffer, shared_resources.buffer_size, buffer, buffer_size);

    // slot_wrapper goes out of scope "normally" which notifies that the slot has been processed and pushes it back to the queue
    // thanks to the lambda function passed to the ResourceManager

    return true;
}
