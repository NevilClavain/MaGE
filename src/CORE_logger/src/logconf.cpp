/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2025
*
* This file is part of MaGE.
*
*    MaGE is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    MaGE is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with MaGE.  If not, see <http://www.gnu.org/licenses/>.
*
*/
/* -*-LIC_END-*- */

#include <unordered_map>
#include "logconf.h"

#include "logoutput.h"
#include "logoutputfile.h"
#include "exceptions.h"

using namespace mage::core;

logger::Configuration::Configuration( void )
{
    m_baseTick = ::GetTickCount();

    m_cb = [&, this](JSONEvent p_event, const std::string& p_id, int p_index, const std::string& p_value, const std::optional<mage::core::DefaultUserData*>&)
    {
        switch (p_event)
        {
            case mage::core::JSONEvent::OBJECT_BEGIN:

                if ("outputs" == p_id)
                {
                    this->m_parsing_state = ParsingState::RECORD_CONFIG;
                }
                else if ("loggers" == p_id)
                {
                    this->m_parsing_state = ParsingState::RECORD_LOGGER;
                }
                break;

            case mage::core::JSONEvent::OBJECT_END:

                if ("outputs" == p_id)
                {
                    // create the output
                    if ("file" == m_mem_output_type)
                    {
                        // file output

                        this->m_outputs[m_mem_output_id] = std::make_unique<OutputFile>(m_mem_output_path);
                        const auto& of{ this->m_outputs[m_mem_output_id] };
                        of.get()->setFlushPeriod(0);
                    }
                    // no other type of output for now

                }
                else if ("loggers" == p_id)
                {
                    const auto output{ this->m_outputs.at(m_mem_logger_output).get() };

                    if (this->m_sinks_infos.count(this->m_mem_logger_source))
                    {
                        auto& sink_info{ this->m_sinks_infos[this->m_mem_logger_source] };

                        const auto sink{ std::get<0>(this->m_sinks_infos.at(this->m_mem_logger_source)) };

                        sink->setCurrentLevel(m_mem_logger_level);
                        sink->setState(m_mem_logger_state);
                        sink->registerOutput(output);

                        std::get<1>(sink_info) = m_mem_logger_state;
                        std::get<2>(sink_info) = m_mem_logger_level;
                        std::get<3>(sink_info) = output;
                    }
                    else
                    {
                        this->m_sinks_infos[this->m_mem_logger_source] = std::make_tuple(nullptr, m_mem_logger_state, m_mem_logger_level, output);
                    }
                }
                break;

                /* not ussed
                case mage::core::json::Event::ARRAY_BEGIN:

                    break;

                case mage::core::json::Event::ARRAY_END:

                    break;
                */

            case mage::core::JSONEvent::STRING:

                if (ParsingState::RECORD_CONFIG == this->m_parsing_state)
                {
                    if ("type" == p_id)
                    {
                        m_mem_output_type = p_value;
                    }
                    else if ("id" == p_id)
                    {
                        m_mem_output_id = p_value;
                    }
                    else if ("path" == p_id)
                    {
                        m_mem_output_path = p_value;
                    }
                }
                else if (ParsingState::RECORD_LOGGER == this->m_parsing_state)
                {
                    if ("source" == p_id)
                    {
                        m_mem_logger_source = p_value;
                    }
                    else if ("level" == p_id)
                    {
                        static const std::unordered_map<std::string, Sink::Level> levelTranslation
                        {
                            { "FATAL", Sink::Level::LEVEL_FATAL},
                            { "ERROR", Sink::Level::LEVEL_ERROR},
                            { "WARN", Sink::Level::LEVEL_WARN},
                            { "DEBUG", Sink::Level::LEVEL_DEBUG},
                            { "TRACE", Sink::Level::LEVEL_TRACE},
                        };

                        m_mem_logger_level = levelTranslation.at(p_value);
                    }
                    else if ("state" == p_id)
                    {
                        static const std::unordered_map<std::string, bool> stateTranslation
                        {
                            { "on", true},
                            { "off", false},
                        };

                        m_mem_logger_state = stateTranslation.at(p_value);
                    }
                    else if ("output" == p_id)
                    {
                        m_mem_logger_output = p_value;
                    }
                }
                break;

                /* not used
                    case mage::core::json::Event::PRIMITIVE:
                        break;
                */
        }


    };
}



void logger::Configuration::registerSink( Sink* p_sink )
{
    const auto name{ p_sink->getName() };
    if (m_sinks_infos.count(name) > 0)
    {
        // entry exists
        p_sink->setCurrentLevel(std::get<2>(m_sinks_infos.at(name)));
        p_sink->setState(std::get<1>(m_sinks_infos.at(name)));
        p_sink->registerOutput(std::get<3>(m_sinks_infos.at(name)));
    }
    else
    {
        // entry does not exists, create it
        m_sinks_infos[name] = std::make_tuple(p_sink, false, Sink::Level::LEVEL_FATAL, nullptr);
    }
}

LONGLONG logger::Configuration::getLastTick( void ) const
{


    return ::GetTickCount() - m_baseTick;
}
