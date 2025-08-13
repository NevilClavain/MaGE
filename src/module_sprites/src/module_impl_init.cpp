
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

#include "maths_helpers.h"

#include "aspects.h"

#include "d3d11system.h"
#include "timesystem.h"
#include "resourcesystem.h"
#include "worldsystem.h"
#include "dataprintsystem.h"
#include "renderingqueuesystem.h"

#include "sysengine.h"
#include "filesystem.h"
#include "logconf.h"

#include "trianglemeshe.h"
#include "texture.h"
#include "renderstate.h"
#include "shader.h"



#include "logger_service.h"

#include "worldposition.h"
#include "animatorfunc.h"

#include "syncvariable.h"
#include "animators_helpers.h"

#include "datacloud.h"

#include "shaders_service.h"
#include "textures_service.h"

#include "entitygraph_helpers.h"


using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void ModuleImpl::init(const std::string p_appWindowsEntityName)
{
	SamplesBase::init(p_appWindowsEntityName);

	/////////// logging conf

	mage::core::FileContent<char> logConfFileContent("./module_sprites_config/logconf.json");
	logConfFileContent.load();

	const auto dataSize{ logConfFileContent.getDataSize() };
	const std::string data(logConfFileContent.getData(), dataSize);

	/*
	mage::core::Json<> jsonParser;
	jsonParser.registerSubscriber(logger::Configuration::getInstance()->getCallback());

	const auto logParseStatus{ jsonParser.parse(data) };

	if (logParseStatus < 0)
	{
		_EXCEPTION("Cannot parse logging configuration")
	}
	*/

	logger::Configuration::getInstance()->applyConfiguration(data);

	////////////////////////

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

	dataCloud->registerData<std::string>("resources_event");
	dataCloud->updateDataValue<std::string>("resources_event", "...");

	/////////// systems

	auto sysEngine{ SystemEngine::getInstance() };


	// dataprint system filters
	const auto dataPrintSystem{ sysEngine->getSystem<mage::DataPrintSystem>(dataPrintSystemSlot) };
	dataPrintSystem->addDatacloudFilter("resources_event");


	d3d11_system_events();
	resource_system_events();

	//////////////////////////

	const auto seed{ ::GetTickCount() };

	m_generator = new std::default_random_engine(seed);

	m_speed_distribution = new std::uniform_real_distribution<double>(0.1, 0.2);
	m_rotation_speed_distribution = new std::uniform_real_distribution<double>(0.25 * core::maths::pi, 2 * core::maths::pi);
	m_speed_sign_distribution = new std::uniform_int_distribution<int>(0, 1);
	m_rotation_speed_sign_distribution = new std::uniform_int_distribution<int>(0, 1);
}

void ModuleImpl::resource_system_events()
{
	const auto sysEngine{ SystemEngine::getInstance() };

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




					const auto rendering_quad_textures_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };


					mage::helpers::plugRenderingQuad(m_entitygraph,
						"fog_queue",
						characteristics_v_width, characteristics_v_height,
						"screenRendering_Filter_DirectForward_Quad_Entity",
						"bufferRendering_Filter_DirectForward_Queue_Entity",
						"bufferRendering_Filter_DirectForward_Quad_Entity",
						"bufferRendering_Filter_DirectForward_View_Entity",
						"filter_directforward_vs",
						"filter_directforward_ps",
						{
							std::make_pair(Texture::STAGE_0, rendering_quad_textures_channnel)
						},
						Texture::STAGE_0);




					// add camera to scene
					maths::Matrix projection;
					projection.perspective(characteristics_v_width, characteristics_v_height, 1.0, 100000.00000000000);
					helpers::plugCamera(m_entitygraph, projection, m_appWindowsEntityName, "cameraEntity");

					// attach animator/positionner to camera
					core::Entitygraph::Node& cameraNode{ m_entitygraph.node("cameraEntity") };
					const auto cameraEntity{ cameraNode.data() };
					auto& camera_world_aspect{ cameraEntity->aspectAccess(core::worldAspect::id) };

					cameraEntity->makeAspect(core::timeAspect::id);
					camera_world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
					(
						{},
						[](const core::ComponentContainer& p_world_aspect,
							const core::ComponentContainer& p_time_aspect,
							const transform::WorldPosition&,
							const std::unordered_map<std::string, std::string>&)
						{

							core::maths::Matrix positionmat;
							positionmat.translation(0.0, 0.0, 5.000);

							transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("camera_position")->getPurpose() };
							wp.local_pos = wp.local_pos * positionmat;
						}
					));


					///////Select camera

					m_currentCamera = "cameraEntity";

					auto texturesChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Filter_DirectForward_Queue_Entity") };
					texturesChannelRenderingQueue->setMainView(m_currentCamera);

					/////////////////////////////////

					rendering::RenderState rs_noculling(rendering::RenderState::Operation::SETCULLING, "cw");
					rendering::RenderState rs_zbuffer(rendering::RenderState::Operation::ENABLEZBUFFER, "false");
					rendering::RenderState rs_fill(rendering::RenderState::Operation::SETFILLMODE, "solid");
					rendering::RenderState rs_texturepointsampling(rendering::RenderState::Operation::SETTEXTUREFILTERTYPE, "point");

					const std::vector<rendering::RenderState> rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling };


					auto ball{ helpers::plug2DSpriteWithSyncVariables(m_entitygraph, "bufferRendering_Filter_DirectForward_Queue_Entity", "sprite#0", 0.05, 0.05, "sprite_vs", "sprite_ps", "tennis_ball.bmp", rs_list, 1000,
																		mage::transform::WorldPosition::TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT) };
					m_sprites.push_back(ball);


					helpers::plug2DSpriteWithSyncVariables(m_entitygraph, "bufferRendering_Filter_DirectForward_Queue_Entity", "terrain", 1.5, 1.1, "sprite_vs", "sprite_ps", "terrain_tennis.jpg", rs_list, 999,
						mage::transform::WorldPosition::TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT);


				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}