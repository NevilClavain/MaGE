
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

#include "samplesopenenv.h"
#include <string>

#include "maths_helpers.h"

#include "aspects.h"
#include "datacloud.h"


using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void SamplesOpenEnv::run(void)
{
	SamplesBase::run();

	/////////////////////////////////////////////////////

	// update shadowmap camera pos/direction

	const auto shadowmap_lookatJoint_Entity{ m_entitygraph.node("shadowmap_lookatJoint_Entity").data() };

	auto& lookat_world_aspect{ shadowmap_lookatJoint_Entity->aspectAccess(core::worldAspect::id) };

	mage::core::maths::Real4Vector light_cartesian;

	auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	light_cartesian = dataCloud->readDataValue<maths::Real4Vector>("std.light0.dir");

	light_cartesian.normalize();
	light_cartesian.scale(400.0);

	

	core::maths::Real3Vector& lookat_localpos{ lookat_world_aspect.getComponent<core::maths::Real3Vector>("lookat_localpos")->getPurpose() };
	lookat_localpos[0] = -light_cartesian[0];
	lookat_localpos[1] = (-light_cartesian[1] + skydomeInnerRadius + groundLevel);
	lookat_localpos[2] = -light_cartesian[2];
}