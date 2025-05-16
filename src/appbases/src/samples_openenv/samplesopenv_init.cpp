
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
					dataCloud->updateDataValue<maths::Real4Vector>("std.ambientlight.color", maths::Real4Vector(0.33, 0.33, 0.33, 1));


					dataCloud->registerData<maths::Real4Vector>("skydome_emissive_color");
					dataCloud->updateDataValue<maths::Real4Vector>("skydome_emissive_color", maths::Real4Vector(1.0, 1.0, 1.0, 1));


					dataCloud->registerData<maths::Real4Vector>("black_color");
					dataCloud->updateDataValue<maths::Real4Vector>("black_color", maths::Real4Vector(0.0, 0.0, 0.0, 1));

					dataCloud->registerData<maths::Real4Vector>("white_color");
					dataCloud->updateDataValue<maths::Real4Vector>("white_color", maths::Real4Vector(1.0, 1.0, 1.0, 1));


					dataCloud->registerData<maths::Real4Vector>("std.light0.dir");
					dataCloud->updateDataValue<maths::Real4Vector>("std.light0.dir", maths::Real4Vector(0.6, -0.18, 0.1, 1));

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
					// SHADOWMAP TARGET TEXTURE


					//helpers::plugTargetTexture(m_entitygraph, p_id, "shadowMap_Texture_Entity", std::make_pair(Texture::STAGE_0, Texture(Texture::Format::TEXTURE_RGB, 1024, 1024)));
					helpers::plugTargetTexture(m_entitygraph, p_id, "shadowMap_Texture_Entity", std::make_pair(Texture::STAGE_0, Texture(Texture::Format::TEXTURE_FLOAT32, 2048, 2048)));



					rendering::Queue shadowMapChannelRenderingQueue("shadowmap_channel_queue");
					shadowMapChannelRenderingQueue.setTargetClearColor({ 250, 0, 0, 255 });
					shadowMapChannelRenderingQueue.enableTargetClearing(true);
					shadowMapChannelRenderingQueue.enableTargetDepthClearing(true);
					shadowMapChannelRenderingQueue.setTargetStage(Texture::STAGE_0);

					mage::helpers::plugRenderingQueue(m_entitygraph, shadowMapChannelRenderingQueue, "shadowMap_Texture_Entity", "bufferRendering_Scene_ShadowMapChannel_Queue_Entity");


					create_openenv_shadowmap_channel_rendergraph("bufferRendering_Scene_ShadowMapChannel_Queue_Entity");
					//create_openenv_shadowmap_channel_rendergraph("bufferRendering_Scene_Debug_Queue_Entity");




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
						},
						Texture::STAGE_0);

					Entity* bufferRendering_Combiner_Fog_Quad_Entity{ m_entitygraph.node("bufferRendering_Combiner_Fog_Quad_Entity").data() };

					auto& screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect{ bufferRendering_Combiner_Fog_Quad_Entity->aspectAccess(core::renderingAspect::id) };

					rendering::DrawingControl& fogDrawingControl{ screenRendering_Combiner_Fog_Quad_Entity_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
					fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.color", "fog_color"));
					fogDrawingControl.pshaders_map.push_back(std::make_pair("std.fog.density", "fog_density"));


					// channel : zdepth

					rendering::Queue zdepthChannelRenderingQueue("zdepth_channel_queue");
					zdepthChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					zdepthChannelRenderingQueue.enableTargetClearing(true);
					zdepthChannelRenderingQueue.enableTargetDepthClearing(true);
					zdepthChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, zdepthChannelRenderingQueue, "bufferRendering_Combiner_Fog_Quad_Entity", "bufferRendering_Scene_ZDepthChannel_Queue_Entity");


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
						},
						Texture::STAGE_0);


					/////////////////////////////////////////////////////////////////


					// channel : textures
					
					rendering::Queue texturesChannelRenderingQueue("textures_channel_queue");
					texturesChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					texturesChannelRenderingQueue.enableTargetClearing(true);
					texturesChannelRenderingQueue.enableTargetDepthClearing(true);
					texturesChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, texturesChannelRenderingQueue, "bufferRendering_Combiner_Modulate_Quad_Entity", "bufferRendering_Scene_TexturesChannel_Queue_Entity");

					create_openenv_textures_channel_rendergraph("bufferRendering_Scene_TexturesChannel_Queue_Entity");
					









					// channel : ligths and shadows effect
					
					const auto combiner_accumulate_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto combiner_accumulate_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto combiner_accumulate_inputC_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };


					mage::helpers::plugRenderingQuad(m_entitygraph,
						"acc_lit_queue",
						characteristics_v_width, characteristics_v_height,
						"bufferRendering_Combiner_Modulate_Quad_Entity",
						"bufferRendering_Combiner_Accumulate_Queue_Entity",
						"bufferRendering_Combiner_Accumulate_Quad_Entity",
						"bufferRendering_Combiner_Accumulate_View_Entity",
						
						"combiner_accumulate3chan_vs",
						"combiner_accumulate3chan_ps",
						{
							std::make_pair(Texture::STAGE_0, combiner_accumulate_inputA_channnel),
							std::make_pair(Texture::STAGE_1, combiner_accumulate_inputB_channnel),
							std::make_pair(Texture::STAGE_2, combiner_accumulate_inputC_channnel),
						},
						Texture::STAGE_0);



					// channel : ambient light

					rendering::Queue ambientLightChannelRenderingQueue("ambientlight_channel_queue");
					ambientLightChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					ambientLightChannelRenderingQueue.enableTargetClearing(true);
					ambientLightChannelRenderingQueue.enableTargetDepthClearing(true);
					ambientLightChannelRenderingQueue.setTargetStage(Texture::STAGE_0);

					mage::helpers::plugRenderingQueue(m_entitygraph, ambientLightChannelRenderingQueue, "bufferRendering_Combiner_Accumulate_Quad_Entity", "bufferRendering_Scene_AmbientLightChannel_Queue_Entity");

					create_openenv_ambientlight_channel_rendergraph("bufferRendering_Scene_AmbientLightChannel_Queue_Entity");


					// channel : emissive

					rendering::Queue emissiveChannelRenderingQueue("emissive_channel_queue");
					emissiveChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					emissiveChannelRenderingQueue.enableTargetClearing(true);
					emissiveChannelRenderingQueue.enableTargetDepthClearing(true);
					emissiveChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, emissiveChannelRenderingQueue, "bufferRendering_Combiner_Accumulate_Quad_Entity", "bufferRendering_Scene_EmissiveChannel_Queue_Entity");

					create_openenv_emissive_channel_rendergraph("bufferRendering_Scene_EmissiveChannel_Queue_Entity");







					// channel to modulate directional lit and shadow map



					const auto combiner_modulatelitshadows_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto combiner_modulatelitshadows_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };



					mage::helpers::plugRenderingQuad(m_entitygraph,
						"mod_lit_shadows_queue",
						characteristics_v_width, characteristics_v_height,
						"bufferRendering_Combiner_Accumulate_Quad_Entity",
						"bufferRendering_Combiner_ModulateLitAndShadows_Queue_Entity",
						"bufferRendering_Combiner_ModulateLitAndShadows_Quad_Entity",
						"bufferRendering_Combiner_ModulateLitAndShadows_View_Entity",

						"combiner_modulate_vs",
						"combiner_modulate_ps",
						{
							std::make_pair(Texture::STAGE_0, combiner_modulatelitshadows_inputA_channnel),
							std::make_pair(Texture::STAGE_1, combiner_modulatelitshadows_inputB_channnel),
						},
						Texture::STAGE_2);







					// channel : directional lit

					rendering::Queue litChannelRenderingQueue("lit_channel_queue");
					litChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					litChannelRenderingQueue.enableTargetClearing(true);
					litChannelRenderingQueue.enableTargetDepthClearing(true);
					litChannelRenderingQueue.setTargetStage(Texture::STAGE_0);

					mage::helpers::plugRenderingQueue(m_entitygraph, litChannelRenderingQueue, "bufferRendering_Combiner_ModulateLitAndShadows_Quad_Entity", "bufferRendering_Scene_LitChannel_Queue_Entity");

					create_openenv_lit_channel_rendergraph("bufferRendering_Scene_LitChannel_Queue_Entity");
					//create_openenv_lit_channel_rendergraph("bufferRendering_Scene_Debug_Queue_Entity");




					// channel : shadow

					rendering::Queue shadowsChannelRenderingQueue("shadows_channel_queue");
					shadowsChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
					shadowsChannelRenderingQueue.enableTargetClearing(true);
					shadowsChannelRenderingQueue.enableTargetDepthClearing(true);
					shadowsChannelRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, shadowsChannelRenderingQueue, "bufferRendering_Combiner_ModulateLitAndShadows_Quad_Entity", "bufferRendering_Scene_ShadowsChannel_Queue_Entity");

					create_openenv_shadows_channel_rendergraph("bufferRendering_Scene_ShadowsChannel_Queue_Entity");
					//create_openenv_shadows_channel_rendergraph("bufferRendering_Scene_Debug_Queue_Entity");

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
		auto& entityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "sphere_Entity") };
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
				positionmat.translation(-45.0, skydomeInnerRadius + groundLevel + 8.0, -20.0);

				transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
				wp.local_pos = wp.local_pos * positionmat;
			}
		));

		auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
		resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair("sphere", "sphere.ac"), TriangleMeshe()));

		m_sphereEntity = entity;
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


	/////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////// add cameras

	m_perpective_projection.perspective(characteristics_v_width, characteristics_v_height, 1.0, 100000.00000000000);
	m_orthogonal_projection.orthogonal(characteristics_v_width * 200, characteristics_v_height * 200, 1.0, 100000.00000000000);






	/////////////// add shadow map camera 
	
	auto& lookatJointEntityNode{ m_entitygraph.add(m_entitygraph.node(m_appWindowsEntityName), "shadowmap_lookatJoint_Entity") };

	const auto lookatJointEntity{ lookatJointEntityNode.data() };

	auto& lookat_time_aspect{ lookatJointEntity->makeAspect(core::timeAspect::id) };
	auto& lookat_world_aspect{ lookatJointEntity->makeAspect(core::worldAspect::id) };

	lookat_world_aspect.addComponent<transform::WorldPosition>("lookat_output");

	lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_dest", core::maths::Real3Vector(0.0, skydomeInnerRadius + groundLevel, 0.0));

	//lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_localpos", core::maths::Real3Vector(-50.0, skydomeInnerRadius + groundLevel + 250, 1.0));
	lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_localpos");

	lookat_world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
		{
			{"lookatJointAnim.output", "lookat_output"},
			{"lookatJointAnim.dest", "lookat_dest"},
			{"lookatJointAnim.localpos", "lookat_localpos"},

		},
		helpers::animators::makeLookatJointAnimator())
	);



	// add camera

	//helpers::plugCamera(m_entitygraph, m_perpective_projection, "shadowmap_lookatJoint_Entity", "shadowmap_camera_Entity");
	helpers::plugCamera(m_entitygraph, m_orthogonal_projection, "shadowmap_lookatJoint_Entity", "shadowmap_camera_Entity");
	







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

	helpers::plugCamera(m_entitygraph, m_perpective_projection, "gblJoint_Entity", "camera_Entity");

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


	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };


		const std::vector<std::pair<size_t, std::pair<std::string, Texture>>> sphere_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("marbre.jpg", Texture())) };



		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_TexturesChannel_Proxy_Entity",
															"scene_texture1stage_keycolor_vs", "scene_texture1stage_keycolor_ps",
															sphere_rs_list,
															1000,
															sphere_textures) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_ZDepthChannel_Proxy_Entity",
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

	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_ZDepthChannel_Proxy_Entity",
															"scene_zdepth_vs", "scene_zdepth_ps",
															sphere_rs_list,
															1000) };

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_ZDepthChannel_Proxy_Entity",
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

	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_AmbientLightChannel_Proxy_Entity",
															"scene_flatcolor_vs", "scene_flatcolor_ps",
															sphere_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ sphere_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.ambientlight.color", "color"));


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		auto& rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
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
}

