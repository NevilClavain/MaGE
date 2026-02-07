
/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2026
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

#include <string>
#include <vector>
#include <utility>

#include "maths_helpers.h"

#include "base.h"

namespace mage
{
    class StreamedOpenEnv : public Base
    {
    public:

        StreamedOpenEnv() = default;
        ~StreamedOpenEnv() = default;

        StreamedOpenEnv(const StreamedOpenEnv&) = delete;
        StreamedOpenEnv(StreamedOpenEnv&&) = delete;
        StreamedOpenEnv& operator=(const StreamedOpenEnv& t) = delete;

        void                            init(const std::string p_appWindowsEntityName);
        void                            run(void);

    protected:
       

        mage::core::maths::Matrix       m_perpective_projection;
        mage::core::maths::Matrix       m_orthogonal_projection;

        void                            d3d11_system_events_openenv();

    };
}