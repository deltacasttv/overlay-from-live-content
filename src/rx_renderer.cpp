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

#include "rx_renderer.hpp"

#include <iostream>
#include <cstring>

RxRenderer::RxRenderer(std::string window_title, int window_width, int window_height, int framerate_ms)
    : _window_title(window_title)
    , _window_width(window_width)
    , _window_height(window_height)
    , _framerate_ms(framerate_ms)
{
}

RxRenderer::~RxRenderer()
{
    stop();
}

bool RxRenderer::init(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format, bool progressive)
{
    _monitor_thread = std::thread(&RxRenderer::monitor, this, image_width, image_height, input_format);
    _progressive = progressive;
    _image_height = image_height;

    return true;
}

bool RxRenderer::monitor(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format)
{
    if (!_monitor.init(_window_width, _window_height, _window_title.c_str(), image_width, image_height, input_format))
    {
        std::cout << "ERROR: VideoViewer initialization failed" << std::endl;
        return false;
    }

    _monitor.render_loop(_framerate_ms);
    _monitor.release();

    return true;
}

bool RxRenderer::start(Deltacast::SharedResources& shared_resources)
{
    _rendering_loop_thread = std::thread(&RxRenderer::render_loop, this, std::ref(shared_resources));

    return true;
}

bool RxRenderer::stop()
{
    _should_stop = true;
    if (_rendering_loop_thread.joinable())
        _rendering_loop_thread.join();

    _monitor.stop();
    if (_monitor_thread.joinable())
        _monitor_thread.join();

    return true;
}

void RxRenderer::render_loop(Deltacast::SharedResources& shared_resources)
{
    std::unique_ptr<uint8_t[]> to_render_data = nullptr;
    uint64_t to_render_data_size = 0;
    
    while (!_should_stop)
    {
        {
            auto lock = shared_resources.synchronization.lock();

            if (shared_resources.buffer && shared_resources.buffer_size)
            {
                if (!to_render_data || (to_render_data_size != shared_resources.buffer_size))
                {
                    to_render_data.reset(new uint8_t[shared_resources.buffer_size]);
                    to_render_data_size = shared_resources.buffer_size;
                }

                if (_progressive) {
                    memcpy(to_render_data.get(), (uint8_t*)shared_resources.buffer, to_render_data_size);
                } else {
                    const int per_line = to_render_data_size / _image_height;
                    for (int i = 0; i < _image_height / 2; ++i) {
                        memcpy(to_render_data.get() + (i * 2) * per_line, (uint8_t*)shared_resources.buffer + i * per_line, per_line);
                    }
                    const int interlace_offset = per_line * (_image_height / 2);
                    for (int i = 0; i < _image_height / 2; ++i) {
                        memcpy(to_render_data.get() + (i * 2 + 1) * per_line, (uint8_t*)shared_resources.buffer + i * per_line + interlace_offset, per_line);
                    }
                }
            }
        }

        uint8_t* monitor_data = nullptr;
        uint64_t monitor_data_size = 0;
        if (_monitor.lock_data(&monitor_data, &monitor_data_size)) 
        {
            if (to_render_data && monitor_data && (monitor_data_size == shared_resources.buffer_size) && shared_resources.buffer)
                memcpy(monitor_data, to_render_data.get(), monitor_data_size);

            _monitor.unlock_data();
        }
    }
}