void SamplesOpenEnv::create_openenv_lit_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_LitChannel_Proxy_Entity",
															"scene_lit_vs", "scene_lit_ps",
															ground_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ ground_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.light0.dir", "light_dir"));

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

	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_LitChannel_Proxy_Entity",
															"scene_lit_vs", "scene_lit_ps",
															sphere_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ sphere_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("std.light0.dir", "light_dir"));

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

	}

	///////////////	add tree

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "none");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> tree_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const std::vector< std::pair<size_t, std::pair<std::string, Texture>>> tree_textures{ std::make_pair(Texture::STAGE_0, std::make_pair("tree2_tex.bmp", Texture())) };

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_LitChannel_Proxy_Entity",
															"scene_lit_keycolor_vs", "scene_lit_keycolor_ps",
															tree_rs_list,
															1000,
															tree_textures) };


		/// connect shader arg

		auto& rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("std.light0.dir", "light_dir"));

		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& tree_resource_aspect{ m_treeEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &tree_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ tree_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& tree_world_aspect{ m_treeEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &tree_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ tree_proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);
	}
}

void SamplesOpenEnv::create_openenv_emissive_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_EmissiveChannel_Proxy_Entity",
															"scene_flatcolor_vs", "scene_flatcolor_ps",
															ground_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ ground_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("black_color", "color"));


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

		const auto clouds_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "clouds_EmissiveChannel_Proxy_Entity",
															"scene_flatcolor_keycolor_vs", "scene_flatcolor_keycolor_ps",
															clouds_rs_list,
															999,
															clouds_textures) };

		/// connect shaders args

		auto& rendering_aspect{ clouds_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_emissive_color", "color"));

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


		const auto skydome_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "skydome_EmissiveChannel_Proxy_Entity",
									"scene_flatcolor_vs", "scene_flatcolor_ps",
									skydome_rs_list,
									900
									) };

		/// connect shaders args

		auto& rendering_aspect{ skydome_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("skydome_emissive_color", "color"));


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

	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear_uvwrap");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_EmissiveChannel_Proxy_Entity",
															"scene_flatcolor_vs", "scene_flatcolor_ps",
															sphere_rs_list,
															1000) };


		/// connect shader arg

		auto& rendering_aspect{ sphere_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("black_color", "color"));



		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_EmissiveChannel_Proxy_Entity",
															"scene_flatcolor_keycolor_vs", "scene_flatcolor_keycolor_ps",
															tree_rs_list,
															1000,
															tree_textures) };

		/// connect shaders args

		auto& rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		drawingControl.pshaders_map.push_back(std::make_pair("black_color", "color"));

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
}

