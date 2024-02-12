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

#include "device.hpp"

#include <string>
#include <thread>

#include "VideoMasterHD_Sdi_Keyer.h"

#include "VideoMasterAPIHelper/handle_manager.hpp"

#include "VideoMasterAPIHelper/VideoInformation/dv.hpp"
#include "VideoMasterAPIHelper/VideoInformation/sdi.hpp"

using Deltacast::Helper::ApiSuccess;

const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_rx_channel_type_prop = {
   { 0, VHD_CORE_BP_RX0_TYPE }, { 1, VHD_CORE_BP_RX1_TYPE },   { 2, VHD_CORE_BP_RX2_TYPE },
   { 3, VHD_CORE_BP_RX3_TYPE }, { 4, VHD_CORE_BP_RX4_TYPE },   { 5, VHD_CORE_BP_RX5_TYPE },
   { 6, VHD_CORE_BP_RX6_TYPE }, { 7, VHD_CORE_BP_RX7_TYPE },   { 8, VHD_CORE_BP_RX8_TYPE },
   { 9, VHD_CORE_BP_RX9_TYPE }, { 10, VHD_CORE_BP_RX10_TYPE }, { 11, VHD_CORE_BP_RX11_TYPE }
};

const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_tx_channel_type_prop = {
   { 0, VHD_CORE_BP_TX0_TYPE }, { 1, VHD_CORE_BP_TX1_TYPE },   { 2, VHD_CORE_BP_TX2_TYPE },
   { 3, VHD_CORE_BP_TX3_TYPE }, { 4, VHD_CORE_BP_TX4_TYPE },   { 5, VHD_CORE_BP_TX5_TYPE },
   { 6, VHD_CORE_BP_TX6_TYPE }, { 7, VHD_CORE_BP_TX7_TYPE },   { 8, VHD_CORE_BP_TX8_TYPE },
   { 9, VHD_CORE_BP_TX9_TYPE }, { 10, VHD_CORE_BP_TX10_TYPE }, { 11, VHD_CORE_BP_TX11_TYPE }
};

const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_passive_loopback_prop = {
   { 0, VHD_CORE_BP_BYPASS_RELAY_0 },
   { 1, VHD_CORE_BP_BYPASS_RELAY_1 },
   { 2, VHD_CORE_BP_BYPASS_RELAY_2 },
   { 3, VHD_CORE_BP_BYPASS_RELAY_3 }
};
const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_active_loopback_prop = {
   { 0, VHD_CORE_BP_ACTIVE_LOOPBACK_0 }
};
const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_firmware_loopback_prop = {
   { 0, VHD_CORE_BP_FIRMWARE_LOOPBACK_0 }
};

