
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
#include "graphicobjects_helpers.h"

using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void ModuleImpl::init(const std::string p_appWindowsEntityName)
{
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

	/////////// systems

	auto sysEngine{ SystemEngine::getInstance() };

	sysEngine->makeSystem<mage::TimeSystem>(timeSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::D3D11System>(d3d11SystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::ResourceSystem>(resourceSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::WorldSystem>(worldSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::RenderingQueueSystem>(renderingQueueSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::DataPrintSystem>(dataPrintSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::AnimationsSystem>(animationsSystemSlot, m_entitygraph);

	// D3D11 system provides compilation shader service : give access to this to resources sytem
	const auto d3d11System{ sysEngine->getSystem<mage::D3D11System>(d3d11SystemSlot) };
	services::ShadersCompilationService::getInstance()->registerSubscriber(d3d11System->getShaderCompilationInvocationCallback());
	services::TextureContentCopyService::getInstance()->registerSubscriber(d3d11System->getTextureContentCopyInvocationCallback());

	// dataprint system filters
	const auto dataPrintSystem{ sysEngine->getSystem<mage::DataPrintSystem>(dataPrintSystemSlot) };
	dataPrintSystem->addDatacloudFilter("resources_event");
	dataPrintSystem->addDatacloudFilter("current_animation");

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

	createEntities(p_appWindowsEntityName);

	//////////////////////////

	m_appWindowsEntityName = p_appWindowsEntityName;
}


void ModuleImpl::createEntities(const std::string p_appWindowsEntityName)
{
	/////////// add screen rendering pass entity

	auto& appwindowNode{ m_entitygraph.node(p_appWindowsEntityName) };

	auto& screenRenderingPassNode{ m_entitygraph.add(appwindowNode, "screenRendering_Pass_DirectForward_Entity") };
	const auto screenRenderingPassEntity{ screenRenderingPassNode.data() };

	auto& screenRendering_rendering_aspect{ screenRenderingPassEntity->makeAspect(core::renderingAspect::id) };

	screenRendering_rendering_aspect.addComponent<rendering::Queue>("renderingQueue", "final_pass");

	auto& rendering_queue{ screenRendering_rendering_aspect.getComponent<rendering::Queue>("renderingQueue")->getPurpose() };
	rendering_queue.setTargetClearColor({ 0, 0, 64, 255 });
	rendering_queue.enableTargetClearing(true);
	

	m_windowRenderingQueue = &rendering_queue;

	auto sysEngine{ SystemEngine::getInstance() };
	const auto dataPrintSystem{ sysEngine->getSystem<mage::DataPrintSystem>(dataPrintSystemSlot) };

	dataPrintSystem->setRenderingQueue(m_windowRenderingQueue);
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

					const auto rendering_quad_textures_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto rendering_quad_fog_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					
					mage::helpers::plugRenderingQuadView(m_entitygraph,
						characteristics_v_width, characteristics_v_height,
						"screenRendering_Pass_DirectForward_Entity",
						"screen_AlignedQuad_Entity",
						"screen_AlignedView_Entity",
						m_windowRenderingQueue,
						"pass_texture1stage_vs",
						"pass_texture1stage_ps",

						{
							std::make_pair(Texture::STAGE_0, rendering_quad_textures_channnel),
							std::make_pair(Texture::STAGE_1, rendering_quad_fog_channnel)
						}
					);


					//////////////////////////////////////////


					/////////// commons shaders params

					dataCloud->registerData<maths::Real4Vector>("texture_keycolor_ps.key_color");
					dataCloud->updateDataValue<maths::Real4Vector>("texture_keycolor_ps.key_color", maths::Real4Vector(0, 0, 0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.light0_dir");
					dataCloud->updateDataValue<maths::Real4Vector>("std.light0_dir", maths::Real4Vector(0, -0.58, 0.6, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog_color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog_color", maths::Real4Vector(0.8, 0.9, 1, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog_density");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog_density", maths::Real4Vector(0.009, 0, 0, 0));








					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_0");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_0", maths::Real4Vector(skydomeOuterRadius, skydomeInnerRadius, skydomeOuterRadius * skydomeOuterRadius, skydomeInnerRadius * skydomeInnerRadius));

					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_1");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_1", maths::Real4Vector(skydomeScaleDepth, 1.0 / skydomeScaleDepth, 1.0 / (skydomeOuterRadius - skydomeInnerRadius), (1.0 / (skydomeOuterRadius - skydomeInnerRadius)) / skydomeScaleDepth));

					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_2");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_2", maths::Real4Vector(1.0 / std::pow(skydomeWaveLength_x, 4.0), 1.0 / std::pow(skydomeWaveLength_y, 4.0), 1.0 / std::pow(skydomeWaveLength_z, 4.0), 0));

					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_3");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_3", maths::Real4Vector(skydomeKr, skydomeKm, 4.0 * skydomeKr * 3.1415927, 4.0 * skydomeKm * 3.1415927));

					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_4");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_4", maths::Real4Vector(skydomeSkyfromspace_ESun, skydomeSkyfromatmo_ESun, skydomeGroundfromspace_ESun, skydomeGroundfromatmo_ESun));

					dataCloud->registerData<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_5");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_ps.atmo_scattering_flag_5", maths::Real4Vector(0.0, 0.0, 0.0, 1));


					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// SCENEGRAPH

					create_scenegraph(p_id);

					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// RENDERGRAPH
					
					
					// Textures channel 

					// textures channels rendering queue
					rendering::Queue texturesChannelsRenderingQueue("textures_channel_queue");
					texturesChannelsRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					texturesChannelsRenderingQueue.enableTargetClearing(true);
					texturesChannelsRenderingQueue.enableTargetDepthClearing(true);
					texturesChannelsRenderingQueue.setTargetStage(Texture::STAGE_0);

					mage::helpers::plugRenderingQueue(m_entitygraph, texturesChannelsRenderingQueue, "screen_AlignedQuad_Entity", "bufferRendering_Scene_TexturesChannel_Queue_Entity");

					create_textures_channel_rendergraph("bufferRendering_Scene_TexturesChannel_Queue_Entity");
					

					// fog channel 

					rendering::Queue fogChannelsRenderingQueue("fog_channel_queue");
					fogChannelsRenderingQueue.setTargetClearColor({ 0, 0, 128, 255 });
					fogChannelsRenderingQueue.enableTargetClearing(true);
					fogChannelsRenderingQueue.enableTargetDepthClearing(true);
					fogChannelsRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, fogChannelsRenderingQueue, "screen_AlignedQuad_Entity", "bufferRendering_Scene_FogChannel_Queue_Entity");


					create_fog_channel_rendergraph("bufferRendering_Scene_FogChannel_Queue_Entity");

					{
						///////Select camera

						m_texturesChannelCurrentCamera = "camera_Entity";

						auto texturesChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_TexturesChannel_Queue_Entity")};
						texturesChannelRenderingQueue->setCurrentView(m_texturesChannelCurrentCamera);


						auto fogChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_FogChannel_Queue_Entity") };
						fogChannelRenderingQueue->setCurrentView(m_texturesChannelCurrentCamera);


					}
				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}

void ModuleImpl::create_scenegraph(const std::string& p_mainWindowsEntityId)
{
	auto& appwindowNode{ m_entitygraph.node(p_mainWindowsEntityId) };
	const auto appwindow{ appwindowNode.data() };

	const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

	const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose() };
	const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose() };


	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "ground_Entity") };
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
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel, 0.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("rect", "ground.ac"), TriangleMeshe()));

		m_groundEntity = entity;
	}

	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "clouds_Entity") };
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
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel + 400, 0.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("rect", "flatclouds.ac"), TriangleMeshe()));

		m_cloudsEntity = entity;
	}

	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "tree_Entity") };
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
				positionmat.translation(0.0, skydomeInnerRadius + groundLevel, -30.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("Plane.001", "tree0.ac"), TriangleMeshe()));

		m_treeEntity = entity;
	}

	////////////////////////////////

	{
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "skydome_Entity") };
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
				positionmat.translation(0.0, 0.0, 0.0);

				maths::Matrix scalingmat;
				scalingmat.scale(skydomeOuterRadius, skydomeOuterRadius, skydomeOuterRadius);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * scalingmat * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("sphere", "skydome.ac"), TriangleMeshe()));

		m_skydomeEntity = entity;
	}

	////////////////////////////////

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
				wp.local_pos = /* wp.local_pos * */ scalemat * positionmat;
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

	/////////////// add camera with gimbal lock jointure ////////////////

	auto& gblJointEntityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "gblJoint_Entity") };

	const auto gblJointEntity{ gblJointEntityNode.data() };

	gblJointEntity->makeAspect(core::timeAspect::id);
	auto& gbl_world_aspect{ gblJointEntity->makeAspect(core::worldAspect::id) };

	gbl_world_aspect.addComponent<transform::WorldPosition>("gbl_output");

	gbl_world_aspect.addComponent<double>("gbl_theta", 0);
	gbl_world_aspect.addComponent<double>("gbl_phi", 0);
	gbl_world_aspect.addComponent<double>("gbl_speed", 0);
	gbl_world_aspect.addComponent<maths::Real3Vector>("gbl_pos", maths::Real3Vector(-50.0, skydomeInnerRadius + groundLevel + 5, 1.0));

	gbl_world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
		{
			// input-output/components keys id mapping
			{"gimbalLockJointAnim.theta", "gbl_theta"},
			{"gimbalLockJointAnim.phi", "gbl_phi"},
			{"gimbalLockJointAnim.position", "gbl_pos"},
			{"gimbalLockJointAnim.speed", "gbl_speed"},
			{"gimbalLockJointAnim.output", "gbl_output"}

		}, helpers::animators::makeGimbalLockJointAnimator()));


	// add camera
	maths::Matrix projection;
	projection.perspective(characteristics_v_width, characteristics_v_height, 1.0, 100000.00000000000);
	helpers::plugCamera(m_entitygraph, projection, "gblJoint_Entity", "camera_Entity");

}