void SamplesOpenEnv::create_openenv_shadows_channel_rendergraph(const std::string& p_queueEntityId)
{
	// get ptr on rendered shadow map
	
	auto& shadowMapNode{ m_entitygraph.node("shadowMap_Texture_Entity") };
	const auto shadowmap_texture_entity{ shadowMapNode.data() };

	auto& sm_resource_aspect{ shadowmap_texture_entity->aspectAccess(core::resourcesAspect::id) };

	std::pair<size_t, Texture>* sm_texture_ptr{ &sm_resource_aspect.getComponent<std::pair<size_t, Texture>>("standalone_rendering_target_texture")->getPurpose() };



	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_ShadowsChannel_Proxy_Entity",
															"scene_shadowsmask_vs", "scene_shadowsmask_ps",
															ground_rs_list,
															1000) };

		
		auto& resource_aspect{ ground_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		resource_aspect.addComponent<std::pair<size_t, Texture>*>("texture_ref", sm_texture_ptr);
		

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

	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_ShadowsChannel_Proxy_Entity",
															"scene_shadowsmask_vs", "scene_shadowsmask_ps",
															sphere_rs_list,
															1000) };



		auto& resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		resource_aspect.addComponent<std::pair<size_t, Texture>*>("texture_ref", sm_texture_ptr);



		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& sphere_resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &sphere_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_ShadowsChannel_Proxy_Entity",
															"scene_shadowsmask_vs", "scene_shadowsmask_ps",
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
		/*
		auto& tree_rendering_aspect{ tree_proxy_entity->aspectAccess(core::renderingAspect::id) };

		rendering::DrawingControl& drawingControl{ tree_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
		drawingControl.pshaders_map.push_back(std::make_pair("texture_keycolor_ps.key_color", "key_color"));
		*/
	}
	
}

