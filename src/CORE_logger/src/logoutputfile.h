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

#include <mutex>

#include "logoutput.h"
#include "file.h"

namespace mage
{
    namespace core
    {
        namespace logger
        {
            class OutputFile : public Output
            {
            public:

                OutputFile(const std::string& p_filename);

                OutputFile() = delete;
                OutputFile(const OutputFile&) = delete;
                OutputFile(OutputFile&&) = delete;
                OutputFile& operator=(const OutputFile& t) = delete;

                ~OutputFile(void) = default;

                void logIt(const std::string& p_trace);                
                void setFlushPeriod(long p_period);

            private:

                std::unique_ptr<File>                       m_file;
                long                                        m_flush_period{ 0 };
                long                                        m_period_count{ 0 };
                std::mutex	                                m_mutex;
            };
        }
    }
}

