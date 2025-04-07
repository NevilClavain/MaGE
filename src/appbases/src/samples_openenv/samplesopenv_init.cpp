
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


void SamplesOpenEnv::init(const std::string p_appWindowsEntityName)
{
	SamplesBase::init(p_appWindowsEntityName);
	d3d11_system_events_openenv();
}


void SamplesOpenEnv::d3d11_system_events_openenv()
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


					//////////////////////////////////////////

					/////////// commons shaders params

					dataCloud->registerData<maths::Real4Vector>("texture_keycolor_ps.key_color");
					dataCloud->updateDataValue<maths::Real4Vector>("texture_keycolor_ps.key_color", maths::Real4Vector(0, 0, 0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.ambientlight.color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.ambientlight.color", maths::Real4Vector(1.0, 1.0, 1.0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.light0.dir");
					dataCloud->updateDataValue<maths::Real4Vector>("std.light0.dir", maths::Real4Vector(0, -0.58, 0.6, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog.color");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog.color", maths::Real4Vector(0.8, 0.9, 1, 1));

					dataCloud->registerData<maths::Real4Vector>("std.fog.density");
					dataCloud->updateDataValue<maths::Real4Vector>("std.fog.density", maths::Real4Vector(0.009, 0, 0, 0));


					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_0");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_0", maths::Real4Vector(skydomeOuterRadius, skydomeInnerRadius, skydomeOuterRadius * skydomeOuterRadius, skydomeInnerRadius * skydomeInnerRadius));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_1");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_1", maths::Real4Vector(skydomeScaleDepth, 1.0 / skydomeScaleDepth, 1.0 / (skydomeOuterRadius - skydomeInnerRadius), (1.0 / (skydomeOuterRadius - skydomeInnerRadius)) / skydomeScaleDepth));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_2");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_2", maths::Real4Vector(1.0 / std::pow(skydomeWaveLength_x, 4.0), 1.0 / std::pow(skydomeWaveLength_y, 4.0), 1.0 / std::pow(skydomeWaveLength_z, 4.0), 0));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_3");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_3", maths::Real4Vector(skydomeKr, skydomeKm, 4.0 * skydomeKr * 3.1415927, 4.0 * skydomeKm * 3.1415927));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_4");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_4", maths::Real4Vector(skydomeSkyfromspace_ESun, skydomeSkyfromatmo_ESun, skydomeGroundfromspace_ESun, skydomeGroundfromatmo_ESun));

					dataCloud->registerData<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_5");
					dataCloud->updateDataValue<maths::Real4Vector>("scene_skydome_ps.atmo_scattering_flag_5", maths::Real4Vector(0.0, 0.0, 0.0, 1));


					

					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// SCENEGRAPH

					create_openenv_scenegraph(p_id);

					///////////////////////////////////////////////////////////////////////////////////////////////////////////
					// RENDERGRAPH

					const auto combiner_fog_input_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto combiner_fog_zdepths_channnel{ Texture(Texture::Format::TEXTURE_FLOAT32, w_width, w_height) };

					mage::helpers::plugRenderingQuad(m_entitygraph,
						"fog_queue",
						characteristics_v_width, characteristics_v_height,
						"screenRendering_Filter_DirectForward_Quad_Entity",
						"bufferRendering_Combiner_Fog_Queue_Entity",
						"bufferRendering_Combiner_Fog_Quad_Entity",
						"bufferRendering_Combiner_Fog_View_Entity",
						"combiner_fog_vs",
						"combiner_fog_ps",
						{
							std::make_pair(Texture::STAGE_0, combiner_fog_input_channnel),
							std::make_pair(Texture::STAGE_1, combiner_fog_zdepths_channnel),
						});

					Entity* bufferRendering_Combiner_Fog_Quad_Entity{ m_entitygraph.node("bufferRendering_Combiner_Fog_Quad_Entity").data() };

					auto& screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect{ bufferRendering_Combiner_Fog_Quad_Entity->aspectAccess(core::renderingAspect::id) };

					rendering::DrawingControl& fogDrawingControl{ screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
					fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.color", "fog_color"));
					fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.density", "fog_density"));


					// channel : zdepth

					rendering::Queue zdepthChannelsRenderingQueue("zdepth_channel_queue");
					zdepthChannelsRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					zdepthChannelsRenderingQueue.enableTargetClearing(true);
					zdepthChannelsRenderingQueue.enableTargetDepthClearing(true);
					zdepthChannelsRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, zdepthChannelsRenderingQueue, "bufferRendering_Combiner_Fog_Quad_Entity", "bufferRendering_Scene_ZDepthChannel_Queue_Entity");


					create_openenv_zdepth_channel_rendergraph("bufferRendering_Scene_ZDepthChannel_Queue_Entity");





					const auto combiner_modulate_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto combiner_modulate_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };

					mage::helpers::plugRenderingQuad(m_entitygraph,
						"modulate_queue",
						characteristics_v_width, characteristics_v_height,
						"bufferRendering_Combiner_Fog_Quad_Entity",
						"bufferRendering_Combiner_Modulate_Queue_Entity",
						"bufferRendering_Combiner_Modulate_Quad_Entity",
						"bufferRendering_Combiner_Modulate_View_Entity",
						"combiner_modulate_vs",
						"combiner_modulate_ps",
						{
							std::make_pair(Texture::STAGE_0, combiner_modulate_inputA_channnel),
							std::make_pair(Texture::STAGE_1, combiner_modulate_inputB_channnel),
						});



					// channel : textures
					
					rendering::Queue texturesChannelsRenderingQueue("textures_channel_queue");
					texturesChannelsRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					texturesChannelsRenderingQueue.enableTargetClearing(true);
					texturesChannelsRenderingQueue.enableTargetDepthClearing(true);
					texturesChannelsRenderingQueue.setTargetStage(Texture::STAGE_0);

					mage::helpers::plugRenderingQueue(m_entitygraph, texturesChannelsRenderingQueue, "bufferRendering_Combiner_Modulate_Quad_Entity", "bufferRendering_Scene_TexturesChannel_Queue_Entity");

					create_openenv_textures_channel_rendergraph("bufferRendering_Scene_TexturesChannel_Queue_Entity");
					


				
					// channel : ambient light

					rendering::Queue ambientLightChannelsRenderingQueue("ambientlight_channel_queue");
					ambientLightChannelsRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					ambientLightChannelsRenderingQueue.enableTargetClearing(true);
					ambientLightChannelsRenderingQueue.enableTargetDepthClearing(true);
					ambientLightChannelsRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, ambientLightChannelsRenderingQueue, "bufferRendering_Combiner_Modulate_Quad_Entity", "bufferRendering_Scene_AmbientLightChannel_Queue_Entity");

					create_openenv_ambientlight_channel_rendergraph("bufferRendering_Scene_AmbientLightChannel_Queue_Entity");
					





					///////////////////////////////////////////////////////////////////////////////////////////////////

					{
						///////Select camera

						m_currentCamera = "camera_Entity";

						auto texturesChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_TexturesChannel_Queue_Entity")};
						texturesChannelRenderingQueue->setCurrentView(m_currentCamera);

						auto ambientLightChannelRenderingQueue{ helpers::getRenderingQueue(m_entitygraph, "bufferRendering_Scene_AmbientLightChannel_Queue_Entity") };
						ambientLightChannelRenderingQueue->setCurrentView(m_currentCamera);


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


void SamplesOpenEnv::create_openenv_scenegraph(const std::string& p_mainWindowsEntityId)
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
}

