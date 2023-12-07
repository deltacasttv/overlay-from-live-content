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

#include "api_success.hpp"
#include "handle_manager.hpp"

std::unique_ptr<Deltacast::Helper::BoardHandle> Deltacast::Helper::get_board_handle(ULONG board_index)
{
   HANDLE raw_handle = NULL;
   ApiSuccess api_success{VHD_OpenBoardHandle(board_index, &raw_handle, NULL, 0)};
   if (!api_success)
   {
      std::cout << "ERROR: Cannot open board " << board_index << " handle (" << api_success << ")" << std::endl;
      return {};
   }

   return std::make_unique<BoardHandle>(raw_handle, [](HANDLE handle)
   {
      VHD_CloseBoardHandle(handle);
   });
}
