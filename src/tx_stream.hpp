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

#include <thread>

#include "device.hpp"
#include "stream.hpp"
#include "shared_resources.hpp"

namespace Deltacast
{
    class TxStream : public Stream
    {
    private:
        TxStream() = delete;
        TxStream(const TxStream&) = delete;
        TxStream& operator=(const TxStream&) = delete;

        TxStream(Device& device, int channel_index, std::unique_ptr<Helper::StreamHandle> stream_handle
                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct)
            : Stream(device, std::string("TX") + std::to_string(channel_index), channel_index, std::move(stream_handle)
                    , buffer_allocation_fct, buffer_deallocation_fct, VHD_WaitSlotSent, VHD_QueueOutSlot)
        {
        }

    public:
        static std::unique_ptr<TxStream> create(Device& device, int channel_index
                                                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct);

        bool configure(SignalInformation signal_info, bool overlay_enabled) override;

    private:
        bool on_start(SharedResources& shared_resources) override;

        bool loop_iteration(SharedResources& shared_resources) override;
    };
}