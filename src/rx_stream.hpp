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

#include "device.hpp"
#include "stream.hpp"
#include "shared_resources.hpp"

namespace Deltacast
{
    class RxStream : public Stream
    {
    public:
        using Processor = std::function<void(const uint8_t* input_buffer, uint32_t input_buffer_size, uint8_t* output_buffer, uint32_t output_buffer_size)>;

    private:
        RxStream() = delete;
        RxStream(const RxStream&) = delete;
        RxStream& operator=(const RxStream&) = delete;

        RxStream(Device& device, int channel_index, std::unique_ptr<Helper::StreamHandle> stream_handle
                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct
                , Processor process_fct)
            : Stream(device, std::string("RX") + std::to_string(channel_index), channel_index, std::move(stream_handle)
                    , buffer_allocation_fct, buffer_deallocation_fct, VHD_WaitSlotFilled, VHD_QueueInSlot)
            , _process_fct(process_fct)
        {
        }

    public:
        static std::unique_ptr<RxStream> create(Device& device, int channel_index
                                                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct
                                                , Processor process_fct);

        bool configure(SignalInformation signal_info, bool overlay_enabled) override;

    private:
        Processor _process_fct;

        bool on_start(SharedResources& shared_resources) override;
        void on_stop() override;

        bool loop_iteration(SharedResources& shared_resources) override;

        std::atomic_uint64_t _currently_active_processing_threads;
    };
}