const std::unordered_map<uint32_t, VHD_CORE_BOARDPROPERTY> id_to_rx_status_prop = {
   { 0, VHD_CORE_BP_RX0_STATUS }, { 1, VHD_CORE_BP_RX1_STATUS },   { 2, VHD_CORE_BP_RX2_STATUS },
   { 3, VHD_CORE_BP_RX3_STATUS }, { 4, VHD_CORE_BP_RX4_STATUS },   { 5, VHD_CORE_BP_RX5_STATUS },
   { 6, VHD_CORE_BP_RX6_STATUS }, { 7, VHD_CORE_BP_RX7_STATUS },   { 8, VHD_CORE_BP_RX8_STATUS },
   { 9, VHD_CORE_BP_RX9_STATUS }, { 10, VHD_CORE_BP_RX10_STATUS }, { 11, VHD_CORE_BP_RX11_STATUS },
};
const std::unordered_map<uint32_t, VHD_SDI_BOARDPROPERTY> id_to_rx_video_standard_prop = {
   { 0, VHD_SDI_BP_RX0_STANDARD },   { 1, VHD_SDI_BP_RX1_STANDARD },
   { 2, VHD_SDI_BP_RX2_STANDARD },   { 3, VHD_SDI_BP_RX3_STANDARD },
   { 4, VHD_SDI_BP_RX4_STANDARD },   { 5, VHD_SDI_BP_RX5_STANDARD },
   { 6, VHD_SDI_BP_RX6_STANDARD },   { 7, VHD_SDI_BP_RX7_STANDARD },
   { 8, VHD_SDI_BP_RX8_STANDARD },   { 9, VHD_SDI_BP_RX9_STANDARD },
   { 10, VHD_SDI_BP_RX10_STANDARD }, { 11, VHD_SDI_BP_RX11_STANDARD },
};
const std::unordered_map<uint32_t, VHD_SDI_BOARDPROPERTY> id_to_rx_clock_divisor_prop = {
   { 0, VHD_SDI_BP_RX0_CLOCK_DIV },   { 1, VHD_SDI_BP_RX1_CLOCK_DIV },
   { 2, VHD_SDI_BP_RX2_CLOCK_DIV },   { 3, VHD_SDI_BP_RX3_CLOCK_DIV },
   { 4, VHD_SDI_BP_RX4_CLOCK_DIV },   { 5, VHD_SDI_BP_RX5_CLOCK_DIV },
   { 6, VHD_SDI_BP_RX6_CLOCK_DIV },   { 7, VHD_SDI_BP_RX7_CLOCK_DIV },
   { 8, VHD_SDI_BP_RX8_CLOCK_DIV },   { 9, VHD_SDI_BP_RX9_CLOCK_DIV },
   { 10, VHD_SDI_BP_RX10_CLOCK_DIV }, { 11, VHD_SDI_BP_RX11_CLOCK_DIV },
};
const std::unordered_map<uint32_t, VHD_SDI_BOARDPROPERTY> id_to_rx_interface_prop = {
   { 0, VHD_SDI_BP_RX0_INTERFACE },   { 1, VHD_SDI_BP_RX1_INTERFACE },
   { 2, VHD_SDI_BP_RX2_INTERFACE },   { 3, VHD_SDI_BP_RX3_INTERFACE },
   { 4, VHD_SDI_BP_RX4_INTERFACE },   { 5, VHD_SDI_BP_RX5_INTERFACE },
   { 6, VHD_SDI_BP_RX6_INTERFACE },   { 7, VHD_SDI_BP_RX7_INTERFACE },
   { 8, VHD_SDI_BP_RX8_INTERFACE },   { 9, VHD_SDI_BP_RX9_INTERFACE },
   { 10, VHD_SDI_BP_RX10_INTERFACE }, { 11, VHD_SDI_BP_RX11_INTERFACE },
};
const std::unordered_map<uint32_t, VHD_GENLOCKSOURCE> id_to_rx_genlock_source = {
   { 0, VHD_GENLOCK_RX0 }, { 1, VHD_GENLOCK_RX1 },   { 2, VHD_GENLOCK_RX2 },
   { 3, VHD_GENLOCK_RX3 }, { 4, VHD_GENLOCK_RX4 },   { 5, VHD_GENLOCK_RX5 },
   { 6, VHD_GENLOCK_RX6 }, { 7, VHD_GENLOCK_RX7 },   { 8, VHD_GENLOCK_RX8 },
   { 9, VHD_GENLOCK_RX9 }, { 10, VHD_GENLOCK_RX10 }, { 11, VHD_GENLOCK_RX11 },
};

const std::unordered_map<uint32_t, VHD_KEYERINPUT> id_to_keyer_rx_input = {
   { 0, VHD_KINPUT_RX0 },
   { 1, VHD_KINPUT_RX1 },
   { 2, VHD_KINPUT_RX2 },
   { 3, VHD_KINPUT_RX3 },
};
const std::unordered_map<uint32_t, VHD_KEYERINPUT> id_to_keyer_tx_input = {
   { 0, VHD_KINPUT_TX0 },
   { 1, VHD_KINPUT_TX1 },
   { 2, VHD_KINPUT_TX2 },
   { 3, VHD_KINPUT_TX3 },
};
const std::unordered_map<uint32_t, VHD_KEYEROUTPUT> id_to_keyer_rx_output = {
   { 0, VHD_KOUTPUT_RX0 },
   { 1, VHD_KOUTPUT_RX1 },
   { 2, VHD_KOUTPUT_RX2 },
   { 3, VHD_KOUTPUT_RX3 },
};
const std::unordered_map<uint32_t, VHD_KEYER_BOARDPROPERTY> id_to_keyer_video_output_prop = {
   { 0, VHD_KEYER_BP_VIDEOOUTPUT_TX_0 },
   { 1, VHD_KEYER_BP_VIDEOOUTPUT_TX_1 },
   { 2, VHD_KEYER_BP_VIDEOOUTPUT_TX_2 },
   { 3, VHD_KEYER_BP_VIDEOOUTPUT_TX_3 },
};
const std::unordered_map<uint32_t, VHD_KEYER_BOARDPROPERTY> id_to_keyer_anc_output_prop = {
   { 0, VHD_KEYER_BP_ANCOUTPUT_TX_0 },
   { 1, VHD_KEYER_BP_ANCOUTPUT_TX_1 },
   { 2, VHD_KEYER_BP_ANCOUTPUT_TX_2 },
   { 3, VHD_KEYER_BP_ANCOUTPUT_TX_3 },
};

Deltacast::Device::~Device()
{
   ULONG number_of_rx_channels = 0;
   VHD_GetBoardProperty(*handle(), VHD_CORE_BP_NB_RXCHANNELS, &number_of_rx_channels);

   for (auto i = 0; i < number_of_rx_channels; i++)
      enable_loopback(i);
}

