
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
#include <queue>
#include <unordered_set>

#include "system.h"

namespace mage
{
    namespace core 
    {
        class Entity; 
        class Entitygraph;
        class ComponentContainer;

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

        void compute_entity(core::Entity* p_entity, const core:: ComponentContainer& p_world_components);

        std::queue<core::Entity*> m_newly_added_entities;

        std::unordered_set<core::Entity*>  m_entities_to_compute_distance;
        std::unordered_set<core::Entity*>  m_entities_to_compute_2d_pos;

        std::unordered_set<core::Entity*>  m_entities_to_compute;

    };
}
