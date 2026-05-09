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

#include <chrono>
#include <string>

#include "sysengine.h"

#include "datacloud.h"

using namespace mage::core;


SystemEngine::SystemEngine()
{
	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	dataCloud->registerData<std::string>("mage.timings.systemengine");
}

void SystemEngine::run()
{
	const auto start_time{ std::chrono::high_resolution_clock::now() };

	for (auto& system : m_systems)
	{
		system.second.get()->run();
	}

	const auto end_time{ std::chrono::high_resolution_clock::now() };
	const auto duration{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time) };

	// estimated fps
	const int estimated_fps = 1000 / duration.count();

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	dataCloud->updateDataValue<std::string>("mage.timings.systemengine", std::to_string(duration.count()) + " ms, estimated fps = " + std::to_string(estimated_fps));
}

System* SystemEngine::getSystem(int p_executionslot) const
{
	if (m_systems.count(p_executionslot))
	{
		return m_systems.at(p_executionslot).get();
	}
	else
	{
		_EXCEPTION("unknow system slot");
	}
}