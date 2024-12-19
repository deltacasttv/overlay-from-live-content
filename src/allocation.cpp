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

#include "allocation.hpp"

#include <iostream>

#if !defined(__linux__) && !defined(__APPLE__)
#include <Windows.h>
#endif

namespace Application::Allocation
{
    VHD_APPLICATION_BUFFER_DESCRIPTOR allocate_buffer(ULONG buffer_size)
    {
        VHD_APPLICATION_BUFFER_DESCRIPTOR buffer_descriptor;
        buffer_descriptor.Size = sizeof(buffer_descriptor);
        buffer_descriptor.RDMAEnabled = FALSE;

    #ifdef WIN32 
        buffer_descriptor.pBuffer = (UBYTE*)(VirtualAlloc(NULL, buffer_size, MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE));
    #else
        int error = posix_memalign((void**)&buffer_descriptor.pBuffer, 4096, buffer_size);
        if (error != 0)
            std::cout << "posix_memalign failed to alloc " << buffer_size << " bytes (error = " << error << ")" << std::endl;
    #endif

        return buffer_descriptor;
    }

    void deallocate_buffer(VHD_APPLICATION_BUFFER_DESCRIPTOR buffer_descriptor)
    {
    #ifdef WIN32 
        VirtualFree(buffer_descriptor.pBuffer, 0, MEM_RELEASE);
    #else
        free(buffer_descriptor.pBuffer);
    #endif
        buffer_descriptor.pBuffer = nullptr;
    }
}