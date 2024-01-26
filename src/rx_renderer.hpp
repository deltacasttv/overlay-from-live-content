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

#include <videoviewer/videoviewer.hpp>
#include <thread>
#include <mutex>

class RxRenderer
{
public:
    RxRenderer(std::string window_title, int window_width, int window_height, int framerate_ms);
    ~RxRenderer();
    
    RxRenderer(const RxRenderer&) = delete;
    RxRenderer& operator=(const RxRenderer&) = delete;
    RxRenderer(RxRenderer&&) = delete;
    RxRenderer& operator=(RxRenderer&&) = delete;

    bool init(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format);
    bool start();
    bool stop();

    void update_buffer(uint8_t* buffer, uint32_t buffer_size);

private:
    std::string _window_title;
    int _window_width;
    int _window_height;
    int _framerate_ms;

    Deltacast::VideoViewer _monitor;
    std::thread _monitor_thread;

    bool _should_stop = false;
    std::thread _rendering_loop_thread;

    bool monitor(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format);
    void render_loop();

    std::mutex _buffer_mutex;
    std::unique_ptr<uint8_t> _buffer = nullptr;
};