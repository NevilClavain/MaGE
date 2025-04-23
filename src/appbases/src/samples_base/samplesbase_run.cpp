
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

#include "samplesbase.h"
#include <string>

#include "maths_helpers.h"

#include "aspects.h"
#include "datacloud.h"
#include "sysengine.h"

#include "linemeshe.h"
#include "trianglemeshe.h"
#include "renderstate.h"

#include "syncvariable.h"

#include "worldposition.h"
#include "animatorfunc.h"
#include "animators_helpers.h"

#include "resourcesystem.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void SamplesBase::run(void)
{
	/////////////////////////////////////////////////////

	auto sysEngine{ SystemEngine::getInstance() };
	sysEngine->run();

	////////////////////////////////////////////////////////
	// loading gear
	{
		const auto resourceSystem{ sysEngine->getSystem<mage::ResourceSystem>(resourceSystemSlot) };

		const auto& rendering_aspect{ m_loading_gear->aspectAccess(renderingAspect::id) };
		rendering::DrawingControl& dc{ rendering_aspect.getComponent<rendering::DrawingControl>("sprite2D_dc")->getPurpose() };

		if (resourceSystem->getNbBusyRunners() > 0)
		{
			dc.draw = true;

			const auto& time_aspect{ m_loading_gear->aspectAccess(timeAspect::id) };
			core::SyncVariable& z_rot{ time_aspect.getComponent<SyncVariable>("z_rot")->getPurpose() };

			z_rot.step = 2 * core::maths::pi * 0.25;
			z_rot.direction = SyncVariable::Direction::INC;
		}
		else
		{
			dc.draw = false;
		}
	}	
}