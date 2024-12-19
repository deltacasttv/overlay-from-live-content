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

#include <CLI/CLI.hpp>

#include <VideoMasterCppApi/exception.hpp>
#include <VideoMasterCppApi/to_string.hpp>
#include <VideoMasterCppApi/api.hpp>
#include <VideoMasterCppApi/board/board.hpp>

#include "version.h"
#include "helper.hpp"
#include "shared_resources.hpp"
#include "windowed_renderer.hpp"
#include "allocation.hpp"
#include "processing.hpp"

using namespace std::chrono_literals;
using namespace Deltacast::Wrapper;

Deltacast::SharedResources shared_resources;

void on_close(int /*signal*/)
{
    shared_resources.synchronization.stop_is_requested = true;
}

bool rx_loop(Application::Helper::TechStream& rx_tech_stream, Deltacast::SharedResources& shared_resources);
bool tx_loop(Application::Helper::TechStream& tx_tech_stream, Deltacast::SharedResources& shared_resources);

int main(int argc, char** argv)
{
    CLI::App app{"Generates some content from input and sends it to output"};
    int device_id = 0;
    app.add_option("-d,--device", device_id, "ID of the device to use");
    int rx_stream_id = 0;
    app.add_option("-i,--input", rx_stream_id, "ID of the input connector to use");
    int tx_stream_id = 0;
    app.add_option("-o,--output", tx_stream_id, "ID of the output connector to use");
    bool overlay_enabled = false;
    app.add_flag("--overlay,!--no-overlay", overlay_enabled, "Activates overlay on the output stream");
    bool renderer_enabled = false;
    app.add_flag("--renderer,!--no-renderer", renderer_enabled, "Activates rendering of the live input stream");
    shared_resources.maximum_latency = 2;
    app.add_option("-l,--maximum-latency", shared_resources.maximum_latency, "Maximum desired latency in frames between input and output");
    CLI11_PARSE(app, argc, argv);

    signal(SIGINT, on_close);

    std::cout << "VideoMaster overlay-from-live-content (" << VERSTRING << ")" << std::endl;
    
    try
    {    
        std::cout << "VideoMaster API version: " << api_version() << std::endl;
        std::cout << "Discovered " << Board::count() << " devices" << std::endl;

        if (device_id >= Board::count())
        {
            std::cout << "Invalid device ID" << std::endl;
            return -1;
        }

        std::cout << "Opening device " << device_id << std::endl;
        auto board = Board::open(device_id, [&rx_stream_id](Board& board) { Application::Helper::enable_loopback(board, rx_stream_id); });

        std::cout << board << std::endl;

        // if (!device->suitable())
        // {
        //     std::cout << "ERROR: Device is not suitable for running this application" << std::endl;
        //     return -1;
        // }

        Application::Helper::disable_loopback(board, rx_stream_id);

        while (!shared_resources.synchronization.stop_is_requested)
        {
            shared_resources.reset();

            std::cout << "Opening RX" << rx_stream_id << " stream..." << std::endl;
            auto rx_tech_stream = Application::Helper::open_stream(board, Application::Helper::rx_index_to_streamtype(rx_stream_id));
            auto& rx_stream = Application::Helper::to_base_stream(rx_tech_stream);

            std::cout << "Waiting for signal..." << std::endl;
            if (!Application::Helper::wait_for_input(board.rx(rx_stream_id), shared_resources.synchronization.stop_is_requested))
            {
                std::cerr << "Application has been stopped before any input was received." << std::endl;
                return -1;
            }

            auto signal_information = Application::Helper::detect_information(rx_tech_stream);
            auto video_characteristics = Application::Helper::get_video_characteristics(signal_information);
            std::cout << "Detected:" << std::endl;
            Application::Helper::print_information(signal_information, "\t");

            std::unique_ptr<WindowedRenderer> renderer;
            if (renderer_enabled)
            {
                auto window_refresh_interval = 10ms;
                renderer = std::make_unique<WindowedRenderer>("Live Content", video_characteristics.width / 2, video_characteristics.height / 2
                                                        , window_refresh_interval.count(), shared_resources.synchronization.stop_is_requested);
                std::cout << "Initializing live content rendering window..." << std::endl;
                renderer->init(video_characteristics.width, video_characteristics.height, Deltacast::VideoViewer::InputFormat::bgr_444_8);
            }

            std::cout << std::endl;

            // std::cout << "Configuring genlock" << std::endl;
            // if (!device->configure_genlock(rx_stream_id, shared_resources.signal_info))
            //     return -1;
            // std::cout << "Waiting for genlock locked" << std::endl;
            // if (!device->wait_genlock_locked(shared_resources.synchronization.stop_is_requested))
            //     return -1;

            std::cout << std::endl;

            if (overlay_enabled)
            {
                // std::cout << "Configuring keyer" << std::endl;
                // if (!device->configure_keyer(rx_stream_id, tx_stream_id))
                //     return -1;

                // std::cout << std::endl;
            }

            std::cout << "Opening TX" << tx_stream_id << " stream..." << std::endl;
            auto tx_tech_stream = Application::Helper::open_stream(board, Application::Helper::tx_index_to_streamtype(tx_stream_id));
            auto& tx_stream = Application::Helper::to_base_stream(tx_tech_stream);
                        
            std::cout << "Configuring RX stream..." << std::endl;
            rx_stream.buffer_queue().set_depth(16);
            rx_stream.buffer_queue().set_transfer_scheme(VHD_TRANSFER_UNCONSTRAINED);
            rx_stream.set_buffer_packing(VHD_BUFPACK_VIDEO_RGB_24);
            Application::Helper::configure_stream(rx_tech_stream, signal_information);
            std::cout << "Starting RX stream..." << std::endl;
            std::thread rx_thread(rx_loop, std::ref(rx_tech_stream), std::ref(shared_resources));

            std::cout << "Configuring TX stream..." << std::endl;
            tx_stream.buffer_queue().set_depth(16);
            tx_stream.buffer_queue().set_preload(0);
            tx_stream.set_buffer_packing(overlay_enabled ? VHD_BUFPACK_VIDEO_RGBA_32 : VHD_BUFPACK_VIDEO_RGB_24);
            if (std::holds_alternative<SdiStream>(tx_tech_stream))
                std::get<SdiStream>(tx_tech_stream).genlock().enable();
            Application::Helper::configure_stream(tx_tech_stream, signal_information);
            std::cout << "Starting TX stream..." << std::endl;
            std::thread tx_thread(tx_loop, std::ref(tx_tech_stream), std::ref(shared_resources));

            if (renderer_enabled)
            {
                std::cout << "Starting live content rendering" << std::endl;
                renderer->start(shared_resources);
                std::cout << std::endl;
            }

            const uint32_t max_number_of_dots = 10;
            uint32_t number_of_dots = 0;
            while (!shared_resources.synchronization.stop_is_requested && !shared_resources.synchronization.incoming_signal_changed)
            {
                if (!Application::Helper::wait_for_input(board.rx(rx_stream_id), shared_resources.synchronization.stop_is_requested))
                {
                    std::this_thread::sleep_for(100ms);
                    continue;
                }
                if (Application::Helper::detect_information(rx_tech_stream) != signal_information)
                {
                    shared_resources.synchronization.incoming_signal_changed = true;
                    continue;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                std::cout << "\r" 
                    << std::string(++number_of_dots, '.')
                    << std::string(max_number_of_dots - number_of_dots, ' ')
                    << std::flush;

                if (number_of_dots >= max_number_of_dots)
                    number_of_dots = 0;
            }
            std::cout << std::endl;

            rx_thread.join();
            tx_thread.join();
        }
    }
    catch (const ApiException& e)
    {
        std::cerr << e.what() << std::endl;
        std::cerr << e.logs() << std::endl;
    }

    return 0;
}

bool rx_loop(Application::Helper::TechStream& rx_tech_stream, Deltacast::SharedResources& shared_resources)
{
    auto& rx_stream = Application::Helper::to_base_stream(rx_tech_stream);
    rx_stream.start();

    while (!shared_resources.synchronization.stop_is_requested)
    {
        {
            std::unique_ptr<Slot> slot = nullptr;
            do
            {
                try { slot = rx_stream.pop_slot(); }
                catch(const ApiException& e) { return e.error_code() == VHDERR_TIMEOUT; }
            } while (rx_stream.buffer_queue().filling() > 0);

            auto& [ buffer, buffer_size ] = slot->video().buffer();
            shared_resources.buffer = buffer;
            shared_resources.buffer_size = buffer_size;

            shared_resources.synchronization.notify_ready_to_process();
            while (!shared_resources.synchronization.stop_is_requested
                && !shared_resources.synchronization.wait_until_processed()) {}
        }
        
        // if (!shared_resources.synchronization.stop_is_requested)
        // {
        //     ULONG slots_count = 0, slots_dropped = 0;
        //     VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_COUNT, &slots_count);
        //     VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_DROPPED, &slots_dropped);

        //     if (!_slots_dropped.has_value())
        //         _slots_dropped = slots_dropped;

        //     if (_slots_dropped != slots_dropped)
        //     {
        //         std::cout << "INFO for " << _name << ": Dropped occurred: slots count=" << slots_count << ", slots dropped=" << slots_dropped << std::endl;
        //         _slots_dropped = slots_dropped;
        //     }
        // }
    }
    
    return true;
}

bool tx_loop_processing(Application::Helper::TechStream& tx_tech_stream, Slot& slot, Deltacast::SharedResources& shared_resources);

bool tx_loop(Application::Helper::TechStream& tx_tech_stream, Deltacast::SharedResources& shared_resources)
{
    auto& tx_stream = Application::Helper::to_base_stream(tx_tech_stream);
    tx_stream.start();

    while (!shared_resources.synchronization.stop_is_requested)
    {
        std::unique_ptr<Slot> slot = nullptr;
        try { slot = tx_stream.pop_slot(); }
        catch(const ApiException& e) { return e.error_code() == VHDERR_TIMEOUT; }

        bool success = tx_loop_processing(tx_tech_stream, *slot, shared_resources);
        shared_resources.synchronization.notify_processing_finished();
        if (!success)
            return false;

        // if (!shared_resources.synchronization.stop_is_requested)
        // {
        //     ULONG slots_count = 0, slots_dropped = 0;
        //     VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_COUNT, &slots_count);
        //     VHD_GetStreamProperty(*handle(), VHD_CORE_SP_SLOTS_DROPPED, &slots_dropped);

        //     if (!_slots_dropped.has_value())
        //         _slots_dropped = slots_dropped;

        //     if (_slots_dropped != slots_dropped)
        //     {
        //         std::cout << "INFO for " << _name << ": Dropped occurred: slots count=" << slots_count << ", slots dropped=" << slots_dropped << std::endl;
        //         _slots_dropped = slots_dropped;
        //     }
        // }
    }
    
    return true;
}

bool tx_loop_processing(Application::Helper::TechStream& tx_tech_stream, Slot& slot, Deltacast::SharedResources& shared_resources)
{
    auto& tx_stream = Application::Helper::to_base_stream(tx_tech_stream);

    while (!shared_resources.synchronization.stop_is_requested && !shared_resources.synchronization.wait_until_ready_to_process()) {}
    if (shared_resources.synchronization.stop_is_requested)
        return false;

    auto buffer_queue_filling = tx_stream.buffer_queue().filling();
    if (buffer_queue_filling > (shared_resources.maximum_latency - 2))
    {
        for (auto i = 0; i < buffer_queue_filling - (shared_resources.maximum_latency - 2); ++i)
        {
            shared_resources.synchronization.notify_processing_finished();
            while (!shared_resources.synchronization.stop_is_requested && !shared_resources.synchronization.wait_until_ready_to_process()) {}
        }
    }

    auto& [ buffer, buffer_size ] = slot.video().buffer();

    generate_frame(shared_resources.buffer, shared_resources.buffer_size, buffer, buffer_size);

    return true;
}