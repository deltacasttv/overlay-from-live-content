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

#include "helper.hpp"

#include <thread>
#include <utility>
#include <optional>
#include <VideoMasterCppApi/to_string.hpp>
#include <VideoMasterCppApi/exception.hpp>
#include <VideoMasterCppApi/to_string.hpp>
#include <VideoMasterCppApi/helper/sdi.hpp>

template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

using namespace Deltacast::Wrapper;
using namespace Deltacast::Wrapper::Helper;

std::ostream& operator<<(std::ostream& os, Board& board)
{
    os << "\t" << "Board " << board.index() << ":  [ " << board.name() << " ]" << std::endl;
    os << "\t" << "\t" << "- " << board.number_of_rx() << " RX / " << board.number_of_tx() << " TX" << std::endl;
    os << "\t" << "\t" << "- Driver: " << board.driver_version() << std::endl;
    os << "\t" << "\t" << "- PCIe ID: " << board.pcie_identifier() << std::endl;
    os << "\t" << "\t" << "- SN: " << board.serial_number() << std::endl;
    auto [ pcie_bus, number_of_lanes ] = board.pcie();
    os << "\t" << "\t" << "- " << to_pretty_string(pcie_bus) << ", " << number_of_lanes << " lanes" << std::endl;

    os << std::hex;
    os << "\t" << "\t" << "- Firmware: 0x" << board.fpga().version() << std::endl;
    if (board.has_scp())
        os << "\t" << "\t" << "- SCP: 0x" << board.scp().version() << std::endl;
    os << std::dec;

    return os;
}

namespace Application::Helper
{
    std::optional<std::reference_wrapper<BoardComponents::Loopback>> get_loopback(Board& board, unsigned int channel_index)
    {
        try { return board.firmware_loopback(channel_index); } catch (const UnavailableResource& e) { }
        try { return board.active_loopback(channel_index); } catch (const UnavailableResource& e) { }
        try { return board.passive_loopback(channel_index); } catch (const UnavailableResource& e) { }
        return std::nullopt;
    }

    void enable_loopback(Board& board, unsigned int channel_index)
    {
        auto optional_loopback = get_loopback(board, channel_index);
        if (optional_loopback)
            optional_loopback.value().get().enable();
    }

    void disable_loopback(Board& board, unsigned int channel_index)
    {
        auto optional_loopback = get_loopback(board, channel_index);
        if (optional_loopback)
            optional_loopback.value().get().disable();
    }

    VHD_STREAMTYPE rx_index_to_streamtype(unsigned int rx_index)
    {
        switch (rx_index)
        {
        case 0: return VHD_ST_RX0;
        case 1: return VHD_ST_RX1;
        case 2: return VHD_ST_RX2;
        case 3: return VHD_ST_RX3;
        case 4: return VHD_ST_RX4;
        case 5: return VHD_ST_RX5;
        case 6: return VHD_ST_RX6;
        case 7: return VHD_ST_RX7;
        case 8: return VHD_ST_RX8;
        case 9: return VHD_ST_RX9;
        case 10: return VHD_ST_RX10;
        case 11: return VHD_ST_RX11;
        default:
            throw std::invalid_argument("Invalid RX index");
        }
    }

    VHD_STREAMTYPE tx_index_to_streamtype(unsigned int tx_index)
    {
        switch (tx_index)
        {
        case 0: return VHD_ST_TX0;
        case 1: return VHD_ST_TX1;
        case 2: return VHD_ST_TX2;
        case 3: return VHD_ST_TX3;
        case 4: return VHD_ST_TX4;
        case 5: return VHD_ST_TX5;
        case 6: return VHD_ST_TX6;
        case 7: return VHD_ST_TX7;
        case 8: return VHD_ST_TX8;
        case 9: return VHD_ST_TX9;
        case 10: return VHD_ST_TX10;
        case 11: return VHD_ST_TX11;
        default:
            throw std::invalid_argument("Invalid TX index");
        }
    }

