
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

#include "shadows_helpers.h"

#include "entitygraph.h"
#include "entity.h"
#include "aspects.h"

void mage::helpers::updateShadowMapDirection(mage::core::Entity* p_shadowmap_lookatJoint_Entity, 
											const mage::core::maths::Real4Vector& p_light_vector, 
											const mage::core::maths::Real4Vector& p_base_vector, 
											double p_vectorscale)
{	
	auto light_cartesian{ p_light_vector };
	light_cartesian.normalize();
	light_cartesian.scale(p_vectorscale);

	auto& lookat_world_aspect{ p_shadowmap_lookatJoint_Entity->aspectAccess(core::worldAspect::id) };

	core::maths::Real3Vector& lookat_localpos{ lookat_world_aspect.getComponent<core::maths::Real3Vector>("lookat_localpos")->getPurpose() };

	lookat_localpos[0] = -light_cartesian[0] + p_base_vector[0];
	lookat_localpos[1] = -light_cartesian[1] + p_base_vector[1];
	lookat_localpos[2] = -light_cartesian[2] + p_base_vector[2];
}