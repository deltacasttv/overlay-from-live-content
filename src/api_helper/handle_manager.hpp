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
#include <memory>

#include "VideoMasterHD_Core.h"

#include "api_success.hpp"
#include "resource_manager.hpp"

namespace Deltacast
{
   namespace Helper
   {
      using BoardHandle = ResourceManager<HANDLE>;
      using StreamHandle = ResourceManager<HANDLE>;

      std::unique_ptr<BoardHandle> get_board_handle(ULONG board_index);

      template<typename StreamProcessingMode>
      std::unique_ptr<StreamHandle> get_stream_handle(BoardHandle& board_handle, VHD_STREAMTYPE stream_type, StreamProcessingMode processing_mode)
      {
         HANDLE raw_handle = NULL;
         ApiSuccess api_success{VHD_OpenStreamHandle(*board_handle, stream_type, processing_mode, NULL, &raw_handle, NULL)};
         if (!api_success)
         {
            std::cout << "ERROR: Cannot open stream handle (" << api_success << ")" << std::endl;
            return {};
         }

         return std::make_unique<StreamHandle>(raw_handle, [](HANDLE handle) { VHD_StopStream(handle); VHD_CloseStreamHandle(handle); });
      }
   }
}