    VHD_KEYERINPUT rx_to_keyer_input(unsigned int rx_index)
    {
        switch (rx_index)
        {
        case 0: return VHD_KINPUT_RX0;
        case 1: return VHD_KINPUT_RX1;
        case 2: return VHD_KINPUT_RX2;
        case 3: return VHD_KINPUT_RX3;
        default:
            throw std::invalid_argument("Invalid RX index");
        }
    }

    VHD_KEYERINPUT tx_to_keyer_input(unsigned int tx_index)
    {
        switch (tx_index)
        {
        case 0: return VHD_KINPUT_TX0;
        case 1: return VHD_KINPUT_TX1;
        case 2: return VHD_KINPUT_TX2;
        case 3: return VHD_KINPUT_TX3;
        default:
            throw std::invalid_argument("Invalid TX index");
        }
    }

    VHD_KEYEROUTPUT rx_to_keyer_output(unsigned int rx_index)
    {
        switch (rx_index)
        {
        case 0: return VHD_KOUTPUT_RX0;
        case 1: return VHD_KOUTPUT_RX1;
        case 2: return VHD_KOUTPUT_RX2;
        case 3: return VHD_KOUTPUT_RX3;
        default:
            throw std::invalid_argument("Invalid RX index");
        }
    }

    VHD_CHANNELTYPE stream_type_to_channel_type(Board& board, VHD_STREAMTYPE stream_type)
    {
        switch (stream_type)
        {
            case VHD_ST_RX0: return board.rx(0).type();
            case VHD_ST_RX1: return board.rx(1).type();
            case VHD_ST_RX2: return board.rx(2).type();
            case VHD_ST_RX3: return board.rx(3).type();
            case VHD_ST_RX4: return board.rx(4).type();
            case VHD_ST_RX5: return board.rx(5).type();
            case VHD_ST_RX6: return board.rx(6).type();
            case VHD_ST_RX7: return board.rx(7).type();
            case VHD_ST_RX8: return board.rx(8).type();
            case VHD_ST_RX9: return board.rx(9).type();
            case VHD_ST_RX10: return board.rx(10).type();
            case VHD_ST_RX11: return board.rx(11).type();
            case VHD_ST_TX0: return board.tx(0).type();
            case VHD_ST_TX1: return board.tx(1).type();
            case VHD_ST_TX2: return board.tx(2).type();
            case VHD_ST_TX3: return board.tx(3).type();
            case VHD_ST_TX4: return board.tx(4).type();
            case VHD_ST_TX5: return board.tx(5).type();
            case VHD_ST_TX6: return board.tx(6).type();
            case VHD_ST_TX7: return board.tx(7).type();
            case VHD_ST_TX8: return board.tx(8).type();
            case VHD_ST_TX9: return board.tx(9).type();
            case VHD_ST_TX10: return board.tx(10).type();
            case VHD_ST_TX11: return board.tx(11).type();
            default:
                throw std::invalid_argument("Invalid stream type");
        }
    }

    bool wait_for_input(BoardComponents::RxConnector& rx_connector, const std::atomic_bool& stop_is_requested)
    {
        while (!stop_is_requested && !rx_connector.signal_present())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return rx_connector.signal_present();
    }

    bool wait_for_genlock(Deltacast::Wrapper::BoardComponents::SdiComponents::Genlock& genlock, const std::atomic_bool& stop_is_requested)
    {
        while (!stop_is_requested && !genlock.locked())
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

        return genlock.locked();
    }

    TechStream open_stream(Board& board, VHD_STREAMTYPE stream_type)
    {
        auto channel_type = stream_type_to_channel_type(board, stream_type);
        switch (channel_type)
        {
            case VHD_CHNTYPE_HDSDI:
            case VHD_CHNTYPE_3GSDI:
            case VHD_CHNTYPE_12GSDI:
                return std::move(board.sdi().open_stream(stream_type, VHD_SDI_STPROC_DISJOINED_VIDEO));
            case VHD_CHNTYPE_HDMI:
            case VHD_CHNTYPE_DISPLAYPORT:
                return std::move(board.dv().open_stream(stream_type, VHD_DV_STPROC_DISJOINED_VIDEO));
            default:
                throw std::invalid_argument("Invalid channel type");
        }
    }

