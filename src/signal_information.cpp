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

#include "signal_information.hpp"

#include "api_helper/api_success.hpp"

using Deltacast::Helper::ApiSuccess;

Deltacast::DecodedSignalInformation Deltacast::decode(const Deltacast::SignalInformation& signal_information)
{
    ULONG width = 0, height = 0, framerate = 0;
    BOOL32 interlaced = FALSE;

    ApiSuccess api_success;
    if (!(api_success = ApiSuccess{VHD_GetVideoCharacteristics(signal_information.video_standard, &width, &height, &interlaced, &framerate)}))
    {
        std::cout << "ERROR: Cannot translate incoming signal information (" << api_success << ")" << std::endl;
        return {};
    }

    DecodedSignalInformation decoded_signal_information;
    decoded_signal_information.width = width;
    decoded_signal_information.height = height;
    decoded_signal_information.progressive = !interlaced;
    decoded_signal_information.framerate = framerate / (signal_information.clock_divisor == VHD_CLOCKDIV_1001 ? 1.001 : 1);

    return decoded_signal_information;
}