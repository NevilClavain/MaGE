
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

#include "system.h"

namespace mage
{
    namespace core 
    {
        class Entity; 
        class Entitygraph;

        namespace maths
        {
            class Matrix;
        }
    }


    class WorldSystem : public core::System
    {
    public:
        WorldSystem() = delete;
        WorldSystem(core::Entitygraph& p_entitygraph);
        ~WorldSystem() = default;

        void run();

    private:

        void extractProjAndViewFromRenderingQueue(const std::string& p_current_view_entity_id, mage::core::maths::Matrix& p_current_view, mage::core::maths::Matrix& p_current_proj);

    };
}
