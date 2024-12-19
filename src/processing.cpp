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

#include "processing.hpp"

#include <iostream>
#include <cstring>
#include <vector>
#include <thread>

void generate_overlay(const uint8_t* buffer, uint32_t buffer_size, uint8_t* overlay_buffer, uint32_t overlay_buffer_size)
{
    auto generate = [] (const uint8_t* buffer, uint8_t* overlay_buffer, uint32_t first_pixel, uint32_t last_pixel)
    {
        for (uint32_t i = first_pixel; i < last_pixel; ++i)
        {
            uint32_t pixel_index = i * 3;
            uint32_t overlay_pixel_index = i * 4;

            overlay_buffer[overlay_pixel_index + 0] = buffer[pixel_index + 0];
            overlay_buffer[overlay_pixel_index + 1] = buffer[pixel_index + 1];
            overlay_buffer[overlay_pixel_index + 2] = buffer[pixel_index + 2];
            overlay_buffer[overlay_pixel_index + 3] = 0xFF;
        }
    };

    memset(overlay_buffer, 0, overlay_buffer_size);

    const uint32_t number_of_pixels = buffer_size / 3;
    const uint32_t starting_point = (number_of_pixels / 2);
    const uint32_t number_of_pixels_to_process = (number_of_pixels / 2);

    const int number_of_partitions = 4;
    const uint32_t partition_size = number_of_pixels_to_process / number_of_partitions;

    std::vector<std::thread> processors;
    for (int i = 0; i < number_of_partitions; ++i)
        processors.emplace_back(generate, buffer, overlay_buffer, starting_point + i * partition_size, starting_point + (i + 1) * partition_size);

    for (auto& processor : processors)
        processor.join();
}

void generate_frame(const uint8_t* buffer, uint32_t buffer_size, uint8_t* output_buffer, uint32_t output_buffer_size)
{
    memcpy(output_buffer, buffer, output_buffer_size/2);
}
