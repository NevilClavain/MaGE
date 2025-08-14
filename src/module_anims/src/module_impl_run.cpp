
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

#include "module_impl.h"
#include <string>

#include "aspects.h"
#include "datacloud.h"
#include "sysengine.h"



#include "maths_helpers.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void ModuleImpl::run(void)
{
	OpenEnv::run();

	auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	
	/////////////////////////////////////////////////////

	{
		const auto raptorEntity{ m_raptorEntity };
		const auto& animations_aspect{ raptorEntity->aspectAccess(core::animationsAspect::id) };

		const auto currentAnimationId{ animations_aspect.getComponent<std::string>("eg.std.currentAnimationId")->getPurpose() };

		const auto currentAnimationTicksDuration{ animations_aspect.getComponent<double>("eg.std.currentAnimationTicksDuration")->getPurpose() };
		const auto currentAnimationSecondsDuration{ animations_aspect.getComponent<double>("eg.std.currentAnimationSecondsDuration")->getPurpose() };
		const auto currentAnimationTicksProgress{ animations_aspect.getComponent<double>("eg.std.currentAnimationTicksProgress")->getPurpose() };
		const auto currentAnimationSecondsProgress{ animations_aspect.getComponent<double>("eg.std.currentAnimationSecondsProgress")->getPurpose() };

		
		dataCloud->updateDataValue<std::string>("current_animation.id", currentAnimationId);

		dataCloud->updateDataValue<double>("current_animation.ticks_duration", currentAnimationTicksDuration);
		dataCloud->updateDataValue<double>("current_animation.ticks_progress", currentAnimationTicksProgress);

		dataCloud->updateDataValue<double>("current_animation.seconds_duration", currentAnimationSecondsDuration);
		dataCloud->updateDataValue<double>("current_animation.seconds_progress", currentAnimationSecondsProgress);
	}
	
	//////////////////////////////////////////////////////

	{
		// compute directional light cartesian coords

		mage::core::maths::Real4Vector light_spherical { m_light_ray, m_light_theta, m_light_phi, 1.0 };
		mage::core::maths::Real4Vector light_cartesian;

		mage::core::maths::sphericaltoCartesian(light_spherical, light_cartesian);

		dataCloud->updateDataValue<maths::Real4Vector>("std.light0.dir", light_cartesian);

	}
}