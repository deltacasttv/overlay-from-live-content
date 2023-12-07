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

#include <array>
#include <optional>
#include <functional>
#include <thread>

#include "api_helper/handle_manager.hpp"
#include "device.hpp"
#include "shared_resources.hpp"

#include "VideoMasterHD_ApplicationBuffers.h"

namespace Deltacast
{
    class Stream
    {
    public:
        using BufferAllocate = std::function<VHD_APPLICATION_BUFFER_DESCRIPTOR(ULONG buffer_size)>;
        using BufferDeallocate = std::function<void(VHD_APPLICATION_BUFFER_DESCRIPTOR buffer_descriptor)>;

        virtual ~Stream();

        virtual bool configure(SignalInformation signal_info, bool overlay_enabled) = 0;
        
        bool start(SharedResources& shared_resources);
        bool stop();

    protected:
        Device& _device;
        int _channel_index;
        std::string _name;

        static const uint32_t _buffer_queue_depth = 2;
        static const uint32_t _slot_timeout_ms = 100;

        using ApiPopSlot = std::function<ULONG(HANDLE stream_handle, HANDLE* slot_handle, ULONG timeout_ms)>;
        using ApiPushSlot = std::function<ULONG(HANDLE slot_handle)>;
        
        bool _should_stop = false;

        Stream(Device& device
                , std::string name, int channel_index, std::unique_ptr<Helper::StreamHandle> stream_handle
                , BufferAllocate buffer_allocation_fct, BufferDeallocate buffer_deallocation_fct
                , ApiPopSlot api_pop_slot_fct, ApiPushSlot api_push_slot_fct)
            : _device(device)
            , _name(name)
            , _channel_index(channel_index)
            , _stream_handle(std::move(stream_handle))
            , _buffer_allocation_fct(buffer_allocation_fct)
            , _buffer_deallocation_fct(buffer_deallocation_fct)
            , _api_pop_slot_fct(api_pop_slot_fct)
            , _api_push_slot_fct(api_push_slot_fct)
        {
        }

        Helper::StreamHandle& handle() { return *_stream_handle; }
        
        virtual bool loop(SharedResources& shared_resources);
        virtual bool loop_iteration(SharedResources& shared_resources) = 0;

        bool configure_application_buffers();
        bool uninit_application_buffers();
        virtual bool on_start() { return true; };

        const std::array<HANDLE, _buffer_queue_depth>& slots() const { return _slots; };
        std::pair<HANDLE, Helper::ApiSuccess> pop_slot();
        Helper::ApiSuccess push_slot(HANDLE slot_handle);

    private:
        std::unique_ptr<Helper::StreamHandle> _stream_handle;

        std::function<VHD_APPLICATION_BUFFER_DESCRIPTOR(ULONG buffer_size)> _buffer_allocation_fct;
        std::function<void(VHD_APPLICATION_BUFFER_DESCRIPTOR buffer_descriptor)> _buffer_deallocation_fct;

        std::thread _loop_thread;

        using SlotBufferDescriptors = std::array<VHD_APPLICATION_BUFFER_DESCRIPTOR, NB_VHD_SDI_BUFFERTYPE>;
        std::array<SlotBufferDescriptors, _buffer_queue_depth> _application_buffers;
        std::array<HANDLE, _buffer_queue_depth> _slots;

        uint64_t _number_of_pushed_slots = 0;
        std::optional<uint32_t> _slots_dropped;

        ApiPopSlot _api_pop_slot_fct;
        ApiPushSlot _api_push_slot_fct;
    };
}