void ModuleImpl::create_textures_channel_rendergraph(const std::string& p_queueEntityId)
{

	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };


		const std::vector<std::pair<size_t, std::pair<std::string, Texture>>> ground_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("grass08.jpg", Texture())) };



		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_TexturesChannel_Proxy_Entity",
															"scene_recursive_texture_vs", "scene_recursive_texture_ps",
															ground_rs_list,
															1000,
															ground_textures) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& ground_resource_aspect{ m_groundEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &ground_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ ground_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& ground_world_aspect{ m_groundEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &ground_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ ground_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

	}


	/////////////////// add clouds

	{

		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "false");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "true");
		RenderState rs_alphablendop(RenderState::Operation::ALPHABLENDOP, "add");
		RenderState rs_alphablendfunc(RenderState::Operation::ALPHABLENDFUNC, "always");
		RenderState rs_alphablenddest(RenderState::Operation::ALPHABLENDDEST, "invsrcalpha");
		RenderState rs_alphablendsrc(RenderState::Operation::ALPHABLENDSRC, "srcalpha");

		const std::vector<RenderState> clouds_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling,
															rs_alphablend, rs_alphablendop, rs_alphablendfunc, rs_alphablenddest, rs_alphablendsrc
		};

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> clouds_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("flatclouds.jpg", Texture())) };

		const auto clouds_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "clouds_TexturesChannel_Proxy_Entity",
															"scene_flatclouds_vs", "scene_flatclouds_ps",
															clouds_rs_list,
															999,
															clouds_textures) };

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& clouds_resource_aspect{ m_cloudsEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &clouds_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ clouds_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		//////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& clouds_world_aspect{ m_cloudsEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &clouds_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ clouds_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

	}

	///// add tree

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> tree_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> tree_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_TexturesChannel_Proxy_Entity",
															"scene_texture1stage_keycolor_vs", "scene_texture1stage_keycolor_ps",
															tree_rs_list,
															1000,
															tree_textures) };

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& tree_resource_aspect{ m_treeEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &tree_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ tree_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		//////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& tree_world_aspect{ m_treeEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &tree_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ tree_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

		////////////////////////////////////////////////////////////////////////

		auto& tree_rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ tree_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));

	}

	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> raptor_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("raptorDif2.png", Texture())) };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_TexturesChannel_Proxy_Entity",
															"scene_texture1stage_skinning_vs", "scene_texture1stage_skinning_ps",
															raptor_rs_list,
															1000,
															raptor_textures) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& raptor_resource_aspect{ m_raptorEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &raptor_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ raptor_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		////////////////////////////////////////////////////////////////////////

		auto& raptor_world_aspect{ m_raptorEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &raptor_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ raptor_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);


		////////////////////////////////////////////////////////////////////////

		auto& proxy_raptor_resource_aspect{ raptor_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::string, Shader>* vshader_ref{ &proxy_raptor_resource_aspect.getComponent<std::pair<std::string, Shader>>("vertexShader")->getPurpose() };
		std::pair<std::string, Shader>* pshader_ref{ &proxy_raptor_resource_aspect.getComponent<std::pair<std::string, Shader>>("pixelShader")->getPurpose() };

		auto& raptor_animations_aspect{ m_raptorEntity->aspectAccess(core::animationsAspect::id) };

		raptor_animations_aspect.getComponent<std::vector<std::pair<std::string, Shader>*>>("target_vshaders")->getPurpose().push_back(vshader_ref);
	}


	///// skydome

	{

		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "false");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "true");
		RenderState rs_alphablendop(RenderState::Operation::ALPHABLENDOP, "add");
		RenderState rs_alphablendfunc(RenderState::Operation::ALPHABLENDFUNC, "always");
		RenderState rs_alphablenddest(RenderState::Operation::ALPHABLENDDEST, "invsrcalpha");
		RenderState rs_alphablendsrc(RenderState::Operation::ALPHABLENDSRC, "srcalpha");

		const std::vector<RenderState> skydome_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling,
															rs_alphablend, rs_alphablendop, rs_alphablendfunc, rs_alphablenddest, rs_alphablendsrc
		};


		const auto skydome_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "skydome_TexturesChannel_Proxy_Entity",
									"skydome_vs", "skydome_ps",
									skydome_rs_list,
									900
									) };

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& skydome_resource_aspect{ m_skydomeEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &skydome_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ skydome_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		////////////////////////////////////////////////////////////////////////

		auto& skydome_world_aspect{ m_skydomeEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &skydome_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ skydome_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

		////////////////////////////////////////////////////////////////////////

		auto& skydom_rendering_aspect{ skydome_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ skydom_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

		drawingControl.pshaders_map.push_back(std::make_pair("std.light0_dir", "light0_dir"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_0", "atmo_scattering_flag_0"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_1", "atmo_scattering_flag_1"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_2", "atmo_scattering_flag_2"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_3", "atmo_scattering_flag_3"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_4", "atmo_scattering_flag_4"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_ps.atmo_scattering_flag_5", "atmo_scattering_flag_5"));
	}
}

void ModuleImpl::create_fog_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };


		const std::vector<std::pair<size_t, std::pair<std::string, Texture>>> ground_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("grass08.jpg", Texture())) };



		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_FogChannel_Proxy_Entity",
															"scene_recursive_texture_vs", "scene_recursive_texture_ps",
															ground_rs_list,
															1000,
															ground_textures) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& ground_resource_aspect{ m_groundEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &ground_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ ground_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& ground_world_aspect{ m_groundEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &ground_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ ground_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

	}

	///// add tree

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> tree_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> tree_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_FogChannel_Proxy_Entity",
															"scene_texture1stage_keycolor_vs", "scene_texture1stage_keycolor_ps",
															tree_rs_list,
															1000,
															tree_textures) };

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& tree_resource_aspect{ m_treeEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &tree_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ tree_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		//////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& tree_world_aspect{ m_treeEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &tree_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ tree_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

		////////////////////////////////////////////////////////////////////////

		auto& tree_rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ tree_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));

	}


	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> raptor_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("raptorDif2.png", Texture())) };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_FogChannel_Proxy_Entity",
															"scene_texture1stage_skinning_vs", "scene_texture1stage_skinning_ps",
															raptor_rs_list,
															1000,
															raptor_textures) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& raptor_resource_aspect{ m_raptorEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &raptor_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ raptor_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		////////////////////////////////////////////////////////////////////////

		auto& raptor_world_aspect{ m_raptorEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &raptor_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ raptor_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);


		////////////////////////////////////////////////////////////////////////

		auto& proxy_raptor_resource_aspect{ raptor_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::string, Shader>* vshader_ref{ &proxy_raptor_resource_aspect.getComponent<std::pair<std::string, Shader>>("vertexShader")->getPurpose() };
		std::pair<std::string, Shader>* pshader_ref{ &proxy_raptor_resource_aspect.getComponent<std::pair<std::string, Shader>>("pixelShader")->getPurpose() };

		auto& raptor_animations_aspect{ m_raptorEntity->aspectAccess(core::animationsAspect::id) };

		raptor_animations_aspect.getComponent<std::vector<std::pair<std::string, Shader>*>>("target_vshaders")->getPurpose().push_back(vshader_ref);
	}
}