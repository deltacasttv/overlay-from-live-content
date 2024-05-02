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

#include <iostream>
#include <string>
#include <csignal>
#include <functional>

#include "VideoMasterHD_Core.h"
#include "VideoMasterHD_Sdi.h"
#include "VideoMasterHD_Keyer.h"

#include "version.h"

#include "VideoMasterAPIHelper/api.hpp"
#include "VideoMasterAPIHelper/api_success.hpp"
#include "VideoMasterAPIHelper/enum_to_string.hpp"
#include "allocation.hpp"
#include "device.hpp"
#include "rx_renderer.hpp"
#include "rx_stream.hpp"
#include "tx_stream.hpp"
#include "processing.hpp"

#include <CLI/CLI.hpp>

Deltacast::SharedResources shared_resources;

void on_close(int /*signal*/)
{
    shared_resources.synchronization.stop_is_requested = true;
}

void waiting(Deltacast::SharedResources& shared_resources)
{
    const uint32_t max_number_of_dots = 10;
    uint32_t number_of_dots = 0;

    while (!shared_resources.synchronization.stop_is_requested
        && !shared_resources.synchronization.signal_has_changed)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cout << "\r"
            << std::string(++number_of_dots, '.')
            << std::string(max_number_of_dots - number_of_dots, ' ')
            << std::flush;

        if (number_of_dots >= max_number_of_dots)
            number_of_dots = 0;
    }

    std::cout << std::endl;
}

int main(int argc, char** argv)
{
    using namespace std::chrono_literals;

    int device_id = 0;
    int rx_stream_id = 0;
    int tx_stream_id = 0;
    bool overlay_enabled = false;
    bool renderer_enabled = false;
    bool auto_stream_reconfiguration_enabled;
    shared_resources.maximum_latency = 2;

    CLI::App app{"Generates some content from input and sends it to output"};
    app.add_option("-d,--device", device_id, "ID of the device to use");
    app.add_option("-i,--input", rx_stream_id, "ID of the input connector to use");
    app.add_option("-o,--output", tx_stream_id, "ID of the output connector to use");
    app.add_flag("--overlay,!--no-overlay", overlay_enabled, "Activates overlay on the output stream");
    app.add_flag("--renderer,!--no-renderer", renderer_enabled, "Activates rendering of the live input stream");
    app.add_flag("--auto-stream-reconfiguration,!--no-auto-stream-reconfiguration", auto_stream_reconfiguration_enabled,
    "Activates the automatic reconfiguration of the stream when the input signal changes, not recommended for HDMI devices.");
    app.add_option("-l,--maximum-latency", shared_resources.maximum_latency, "Maximum desired latency in frames between input and output");
    CLI11_PARSE(app, argc, argv);

    signal(SIGINT, on_close);

    std::cout << "OverlayFromLiveContent (" << VERSTRING << ")" << std::endl;

    std::cout << std::endl;

    std::cout << "VideoMaster: " << Deltacast::Helper::get_api_version() << std::endl;
    std::cout << "Discovered " << Deltacast::Helper::get_number_of_devices() << " devices" << std::endl;

    std::cout << "Opening device " << device_id << std::endl;
    auto device = Deltacast::Device::create(device_id);
    if (!device)
        return -1;

    std::cout << *device << std::endl;

    if (!device->suitable())
    {
        std::cout << "ERROR: Device is not suitable for running this application" << std::endl;
        return -1;
    }

    while (!shared_resources.synchronization.stop_is_requested)
    {
        shared_resources.reset();

        // identify connector type
        shared_resources.rx_video_info = std::move(device->factory_create_video_information_for_channel(rx_stream_id, Deltacast::Device::Direction::RX));
        shared_resources.tx_video_info = std::move(device->factory_create_video_information_for_channel(tx_stream_id, Deltacast::Device::Direction::TX));
        std::cout << "Input  connector : " << *shared_resources.rx_video_info << std::endl;
        std::cout << "Output connector : " << *shared_resources.tx_video_info << std::endl;

        std::cout << "Waiting for incoming signal" << std::endl;
        if (!device->wait_for_incoming_signal(rx_stream_id, shared_resources.synchronization.stop_is_requested))
            return -1;

        std::cout << "Configuring genlock" << std::endl;
        if (!device->configure_sync(rx_stream_id, shared_resources.rx_video_info))
            return -1;
        std::cout << "Waiting for genlock locked" << std::endl;
        if (!device->wait_sync_locked(shared_resources.synchronization.stop_is_requested, shared_resources.rx_video_info))
            return -1;

        if (overlay_enabled)
        {
            std::cout << "Configuring keyer" << std::endl;
            if (!device->configure_keyer(rx_stream_id, tx_stream_id, shared_resources.rx_video_info))
                return -1;
        }

        std::cout << "Disabling loopback" << std::endl;
        device->disable_loopback(rx_stream_id);

        std::cout << "Opening RX stream " << rx_stream_id << "" << std::endl;
        auto rx_stream = Deltacast::RxStream::create(*device, rx_stream_id, shared_resources.rx_video_info
                                                    , allocate_buffer, deallocate_buffer, auto_stream_reconfiguration_enabled);
        if (!rx_stream)
            return -1;

        std::cout << "Configuring RX stream" << std::endl;
        if (!rx_stream->configure(shared_resources.rx_video_info, overlay_enabled))
            return -1;

        std::cout << "Incoming video format" << std::endl << rx_stream->video_format() << std::endl;
        auto window_refresh_interval = 10ms;
        RxRenderer renderer("Live Content", rx_stream->video_format().width / 2, rx_stream->video_format().height / 2, window_refresh_interval.count());
        if (renderer_enabled)
        {
            std::cout << "Initializing live content rendering window" << std::endl;
            if (!renderer.init(rx_stream->video_format().width, rx_stream->video_format().height, Deltacast::VideoViewer::InputFormat::bgr_444_8))
                return -1;

            std::cout << std::endl;
        }

        std::cout << "Opening TX stream " << tx_stream_id << "" << std::endl;
        auto tx_stream = Deltacast::TxStream::create(*device, tx_stream_id, shared_resources.tx_video_info
                                                    , allocate_buffer, deallocate_buffer
                                                    , (overlay_enabled ? generate_overlay : generate_frame));
        if (!tx_stream)
            return -1;

        std::cout << "Starting RX stream" << std::endl;
        if (!rx_stream->start(shared_resources))
            return -1;

        std::cout << "Configuring and starting TX stream" << std::endl;
        auto props = rx_stream->get_stream_properties(shared_resources.rx_video_info);
        if (!tx_stream->configure(shared_resources.tx_video_info, overlay_enabled, props))
            return -1;
        std::cout << "Outgoing video format" << std::endl << tx_stream->video_format() << std::endl;
        if (!tx_stream->start(shared_resources))
            return -1;

        if (renderer_enabled)
        {
            std::cout << "Starting live content rendering" << std::endl;
            renderer.start(shared_resources);

            std::cout << std::endl;
        }

        std::cout << "Press CTRL+C to stop" << std::endl;
        waiting(shared_resources);

        std::cout << std::endl;

        if (renderer_enabled)
        {
            std::cout << "Stopping live content rendering" << std::endl;
            renderer.stop();

        std::cout << std::endl;
        }

        std::cout << "Enabling loopback" << std::endl;
        device->enable_loopback(rx_stream_id);

        std::cout << std::endl;

        std::cout << "Stopping RX and TX loops" << std::endl;
        tx_stream->stop();
        rx_stream->stop();

        std::cout << std::endl;
    }

    return 0;
}
