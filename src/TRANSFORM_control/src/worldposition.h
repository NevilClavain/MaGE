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
#include <memory>

#include "matrix.h"


namespace mage
{
	namespace transform
	{
        struct WorldPosition
        {
            WorldPosition()
            {
                local_pos.identity();
                global_pos->identity();
            }

            WorldPosition(const core::maths::Matrix& p_local)
            {
                local_pos = p_local;
            }

            ~WorldPosition() = default;

            enum class TransformationComposition
            {
                TRANSFORMATION_RELATIVE_FROM_PARENT,
                TRANSFORMATION_ABSOLUTE,
                TRANSFORMATION_PARENT_PROJECTEDPOS 
            };

            //////////
            core::maths::Matrix local_pos;

			std::shared_ptr<core::maths::Matrix> global_pos{ std::make_shared<core::maths::Matrix>() };
			bool				                globalpos_is_valid{ false }; // was computed at least once
            //////////

            bool                wp_projected_z_neg{ false };

            

            TransformationComposition composition_operation{ TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT };
        };
	}
}
