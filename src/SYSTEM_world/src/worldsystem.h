
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
#include "system.h"
#include "tvector.h"
#include "eventsource.h"

namespace mage
{
    namespace core { class Entity; }
    namespace core { class Entitygraph; }

    enum class WorldSystemEvent
    {
        WORLD_POSITION_UPDATED
    };
   
    class WorldSystem : public core::System, public mage::property::EventSource<WorldSystemEvent, const std::string&>
    {
    public:
        WorldSystem() = delete;
        WorldSystem(core::Entitygraph& p_entitygraph);
        ~WorldSystem() = default;

        void run();
    private:

        void inline position_delta_event(core::Entity* p_entity, const core::maths::Real3Vector& p_vA, const core::maths::Real3Vector& p_vB);
    };
}
