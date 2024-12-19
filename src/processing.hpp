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

#include <cstdint>
#include <functional>

using Processor = std::function<void(const uint8_t*, uint32_t, uint8_t*, uint32_t)>;

void generate_overlay(const uint8_t* buffer, uint32_t buffer_size, uint8_t* overlay_buffer, uint32_t overlay_buffer_size);
void generate_frame(const uint8_t* buffer, uint32_t buffer_size, uint8_t* output_buffer, uint32_t output_buffer_size);
