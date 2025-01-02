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

#include "shared_resources.hpp"

#include <iostream>
#include <thread>
#include <atomic>

class WindowedRenderer
{
public:
    WindowedRenderer(std::string window_title, int window_width, int window_height, int framerate_ms, std::atomic_bool& stop_is_requested);
    ~WindowedRenderer();
    
    WindowedRenderer(const WindowedRenderer&) = delete;
    WindowedRenderer& operator=(const WindowedRenderer&) = delete;
    WindowedRenderer(WindowedRenderer&&) = delete;
    WindowedRenderer& operator=(WindowedRenderer&&) = delete;

    bool init(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format);
    bool start(Deltacast::SharedResources& shared_resources);
    bool stop();

private:
    std::string _window_title;
    int _window_width;
    int _window_height;
    int _framerate_ms;

    Deltacast::VideoViewer _monitor;
    std::thread _monitor_thread;

    std::atomic_bool& _should_stop;
    std::atomic_bool _monitor_ready;
    std::thread _rendering_loop_thread;

    bool monitor(int image_width, int image_height, Deltacast::VideoViewer::InputFormat input_format);
    void render_loop(Deltacast::SharedResources& shared_resources);
};