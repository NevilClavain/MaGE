
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
				
					complete_textures_channel_rendergraph("bufferRendering_Scene_TexturesChannel_Queue_Entity");

					complete_openenv_ambientlight_channel_rendergraph("bufferRendering_Scene_AmbientLightChannel_Queue_Entity");
					complete_openenv_lit_channel_rendergraph("bufferRendering_Scene_LitChannel_Queue_Entity");

					complete_emissive_lit_channel_rendergraph("bufferRendering_Scene_EmissiveChannel_Queue_Entity");

					complete_zdepth_channel_rendergraph("bufferRendering_Scene_ZDepthChannel_Queue_Entity");


					{
						///////Select camera

						m_currentCamera = "camera_Entity";

						auto texturesChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_TexturesChannel_Queue_Entity") };
						texturesChannelRenderingQueue->setCurrentView(m_currentCamera);

						auto ambientLightChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientLightChannelRenderingQueue->setCurrentView(m_currentCamera);

						auto emissiveChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_EmissiveChannel_Queue_Entity") };
						emissiveChannelRenderingQueue->setCurrentView(m_currentCamera);

						auto litChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_LitChannel_Queue_Entity") };
						litChannelRenderingQueue->setCurrentView(m_currentCamera);

						auto fogChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_ZDepthChannel_Queue_Entity") };
						fogChannelRenderingQueue->setCurrentView(m_currentCamera);
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



void ModuleImpl::complete_textures_channel_rendergraph(const std::string& p_queueEntityId)
{

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
}

void ModuleImpl::complete_zdepth_channel_rendergraph(const std::string& p_queueEntityId)
{
	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_ZDepthChannel_Proxy_Entity",
															"scene_zdepth_skinning_vs", "scene_zdepth_skinning_ps",
															raptor_rs_list,
															1000) };

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

void ModuleImpl::complete_openenv_ambientlight_channel_rendergraph(const std::string& p_queueEntityId)
{
	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_AmbientLightChannel_Proxy_Entity",
															"scene_flatcolor_skinning_vs", "scene_flatcolor_skinning_ps",
															raptor_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ raptor_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));


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

void ModuleImpl::complete_openenv_lit_channel_rendergraph(const std::string& p_queueEntityId)
{
	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_LitChannel_Proxy_Entity",
															"scene_lit_skinning_vs", "scene_lit_skinning_ps",
															raptor_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ raptor_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.light0.dir", "light_dir"));

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

void ModuleImpl::complete_emissive_lit_channel_rendergraph(const std::string& p_queueEntityId)
{
	///// Raptor

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> raptor_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto raptor_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "raptor_EmissiveChannel_Proxy_Entity",
															"scene_flatcolor_skinning_vs", "scene_flatcolor_skinning_ps",
															raptor_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ raptor_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.black_emissive_color", "color"));


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