    Stream& to_base_stream(TechStream& stream)
    {
        return std::visit(overloaded{
            [](SdiStream& sdi_stream) -> Stream& { return sdi_stream; },
            [](DvStream& dv_stream) -> Stream& { return dv_stream; }
        }, stream);
    }

    void configure_stream(TechStream& stream, const SignalInformation& signal_information)
    {
        std::visit(overloaded{
            [&signal_information](SdiStream& sdi_stream)
            {
                const SdiSignalInformation& sdi_signal_information = std::get<SdiSignalInformation>(signal_information);
                sdi_stream.set_video_standard(sdi_signal_information.video_standard);
                sdi_stream.set_interface(sdi_signal_information.video_interface);
            },
            [&signal_information](DvStream& dv_stream)
            {
                const DvSignalInformation& dv_signal_information = std::get<DvSignalInformation>(signal_information);
                dv_stream.set_active_width(dv_signal_information.width);
                dv_stream.set_active_height(dv_signal_information.height);
                dv_signal_information.progressive ? dv_stream.set_progressive() : dv_stream.set_interlaced();
                dv_stream.set_frame_rate(dv_signal_information.framerate);
                dv_stream.set_cable_color_space(dv_signal_information.cable_color_space);
                if (dv_stream.is_tx(dv_stream.type()))
                    dv_stream.set_cable_sampling(dv_signal_information.cable_sampling);
            }
        }, stream);
    }

    void print_information(const SignalInformation& signal_information, const std::string& prefix /*= ""*/)
    {
        std::visit(overloaded{
            [&prefix](const SdiSignalInformation& sdi_signal_info)
            {
                std::cout << prefix << "Video standard: " << to_pretty_string(sdi_signal_info.video_standard) << std::endl;
                std::cout << prefix << "Clock divisor: " << to_pretty_string(sdi_signal_info.clock_divisor) << std::endl;
                std::cout << prefix << "Interface: " << to_pretty_string(sdi_signal_info.video_interface) << std::endl;
            },
            [&prefix](const DvSignalInformation& dv_signal_info)
            {
                std::cout << prefix << dv_signal_info.width << "x" << dv_signal_info.height 
                                    << (dv_signal_info.progressive ? "p" : "i") 
                                    << dv_signal_info.framerate << std::endl;
                std::cout << prefix << to_pretty_string(dv_signal_info.cable_color_space) << std::endl;
                std::cout << prefix << to_pretty_string(dv_signal_info.cable_sampling) << std::endl;
            }
        }, signal_information);
    }

    SignalInformation detect_information(TechStream& stream)
    {
        return std::visit(overloaded{
            [](SdiStream& sdi_stream) -> SignalInformation
            {
                return SdiSignalInformation{sdi_stream.video_standard(), sdi_stream.clock_divisor(), sdi_stream.interface()};
            },
            [](DvStream& dv_stream) -> SignalInformation
            {
                return DvSignalInformation{dv_stream.active_width(), dv_stream.active_height(), !dv_stream.interlaced(), dv_stream.frame_rate()
                                            , dv_stream.cable_color_space(), dv_stream.cable_sampling()};
            }
        }, stream);
    }

    VideoCharacteristics get_video_characteristics(const SignalInformation& signal_information)
    {
        return std::visit(overloaded{
            [](const SdiSignalInformation& sdi_signal_info) -> VideoCharacteristics
            {
                return Sdi::video_standard_to_characteristics(sdi_signal_info.video_standard);
            },
            [](const DvSignalInformation& dv_signal_info) -> VideoCharacteristics
            {
                return { dv_signal_info.width, dv_signal_info.height, !dv_signal_info.progressive, dv_signal_info.framerate };
            }
        }, signal_information);
    }
}