std::unique_ptr<Deltacast::Device> Deltacast::Device::create(int device_index)
{
   auto device_handle = Helper::get_board_handle(device_index);
   if (!device_handle)
      return nullptr;

   return std::unique_ptr<Device>(new Device(device_index, std::move(device_handle)));
}

bool Deltacast::Device::suitable()
{
   ULONG number_of_on_board_keyers = 0;
   VHD_GetBoardCapability(*handle(), VHD_KEYER_BOARD_CAP_KEYER, &number_of_on_board_keyers);

   return (number_of_on_board_keyers > 0);
}

bool Deltacast::Device::set_loopback_state(int index, bool enabled)
{
   ULONG has_passive_loopback = FALSE;
   ULONG has_active_loopback = FALSE;
   ULONG has_firmware_loopback = FALSE;

   VHD_GetBoardCapability(*handle(), VHD_CORE_BOARD_CAP_PASSIVE_LOOPBACK, &has_passive_loopback);
   VHD_GetBoardCapability(*handle(), VHD_CORE_BOARD_CAP_ACTIVE_LOOPBACK, &has_active_loopback);
   VHD_GetBoardCapability(*handle(), VHD_CORE_BOARD_CAP_FIRMWARE_LOOPBACK, &has_firmware_loopback);

   if (has_firmware_loopback &&
       id_to_firmware_loopback_prop.find(index) != id_to_firmware_loopback_prop.end())
      return ApiSuccess{ VHD_SetBoardProperty(*handle(), id_to_firmware_loopback_prop.at(index),
                                              enabled) };
   else if (has_active_loopback &&
            id_to_active_loopback_prop.find(index) != id_to_active_loopback_prop.end())
      return ApiSuccess{ VHD_SetBoardProperty(*handle(), id_to_active_loopback_prop.at(index),
                                              enabled) };
   else if (has_passive_loopback &&
            id_to_passive_loopback_prop.find(index) != id_to_passive_loopback_prop.end())
      return ApiSuccess{ VHD_SetBoardProperty(*handle(), id_to_passive_loopback_prop.at(index),
                                              enabled) };
   return true;
}

void Deltacast::Device::enable_loopback(int index)
{
   set_loopback_state(index, true);
}

void Deltacast::Device::disable_loopback(int index)
{
   set_loopback_state(index, false);
}

bool Deltacast::Device::wait_for_incoming_signal(int                     rx_index,
                                                 const std::atomic_bool& stop_is_requested)
{
   if (id_to_rx_status_prop.find(rx_index) == id_to_rx_status_prop.end())
      return false;

   while (!stop_is_requested.load())
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      ULONG status = VHD_CORE_RXSTS_UNLOCKED;
      auto  api_success = ApiSuccess{
         VHD_GetBoardProperty(*handle(), id_to_rx_status_prop.at(rx_index), &status)
      };
      if (api_success && !(status & VHD_CORE_RXSTS_UNLOCKED))
         return true;
   }

   return false;
}

bool Deltacast::Device::wait_genlock_locked(const std::atomic_bool& stop_is_requested, std::unique_ptr<VideoMasterVideoInformation>& video_info)
{
    auto genlock_status_prop_optional = video_info->get_genlock_status_properties();
    if (!genlock_status_prop_optional.has_value())
        return false;

   while (!stop_is_requested.load())
   {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      ULONG status = VHD_SDI_GNLKSTS_NOREF | VHD_SDI_GNLKSTS_UNLOCKED;
      auto  api_success = ApiSuccess{ VHD_GetBoardProperty(*handle(), genlock_status_prop_optional.value(), &status) };
      if (api_success && !(status & VHD_SDI_GNLKSTS_NOREF) && !(status & VHD_SDI_GNLKSTS_UNLOCKED))
         return true;
   }

   return false;
}

bool Deltacast::Device::configure_genlock(int genlock_source_rx_index,
                                          std::unique_ptr<VideoMasterVideoInformation>& video_info)
{

   if (id_to_rx_genlock_source.find(genlock_source_rx_index) == id_to_rx_genlock_source.end())
      return false;

   ApiSuccess api_success;
   auto       genlock_src_property = video_info->get_genlock_source_properties();
   if (video_info->configure_genlock(handle(), genlock_source_rx_index))
      return true;
   return false;
}

