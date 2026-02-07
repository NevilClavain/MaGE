
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

#include "openenv.h"
#include <string>

#include "maths_helpers.h"
#include "shadows_helpers.h"

#include "aspects.h"
#include "datacloud.h"


using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void OpenEnv::run(void)
{
	Base::run();

	/////////////////////////////////////////////////////

	// update all shadowmaps camera pos/direction

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	const auto light_cartesian4{ dataCloud->readDataValue<maths::Real4Vector>("std.light0.dir") };

	maths::Real3Vector light_cartesian3(light_cartesian4[0], light_cartesian4[1], light_cartesian4[2]);


	for (const auto& e : m_shadowmap_joints_list)
	{
		const auto& camera_joint_id{ e.first };
		const auto& base_vector{ e.second };

		mage::helpers::updateShadowMapDirection(m_entitygraph.node(camera_joint_id).data(), light_cartesian3, base_vector, 400);
	}
}