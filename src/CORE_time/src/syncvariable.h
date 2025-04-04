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

namespace mage
{
	namespace core
	{
        struct SyncVariable
        {
        public:
            enum class Type
            {
                ANGLE,
                POSITION
            };

            enum class Direction
            {
                INC,
                DEC,
                ZERO
            };

            enum class BoundariesManagement
            {
                STOP,
                MIRROR,
                WRAP
            };

            enum class State
            {
                ON,
                OFF
            };

            struct Boundaries
            {
                double  min;
                double  max;
            };

            SyncVariable() = default;
            ~SyncVariable() = default;

            SyncVariable(Type p_type, double p_step, Direction p_direction = Direction::INC, 
                            double p_initial_value = 0.0, Boundaries p_boundaries = { NAN, NAN }, 
                            BoundariesManagement p_boundariesManagement = { BoundariesManagement::STOP } ) :

                type(p_type),
                step(p_step),
                direction(p_direction),
                value(p_initial_value),
                boundaries(p_boundaries),
                boundaries_management(p_boundariesManagement)
            {
            };

            double                  value{ 0.0 };
            double                  step{ 0.0 };

            State                   state{ State::ON };

            Direction               direction{ Direction::INC };
            Type                    type;
            Boundaries              boundaries{ NAN, NAN };
            BoundariesManagement    boundaries_management{ BoundariesManagement::STOP };
            
        };
	}
}