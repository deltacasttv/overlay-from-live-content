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

#include <functional>

namespace Deltacast
{
   namespace Helper
   {
      template<typename ResourceType>
      class ResourceManager
      {
      public:
         ResourceManager(ResourceType resource, std::function<void(ResourceType)> destroyer) : _resource(resource), _destroyer(destroyer) {}
         ~ResourceManager() { _destroyer(_resource); }

         ResourceManager& operator=(const ResourceManager&) = delete;
         ResourceManager(const ResourceManager&) = delete;

         constexpr ResourceType* operator->() { return &_resource; };
         constexpr ResourceType& operator*() { return _resource; };

      private:
         ResourceType _resource;
         std::function<void(ResourceType)> _destroyer;
      };
   }
}