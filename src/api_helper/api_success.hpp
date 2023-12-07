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

#include "VideoMasterHD_Core.h"

#include "enum_to_string.hpp"

namespace Deltacast
{
    namespace Helper
    {
        class ApiSuccess
        {
        public:
            ApiSuccess() : _api_error_code(VHDERR_NOERROR) {};
            explicit ApiSuccess(ULONG api_error_code) : _api_error_code((VHD_ERRORCODE)api_error_code) {};
            explicit ApiSuccess(VHD_ERRORCODE api_error_code) : _api_error_code(api_error_code) {};
            
            ApiSuccess& operator=(ULONG api_error_code) { _api_error_code = (VHD_ERRORCODE)api_error_code; return *this; } 

            VHD_ERRORCODE error_code() const { return _api_error_code; };
            operator bool() const { return _api_error_code == VHDERR_NOERROR; };

            friend std::ostream& operator<<(std::ostream& os, const ApiSuccess& api_success)
            {
                if (api_success)
                    os << "no error";
                else
                    os << enum_to_string(api_success._api_error_code) << "(error code=" << api_success._api_error_code << ")";

                return os;
            };

        private:
            VHD_ERRORCODE _api_error_code;
        };
    }
}
