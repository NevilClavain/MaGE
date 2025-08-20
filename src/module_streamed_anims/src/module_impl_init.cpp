
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
#include <vector>
#include <unordered_map>
#include <utility>
#include <list>

#include <chrono>

#include "aspects.h"

#include "d3d11system.h"
#include "timesystem.h"
#include "resourcesystem.h"
#include "worldsystem.h"
#include "dataprintsystem.h"
#include "renderingqueuesystem.h"
#include "animationssystem.h"

#include "sysengine.h"
#include "filesystem.h"
#include "logconf.h"

#include "logger_service.h"

#include "worldposition.h"
#include "animatorfunc.h"

#include "trianglemeshe.h"
#include "renderstate.h"

#include "animationbone.h"
#include "scenenode.h"


#include "syncvariable.h"
#include "animators_helpers.h"

#include "datacloud.h"

#include "shaders_service.h"
#include "textures_service.h"

#include "entitygraph_helpers.h"
#include "renderingpasses_helpers.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void ModuleImpl::init(const std::string p_appWindowsEntityName)
{
	StreamedOpenEnv::init(p_appWindowsEntityName);

	/////////// logging conf

	mage::core::FileContent<char> logConfFileContent("./module_streamed_anims_config/logconf.json");
	logConfFileContent.load();

	const auto dataSize{ logConfFileContent.getDataSize() };
	const std::string data(logConfFileContent.getData(), dataSize);

	logger::Configuration::getInstance()->applyConfiguration(data);

	///////////////////////////

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

	dataCloud->registerData<std::string>("resources_event");
	dataCloud->updateDataValue<std::string>("resources_event", "...");


	dataCloud->registerData<std::string>("current_animation.id");
	dataCloud->registerData<double>("current_animation.ticks_progress");
	dataCloud->registerData<double>("current_animation.seconds_progress");
	dataCloud->registerData<double>("current_animation.ticks_duration");
	dataCloud->registerData<double>("current_animation.seconds_duration");



	auto sysEngine{ SystemEngine::getInstance() };

	// dataprint system filters
	const auto dataPrintSystem{ sysEngine->getSystem<mage::DataPrintSystem>(dataPrintSystemSlot) };
	dataPrintSystem->addDatacloudFilter("resources_event");
	dataPrintSystem->addDatacloudFilter("current_animation");
	dataPrintSystem->addDatacloudFilter("debug");
	dataPrintSystem->addDatacloudFilter("std");


	///////////////////////////


	d3d11_system_events();
	resource_system_events();

	//////////////////////////

}





void ModuleImpl::resource_system_events()
{	
	// register to resource system events
	const ResourceSystem::Callback rs_cb
	{
		[&, this](ResourceSystemEvent p_event, const std::string& p_resourceName)
		{
			auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

			const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

			switch (p_event)
			{
				case ResourceSystemEvent::RESOURCE_SHADER_CACHE_CREATED:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_SHADER_CACHE_CREATED : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Shader cache creation : " + p_resourceName);
					break;

				case ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_BEGIN:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_SHADER_COMPILATION_BEGIN : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Shader compilation: " + p_resourceName + " BEGIN");
					break;

				case ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_SUCCESS:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_SHADER_COMPILATION_SUCCESS : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Shader compilation " + p_resourceName + " SUCCESS");
					break;

				case ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_ERROR:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_SHADER_COMPILATION_ERROR : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Shader compilation " + p_resourceName + " ERROR");
					break;

				case ResourceSystemEvent::RESOURCE_TEXTURE_LOAD_SUCCESS:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_TEXTURE_LOAD_SUCCESS : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Texture loaded :" + p_resourceName);
					break;

				case ResourceSystemEvent::RESOURCE_MESHE_LOAD_SUCCESS:
					_MAGE_DEBUG(eventsLogger, "RECV EVENT -> RESOURCE_MESHE_LOAD_SUCCESS : " + p_resourceName);
					dataCloud->updateDataValue<std::string>("resources_event", "Meshe loaded :" + p_resourceName);
								
					break;
			}
		}
	};

	const auto sysEngine{ SystemEngine::getInstance() };
	const auto resourceSystem{ sysEngine->getSystem<mage::ResourceSystem>(resourceSystemSlot) };
	resourceSystem->registerSubscriber(rs_cb);
}

void ModuleImpl::d3d11_system_events()
{
	const auto sysEngine{ SystemEngine::getInstance() };
	const auto d3d11System{ sysEngine->getSystem<mage::D3D11System>(d3d11SystemSlot) };

	const D3D11System::Callback d3d11_cb
	{
		[&, this](D3D11SystemEvent p_event, const std::string& p_id)
		{
			switch (p_event)
			{
				case D3D11SystemEvent::D3D11_WINDOW_READY:
				{
					
					
					auto& appwindowNode{ m_entitygraph.node(p_id) };
					const auto appwindow{ appwindowNode.data() };

					const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

					const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose()};
					const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose()};


					const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

					const auto window_dims{ dataCloud->readDataValue<mage::core::maths::IntCoords2D>("std.window_resol") };

					const int w_width{ window_dims.x() };
					const int w_height{ window_dims.y() };


					m_appReady = true;
				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}