void SamplesOpenEnv::create_openenv_shadowmap_channel_rendergraph(const std::string& p_queueEntityId)
{
	///////////////	add ground

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> ground_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto ground_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "ground_ShadowMapChannel_Proxy_Entity",
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


	///////////////	add sphere

	{
		RenderState rs_noculling(RenderState::Operation::SETCULLING, "cw");
		RenderState rs_zbuffer(RenderState::Operation::ENABLEZBUFFER, "true");
		RenderState rs_fill(RenderState::Operation::SETFILLMODE, "solid");
		RenderState rs_texturepointsampling(RenderState::Operation::SETTEXTUREFILTERTYPE, "linear");

		RenderState rs_alphablend(RenderState::Operation::ALPHABLENDENABLE, "false");

		const std::vector<RenderState> sphere_rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling, rs_alphablend };

		const auto sphere_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "sphere_ShadowMapChannel_Proxy_Entity",
															"scene_zdepth_vs", "scene_zdepth_ps",
															sphere_rs_list,
															1000) };


		//////////////////////////////////////////////////////////////////////

		// link triangle meshe to related entity in scenegraph side 
		auto& resource_aspect{ m_sphereEntity->aspectAccess(core::resourcesAspect::id) };
		std::pair<std::pair<std::string, std::string>, TriangleMeshe>* meshe_ref{ &resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		auto& proxy_resource_aspect{ sphere_proxy_entity->aspectAccess(core::resourcesAspect::id) };
		proxy_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);


		///////////////////////////////////////////////////////////////////////

		// link transforms to related entity in scenegraph side 
		auto& world_aspect{ m_sphereEntity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_world_aspect{ sphere_proxy_entity->makeAspect(core::worldAspect::id) };
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

		const auto tree_proxy_entity{ helpers::plugRenderingProxyEntity(m_entitygraph, p_queueEntityId, "tree_ShadowMapChannel_Proxy_Entity",
															"scene_zdepth_vs", "scene_zdepth_ps",
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