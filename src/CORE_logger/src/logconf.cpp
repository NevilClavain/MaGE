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

void logger::Configuration::applyConfiguration(const std::string& p_jsondata)
{
    json::Logconf logconf;
    JS::ParseContext parseContext(p_jsondata);

    const auto parse_status{ parseContext.parseTo(logconf) };

    if (parse_status != JS::Error::NoError)
    {
        _EXCEPTION("Cannot parse logging configuration")
    }

    for (const auto& output : logconf.outputs)
    {
        if ("file" == output.type)
        {
            m_outputs[output.id] = std::make_unique<OutputFile>(output.path);
            const auto& of{ m_outputs.at(output.id) };
            of.get()->setFlushPeriod(0);
        }
        // no other type of output for now
    }

    for (const auto& logger : logconf.loggers)
    {
        const auto output{ m_outputs.at(logger.output).get() };

        static const std::unordered_map<std::string, Sink::Level> levelTranslation
        {
            { "FATAL", Sink::Level::LEVEL_FATAL},
            { "ERROR", Sink::Level::LEVEL_ERROR},
            { "WARN", Sink::Level::LEVEL_WARN},
            { "DEBUG", Sink::Level::LEVEL_DEBUG},
            { "TRACE", Sink::Level::LEVEL_TRACE},
        };
        const auto logger_level = levelTranslation.at(logger.level);

        static const std::unordered_map<std::string, bool> stateTranslation
        {
            { "on", true},
            { "off", false},
        };

        const auto logger_state = stateTranslation.at(logger.state);

        if (m_sinks_infos.count(logger.source))
        {
            auto& sink_info{ m_sinks_infos.at(logger.source) };

            const auto sink{ std::get<0>(sink_info) };

            sink->setCurrentLevel(logger_level);
            sink->setState(logger_state);
            sink->registerOutput(output);

            std::get<1>(sink_info) = logger_state;
            std::get<2>(sink_info) = logger_level;
            std::get<3>(sink_info) = output;
        }
        else
        {
            m_sinks_infos[logger.source] = std::make_tuple(nullptr, logger_state, logger_level, output);
        }
    }
}