std::unique_ptr<Deltacast::VideoMasterVideoInformation>
Deltacast::Device::get_video_information_for_channel(int index)
{
   std::unique_ptr<VideoMasterVideoInformation> _video_information = {};

   // identify the channel type to know which VideoInformation implementation to use
   auto channel_type_optional = get_channel_type(index, Direction::RX);
   if (!channel_type_optional.has_value())
      return {};

   ULONG channel_type = channel_type_optional.value();

   switch (channel_type)
   {
   case VHD_CHNTYPE_HDSDI:
   case VHD_CHNTYPE_3GSDI:
   case VHD_CHNTYPE_12GSDI:
      _video_information = std::make_unique<VideoMasterSdiVideoInformation>();
      break;
   case VHD_CHNTYPE_HDMI:
   case VHD_CHNTYPE_DISPLAYPORT:
      _video_information = std::make_unique<VideoMasterDvVideoInformation>();
      break;
   default:
      break;
   }

   return _video_information;
}

bool Deltacast::Device::configure_keyer(int rx_index, int tx_index)
{
   if ((id_to_keyer_rx_input.find(rx_index) == id_to_keyer_rx_input.end()) ||
       (id_to_keyer_tx_input.find(tx_index) == id_to_keyer_tx_input.end()) ||
       (id_to_keyer_rx_output.find(rx_index) == id_to_keyer_rx_output.end()) ||
       (id_to_keyer_video_output_prop.find(tx_index) == id_to_keyer_video_output_prop.end()) ||
       (id_to_keyer_anc_output_prop.find(tx_index) == id_to_keyer_anc_output_prop.end()))
      return false;

   ApiSuccess api_success;
   if (!(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_INPUT_A,
                                                        id_to_keyer_rx_input.at(rx_index)) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_INPUT_B,
                                                        id_to_keyer_tx_input.at(tx_index)) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_INPUT_K,
                                                        id_to_keyer_tx_input.at(tx_index)) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(
             *handle(), id_to_keyer_video_output_prop.at(tx_index), VHD_KOUTPUT_KEYER) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(),
                                                        id_to_keyer_anc_output_prop.at(tx_index),
                                                        id_to_keyer_rx_output.at(rx_index)) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_ALPHACLIP_MIN,
                                                        0) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_ALPHACLIP_MAX,
                                                        1020) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_ALPHABLEND_FACTOR,
                                                        1023) }) ||
       !(api_success = ApiSuccess{ VHD_SetBoardProperty(*handle(), VHD_KEYER_BP_ENABLE, TRUE) }))
   {
      std::cout << "ERROR: Cannot configure keyer (" << api_success << ")" << std::endl;
      return false;
   }

   return true;
}

std::optional<ULONG> Deltacast::Device::get_channel_type(int                          index,
                                                         Deltacast::Device::Direction direction)
{
   ULONG       _channel_type;
   const auto& id_to_channel_type_prop = direction == Direction::RX ? id_to_rx_channel_type_prop
                                                                    : id_to_tx_channel_type_prop;

   ApiSuccess api_success;
   api_success = VHD_GetBoardProperty(*handle(), id_to_channel_type_prop.at(index),
                                      (ULONG*)&_channel_type);

   if (!api_success)
   {
      std::cout << "ERROR: Cannot get channel type (" << api_success << ")" << std::endl;
      return {};
   }

   return _channel_type;
}

namespace Deltacast
{
std::ostream& operator<<(std::ostream& os, const Device& device)
{
   ULONG driver_version = 0, firmware_version, number_of_rx_channels, number_of_tx_channels;

   VHD_GetBoardProperty(**(device._device_handle), VHD_CORE_BP_DRIVER_VERSION, &driver_version);
   VHD_GetBoardProperty(**(device._device_handle), VHD_CORE_BP_FIRMWARE_VERSION, &firmware_version);
   VHD_GetBoardProperty(**(device._device_handle), VHD_CORE_BP_NB_RXCHANNELS,
                        &number_of_rx_channels);
   VHD_GetBoardProperty(**(device._device_handle), VHD_CORE_BP_NB_TXCHANNELS,
                        &number_of_tx_channels);

   char pcie_id_string[64];
   VHD_GetPCIeIdentificationString(device._device_index, pcie_id_string);

   os << "  Board " << device._device_index << ":  [ " << VHD_GetBoardModel(device._device_index)
      << " ]"
      << "\n";
   os << "    - PCIe Id string: " << pcie_id_string << "\n";
   os << "    - Driver v" << ((driver_version & 0xFF000000) >> 24) << "."
      << ((driver_version & 0x00FF0000) >> 16) << "." << ((driver_version & 0x0000FF00) >> 8) << "."
      << ((driver_version & 0x000000FF) >> 0) << "\n";

   os << std::hex;

   os << "    - Board fpga firmware v" << ((firmware_version & 0xFF000000) >> 24) << "."
      << ((firmware_version & 0x00FF0000) >> 16) << "." << ((firmware_version & 0x0000FF00) >> 8)
      << "." << ((firmware_version & 0x000000FF) >> 0) << "\n";

   os << std::dec;

   os << "    - " << number_of_rx_channels << " In / " << number_of_tx_channels << " Out"
      << "\n";

   return os;
}
}  // namespace Deltacast