void SamplesOpenEnv::create_openenv_textures_channel_rendergraph(const std::string& p_queueEntityId)
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
									"scene_skydome_vs", "scene_skydome_ps",
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

		drawingControl.pshaders_map.push_back(std::make_pair("std.light0.dir", "light0_dir"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_0", "atmo_scattering_flag_0"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_1", "atmo_scattering_flag_1"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_2", "atmo_scattering_flag_2"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_3", "atmo_scattering_flag_3"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_4", "atmo_scattering_flag_4"));
		drawingControl.pshaders_map.push_back(std::make_pair("scene_skydome_ps.atmo_scattering_flag_5", "atmo_scattering_flag_5"));
	}
	
}

void SamplesOpenEnv::create_openenv_zdepth_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_FogChannel_Proxy_Entity",
															"scene_zdepth_vs", "scene_zdepth_ps",
															ground_rs_list,
															1000) };

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
															"scene_zdepth_keycolor_vs", "scene_zdepth_keycolor_ps",
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
}

void SamplesOpenEnv::create_openenv_ambientlight_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_AmbientLightChannel_Proxy_Entity",
															"scene_flatcolor_vs", "scene_flatcolor_ps",
															ground_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ ground_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));
		

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

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_AmbientLightChannel_Proxy_Entity",
															"scene_flatcolor_keycolor_vs", "scene_flatcolor_keycolor_ps",
															tree_rs_list,
															1000,
															tree_textures) };

		/// connect shaders args

		auto& tree_rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ tree_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));

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

		const auto clouds_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "clouds_AmbientLightChannel_Proxy_Entity",
															"scene_flatcolor_keycolor_vs", "scene_flatcolor_keycolor_ps",
															clouds_rs_list,
															999,
															clouds_textures) };

		/// connect shaders args

		auto& clouds_rendering_aspect{ clouds_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ clouds_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));

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


		const auto skydome_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "skydome_AmbientLightChannel_Proxy_Entity",
									"scene_flatcolor_keycolor_vs", "scene_flatcolor_keycolor_ps",
									skydome_rs_list,
									900
									) };

		/// connect shaders args

		auto& skydome_rendering_aspect{ skydome_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ skydome_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));


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


	}

}