
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
#include "rendering_helpers.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void ModuleImpl::init(const std::string p_appWindowsEntityName)
{
	SamplesOpenEnv::init(p_appWindowsEntityName);

	/////////// logging conf

	mage::core::FileContent<char> logConfFileContent("./module_anims_config/logconf.json");
	logConfFileContent.load();

	const auto dataSize{ logConfFileContent.getDataSize() };
	const std::string data(logConfFileContent.getData(), dataSize);

	mage::core::Json<> jsonParser;
	jsonParser.registerSubscriber(logger::Configuration::getInstance()->getCallback());

	const auto logParseStatus{ jsonParser.parse(data) };

	if (logParseStatus < 0)
	{
		_EXCEPTION("Cannot parse logging configuration")
	}

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

	///////////////////////////

	auto now = std::chrono::system_clock::now();
	auto now_c = std::chrono::system_clock::to_time_t(now);
	const int time_based_seed{ static_cast<int>(now_c) };
	m_random_engine.seed(time_based_seed);

	///////////////////////////

	d3d11_system_events();
	resource_system_events();
	animation_system_events();

	//////////////////////////

}


void ModuleImpl::animation_system_events()
{
	// register to animation system events
	const AnimationsSystem::Callback cb
	{
		[&, this](AnimationSystemEvent p_event, const std::string& /*p_entityId*/, const std::string& /*p_animationName*/)
		{
			switch (p_event)
			{
				case AnimationSystemEvent::ANIMATION_START:
					break;

				case AnimationSystemEvent::ANIMATION_END:

					choose_animation();
					break;
			}
		}
	};

	const auto sysEngine{ SystemEngine::getInstance() };
	const auto animationsSystem{ sysEngine->getSystem<mage::AnimationsSystem>(animationsSystemSlot) };
	animationsSystem->registerSubscriber(cb);
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
					
					if ("raptor.fbx" == p_resourceName)
					{											
						const auto& resources_aspect{ m_raptorEntity->aspectAccess(core::resourcesAspect::id) };

						const auto& meshe_comp{ resources_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe") };

						const auto& meshe_descr{ meshe_comp->getPurpose() };
						const TriangleMeshe& meshe{ meshe_descr.second };

						m_raptor_animations = meshe.getAnimationsKeys();
						m_distribution = new std::uniform_int_distribution<int>(0, m_raptor_animations.size() - 1);

						choose_animation();
						
					}					
					break;
			}
		}
	};

	const auto sysEngine{ SystemEngine::getInstance() };
	const auto resourceSystem{ sysEngine->getSystem<mage::ResourceSystem>(resourceSystemSlot) };
	resourceSystem->registerSubscriber(rs_cb);
}

