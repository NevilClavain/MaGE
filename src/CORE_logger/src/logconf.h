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

#pragma once

#include <windows.h>
#include <json_struct/json_struct.h>

#include <string>
#include <unordered_map>
#include <tuple>
#include <vector>

#include "singleton.h"
#include "logsink.h"
#include "json.h"

namespace mage
{
    namespace core
    {
        namespace logger
        {
            namespace json
            {
                struct Output
                {
                    std::string type;
                    std::string id;
                    std::string path;

                    JS_OBJ(type, id, path);
                };

                struct Logger
                { 
                    std::string source;
                    std::string level;
                    std::string state;
                    std::string output;

                    JS_OBJ(source, level, state, output);
                };

                struct Logconf
                {
                    std::vector<Output> outputs;
                    std::vector<Logger> loggers;

                    JS_OBJ(outputs, loggers);
                };
            }


            // fwd decl
            class Output;

            class Configuration : public property::Singleton<Configuration>
            {
            public:
                Configuration(void);
                ~Configuration(void) = default;

                void                    registerSink(Sink* p_sink);
                LONGLONG                getLastTick(void) const;

                /*
                Json<>::Callback getCallback() const
                {
                    return m_cb;
                }
                */

                void applyConfiguration(const std::string& p_jsondata);

            private:

                //Json<>::Callback	                                                m_cb;

                std::unordered_map<std::string, std::unique_ptr<Output>>        m_outputs;

                using SinkInfos = std::tuple<Sink*, bool, Sink::Level, Output*>;
                std::unordered_map<std::string, SinkInfos>                      m_sinks_infos;

                DWORD                                                           m_baseTick;

                //// JSON parsing working members

                enum class ParsingState
                {
                    IDLE,
                    RECORD_CONFIG,
                    RECORD_LOGGER
                };

                //ParsingState                                        m_parsing_state{ ParsingState::IDLE };

                //std::string                                         m_mem_output_type;
                //std::string                                         m_mem_output_id;
                //std::string                                         m_mem_output_path;

                //std::string                                         m_mem_logger_source;
                //Sink::Level                                         m_mem_logger_level;
                //bool                                                m_mem_logger_state;
                //std::string                                         m_mem_logger_output;
            };
        }
    }
}

