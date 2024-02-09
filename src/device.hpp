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

#include <iostream>
#include <atomic>

#include "VideoMasterAPIHelper/handle_manager.hpp"
#include "VideoMasterAPIHelper/VideoInformation/core.hpp"
#include "signal_information.hpp"

namespace Deltacast
{
    class Device
    {
    private:
        Device() = delete;
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        Device(int device_index, std::unique_ptr<Helper::BoardHandle> device_handle)
            : _device_index(device_index)
            , _device_handle(std::move(device_handle))
        {
        }

    public:
        ~Device();

        static std::unique_ptr<Device> create(int device_index);

        bool suitable();

        void enable_loopback(int index);
        void disable_loopback(int index);
        bool wait_for_incoming_signal(int rx_index, std::unique_ptr<VideoMasterVideoInformation>& v_info, const std::atomic_bool& stop_is_requested);
        // bool detect_incoming_signal_properties(int rx_index, std::unique_ptr<VideoMasterVideoInformation>& sdi_v_info);
        std::unique_ptr<VideoMasterVideoInformation> get_video_information_for_channel(int index);

        bool wait_genlock_locked(const std::atomic_bool& stop_is_requested);
        bool configure_genlock(int genlock_source_rx_index, std::unique_ptr<VideoMasterVideoInformation>& video_info);

        bool configure_keyer(int rx_index, int tx_index);

        int& index() { return _device_index; }
        Helper::BoardHandle& handle() { return *_device_handle; }

        friend std::ostream& operator<<(std::ostream& os, const Device& device);

        enum class Direction
        {
            RX,
            TX
        };

    private:
        int _device_index;
        std::unique_ptr<Helper::BoardHandle> _device_handle;

        bool set_loopback_state(int index, bool enabled);
        std::optional<ULONG> get_channel_type(int index, Direction direction);
    };
}