void ModuleImpl::choose_animation()
{
	
	int anim_index = (*m_distribution)(m_random_engine);
	std::vector<std::string> anims_names;

	for (const auto& e : m_raptor_animations)
	{
		anims_names.push_back(e.first);
	}

	const std::string choosen_anim{ anims_names.at(anim_index) };

	const auto raptorEntity{ m_raptorEntity };
	auto& anims_aspect{ raptorEntity->aspectAccess(core::animationsAspect::id) };
	auto& animationsIdList{ anims_aspect.getComponent<std::list<std::string>>("eg.std.animationsIdList")->getPurpose() };

	animationsIdList.push_back(choosen_anim);

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

					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// SCENEGRAPH

					complete_scenegraph(p_id);

					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// RENDERGRAPH

					const auto renderingHelper{ mage::helpers::Rendering::getInstance() };

					// raptor rendering
					{
						auto textures_channel_config{ renderingHelper->getPassConfig("TexturesChannel") };
						textures_channel_config.vshader = "scene_texture1stage_skinning_vs";
						textures_channel_config.pshader = "scene_texture1stage_skinning_ps";
						textures_channel_config.textures_files_list = { std::make_pair(Texture::STAGE_0, std::make_pair("raptorDif2.png", Texture())) };


						auto zdepth_channel_config{ renderingHelper->getPassConfig("ZDepthChannel") };
						zdepth_channel_config.vshader = "scene_zdepth_skinning_vs";
						zdepth_channel_config.pshader = "scene_zdepth_skinning_ps";

						auto ambientlight_channel_config{ renderingHelper->getPassConfig("AmbientLightChannel") };
						ambientlight_channel_config.vshader = "scene_flatcolor_skinning_vs";
						ambientlight_channel_config.pshader = "scene_flatcolor_skinning_ps";

						auto lit_channel_config{ renderingHelper->getPassConfig("LitChannel") };
						lit_channel_config.vshader = "scene_lit_skinning_vs";
						lit_channel_config.pshader = "scene_lit_skinning_ps";

						auto em_channel_config{ renderingHelper->getPassConfig("EmissiveChannel") };
						em_channel_config.vshader = "scene_flatcolor_skinning_vs";
						em_channel_config.pshader = "scene_flatcolor_skinning_ps";

	

						const std::unordered_map< std::string, helpers::PassConfig> config =
						{
							{ "TexturesChannel", textures_channel_config },
							{ "ZDepthChannel", zdepth_channel_config },
							{ "AmbientLightChannel", ambientlight_channel_config },
							{ "LitChannel", lit_channel_config },
							{ "EmissiveChannel", em_channel_config }
						};

						std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> vertex_shaders_params =
						{
						};

						std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> pixel_shaders_params =
						{
							{ "AmbientLightChannel",
								{
									{ std::make_pair("std.ambientlight.color", "color") }
								}
							},
							{ "LitChannel",
								{
									{ std::make_pair("std.light0.dir", "light_dir") }
								}
							},
							{ "EmissiveChannel",
								{
									{ std::make_pair("std.black_color", "color") }
								}
							}
						};

						renderingHelper->registerToPasses(m_entitygraph, m_raptorEntity, config, vertex_shaders_params, pixel_shaders_params);
					}

					complete_install_shadows_renderer_objects();

					///////////////////////////////////////////////////////////////////////////////////////////////////////////

					{
						///////Select camera

						/*
							"bufferRendering_Scene_TexturesChannel_Queue_Entity", "camera_Entity", ""
							"bufferRendering_Scene_AmbientLightChannel_Queue_Entity", "camera_Entity", ""
							...

							"bufferRendering_Scene_ShadowsChannel_Queue_Entity", "camera_Entity", "shadowmap_camera_Entity"

							"bufferRendering_Scene_ShadowMapChannel_Queue_Entity", "shadowmap_camera_Entity", ""


						*/



						m_currentCamera = "camera_Entity";
						
						auto texturesChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						texturesChannelRenderingQueue->setMainView(m_currentCamera);

						auto ambientLightChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientLightChannelRenderingQueue->setMainView(m_currentCamera);

						auto emissiveChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						emissiveChannelRenderingQueue->setMainView(m_currentCamera);
						
						auto litChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_LitChannel_Queue_Entity") };
						litChannelRenderingQueue->setMainView(m_currentCamera);

						auto zDepthChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						zDepthChannelRenderingQueue->setMainView(m_currentCamera);
					}
				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}


void ModuleImpl::complete_scenegraph(const std::string& p_mainWindowsEntityId)
{
	
	auto& appwindowNode{ m_entitygraph.node(p_mainWindowsEntityId) };
	const auto appwindow{ appwindowNode.data() };

	const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

	const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose() };
	const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose() };
	
	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "raptor_Entity") };
		const auto entity{ entityNode.data() };

		auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
		entity->makeAspect(core::timeAspect::id);

		world_aspect.addComponent<transform::WorldPosition>("position");

		world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
		(
			{},
			[=](const core::ComponentContainer& p_world_aspect,
				const core::ComponentContainer& p_time_aspect,
				const transform::WorldPosition&,
				const std::unordered_map<std::string, std::string>&)
			{

				maths::Matrix positionmat;
				positionmat.translation(-40.0, skydomeInnerRadius + groundLevel, -30.0);

				maths::Matrix scalemat;
				scalemat.scale(0.03, 0.03, 0.03);


				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = scalemat * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("raptorMesh", "raptor.fbx"), TriangleMeshe()));

		auto& raptor_animations_aspect{ entity->makeAspect(core::animationsAspect::id) };
		raptor_animations_aspect.addComponent<int>("eg.std.animationbonesArrayArgIndex", 0);


		raptor_animations_aspect.addComponent<std::list<std::string>>("eg.std.animationsIdList");
		raptor_animations_aspect.addComponent<std::list<std::pair<std::string, AnimationKeys>>>("eg.std.animationsList");

		raptor_animations_aspect.addComponent<core::TimeMark>("eg.std.animationsTimeMark", TimeControl::getInstance()->buildTimeMark());

		raptor_animations_aspect.addComponent<std::string>("eg.std.currentAnimationId");
		raptor_animations_aspect.addComponent<AnimationKeys>("eg.std.currentAnimation");


		raptor_animations_aspect.addComponent<double>("eg.std.currentAnimationTicksDuration");
		raptor_animations_aspect.addComponent<double>("eg.std.currentAnimationSecondsDuration");

		raptor_animations_aspect.addComponent<double>("eg.std.currentAnimationTicksProgress");
		raptor_animations_aspect.addComponent<double>("eg.std.currentAnimationSecondsProgress");

		raptor_animations_aspect.addComponent<std::vector<std::pair<std::string, Shader>*>>("target_vshaders");


		m_raptorEntity = entity;
	}
}


void ModuleImpl::complete_install_shadows_renderer_objects()
{
	auto& shadowMapNode{ m_entitygraph.node("shadowMap_Texture_Entity") };
	const auto shadowmap_texture_entity{ shadowMapNode.data() };
	auto& sm_resource_aspect{ shadowmap_texture_entity->aspectAccess(core::resourcesAspect::id) };
	std::pair<size_t, Texture>* sm_texture_ptr{ &sm_resource_aspect.getComponent<std::pair<size_t, Texture>>("standalone_rendering_target_texture")->getPurpose() };

	const auto renderingHelper{ mage::helpers::Rendering::getInstance() };

	renderingHelper->registerPass("ShadowsChannel", "bufferRendering_Scene_ShadowsChannel_Queue_Entity");
	renderingHelper->registerPass("ShadowMapChannel", "bufferRendering_Scene_ShadowMapChannel_Queue_Entity");


	// raptor shadows rendering
	{
		auto shadows_channel_config{ renderingHelper->getPassConfig("ShadowsChannel") };
		shadows_channel_config.vshader = "scene_shadowsmask_skinning_vs";
		shadows_channel_config.pshader = "scene_shadowsmask_skinning_ps";
		shadows_channel_config.textures_ptr_list = { sm_texture_ptr };

		auto shadowmap_channel_config{ renderingHelper->getPassConfig("ShadowMapChannel") };
		shadowmap_channel_config.vshader = "scene_zdepth_skinning_vs";
		shadowmap_channel_config.pshader = "scene_zdepth_skinning_ps";
		shadowmap_channel_config.rs_list.at(0).setOperation(RenderState::Operation::SETCULLING);
		shadowmap_channel_config.rs_list.at(0).setArg("ccw");

		const std::unordered_map< std::string, helpers::PassConfig> config =
		{
			{ "ShadowsChannel", shadows_channel_config },
			{ "ShadowMapChannel", shadowmap_channel_config }
		};

		std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> vertex_shaders_params =
		{
		};

		std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> pixel_shaders_params =
		{
			{ "ShadowsChannel",
				{
					{ std::make_pair("shadow_bias", "shadow_bias") },
					{ std::make_pair("shadowmap_resol", "shadowmap_resol") }
				}
			}
		};

		renderingHelper->registerToPasses(m_entitygraph, m_raptorEntity, config, vertex_shaders_params, pixel_shaders_params);
	}
}

