
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

#include <utility> 

#include "shadows_helpers.h"
#include "entitygraph_helpers.h"
#include "animators_helpers.h"

#include "animatorfunc.h"

#include "entitygraph.h"
#include "entity.h"
#include "aspects.h"

#include "texture.h"


void mage::helpers::updateShadowMapDirection(mage::core::Entity* p_shadowmap_lookatJoint_Entity, 
											const mage::core::maths::Real3Vector& p_light_vector, 
											const mage::core::maths::Real3Vector& p_base_vector, 
											double p_vectorscale)
{	
	auto light_cartesian{ p_light_vector };
	light_cartesian.normalize();
	light_cartesian.scale(p_vectorscale);

	auto& lookat_world_aspect{ p_shadowmap_lookatJoint_Entity->aspectAccess(core::worldAspect::id) };

	core::maths::Real3Vector& lookat_localpos{ lookat_world_aspect.getComponent<core::maths::Real3Vector>("lookat_localpos")->getPurpose() };

	lookat_localpos[0] = -light_cartesian[0] + p_base_vector[0];
	lookat_localpos[1] = -light_cartesian[1] + p_base_vector[1];
	lookat_localpos[2] = -light_cartesian[2] + p_base_vector[2];
}

void mage::helpers::install_shadows_renderer_queues(mage::core::Entitygraph& p_entitygraph, 
													int p_w_width, int p_w_height,
													float p_characteristics_v_width, float p_characteristics_v_height,
													int p_shadowmap_resol,
													const std::string& p_queue_to_move,
													const std::string& p_rootpass_queue,
													const std::string& p_combiner_entities_prefix,
													const std::string& p_shadows_scene_entity_id,
													const std::string& p_shadowmap_scene_entity_id,
													const std::string& p_shadowmap_target_entity_id)
{
	const std::string combiner_queue_entity_id = p_combiner_entities_prefix + "_Queue_Entity";
	const std::string combiner_quad_entity_id = p_combiner_entities_prefix + "_Quad_Entity";
	const std::string combiner_view_entity_id = p_combiner_entities_prefix + "_View_Entity";


	/////////////////////////////////////////////////////////////////////////////////////
	/////// update rendering graph : queue hierarchy

	const auto combiner_modulatelitshadows_inputA_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };
	const auto combiner_modulatelitshadows_inputB_channnel{ Texture(Texture::Format::TEXTURE_RGB, p_w_width, p_w_height) };

	mage::helpers::plugRenderingQuad(p_entitygraph,
		"mod_lit_shadows_queue",
		p_characteristics_v_width, p_characteristics_v_height,
		p_rootpass_queue,
		combiner_queue_entity_id,
		combiner_quad_entity_id,
		combiner_view_entity_id,

		"combiner_modulate_vs",
		"combiner_modulate_ps",
		{
			std::make_pair(Texture::STAGE_0, combiner_modulatelitshadows_inputA_channnel),
			std::make_pair(Texture::STAGE_1, combiner_modulatelitshadows_inputB_channnel),
		},
		Texture::STAGE_2);


	// channel : shadow

	rendering::Queue shadowsChannelRenderingQueueDef("shadows_channel_queue");
	shadowsChannelRenderingQueueDef.setTargetClearColor({ 0, 0, 0, 255 });
	shadowsChannelRenderingQueueDef.enableTargetClearing(true);
	shadowsChannelRenderingQueueDef.enableTargetDepthClearing(true);
	shadowsChannelRenderingQueueDef.setTargetStage(Texture::STAGE_1);

	mage::helpers::plugRenderingQueue(p_entitygraph, shadowsChannelRenderingQueueDef, combiner_quad_entity_id, p_shadows_scene_entity_id);


	auto& lit_channel_queue_entity_node{ p_entitygraph.node(p_queue_to_move) };

	const auto lit_channel_queue_entity{ lit_channel_queue_entity_node.data() };

	auto& renderingAspect{ lit_channel_queue_entity->aspectAccess(core::renderingAspect::id) };
	auto& stored_rendering_queue{ renderingAspect.getComponent<rendering::Queue>("renderingQueue")->getPurpose() };

	rendering::Queue litChannelRenderingQueue("lit_channel_queue");
	litChannelRenderingQueue.setTargetClearColor({ 0, 0, 0, 255 });
	litChannelRenderingQueue.enableTargetClearing(true);
	litChannelRenderingQueue.enableTargetDepthClearing(true);
	litChannelRenderingQueue.setTargetStage(Texture::STAGE_0);
	stored_rendering_queue = litChannelRenderingQueue;

	p_entitygraph.move_subtree(p_entitygraph.node(combiner_quad_entity_id), lit_channel_queue_entity_node);


	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// SHADOWMAP TARGET TEXTURE
	// Plug shadowmap just bellow, to be sure shadowmap is rendered before shadosw mask PASS above (remind : leaf to root order)

	helpers::plugTargetTexture(p_entitygraph, p_shadows_scene_entity_id, p_shadowmap_target_entity_id, std::make_pair(Texture::STAGE_0, Texture(Texture::Format::TEXTURE_FLOAT32, p_shadowmap_resol, p_shadowmap_resol)));

	rendering::Queue shadowMapChannelRenderingQueueDef("shadowmap_channel_queue");
	shadowMapChannelRenderingQueueDef.setTargetClearColor({ 255, 255, 255, 255 });
	shadowMapChannelRenderingQueueDef.enableTargetClearing(true);
	shadowMapChannelRenderingQueueDef.enableTargetDepthClearing(true);
	shadowMapChannelRenderingQueueDef.setTargetStage(Texture::STAGE_0);

	mage::helpers::plugRenderingQueue(p_entitygraph, shadowMapChannelRenderingQueueDef, p_shadowmap_target_entity_id, p_shadowmap_scene_entity_id);
}

void mage::helpers::install_shadows_rendering(mage::core::Entitygraph& p_entitygraph,
												int p_w_width, int p_w_height,
												float p_characteristics_v_width, float p_characteristics_v_height,
												const std::string p_appwindows_entityname,
												std::vector<std::pair<std::string, core::maths::Real3Vector>>& p_shadowmap_joints_list,
												const ShadowsRenderingParams& p_shadows_rendering_params)
{
	////// step I : create shadow map camera

	auto& lookatJointEntityNode{ p_entitygraph.add(p_entitygraph.node(p_appwindows_entityname), "shadowmap_lookatJoint_Entity") };

	const auto lookatJointEntity{ lookatJointEntityNode.data() };

	auto& lookat_time_aspect{ lookatJointEntity->makeAspect(core::timeAspect::id) };
	auto& lookat_world_aspect{ lookatJointEntity->makeAspect(core::worldAspect::id) };

	lookat_world_aspect.addComponent<transform::WorldPosition>("lookat_output");
	lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_dest", p_shadows_rendering_params.shadowmap_camerajoint_lookat_dest);
	lookat_world_aspect.addComponent<core::maths::Real3Vector>("lookat_localpos");

	lookat_world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
		{
			{"lookatJointAnim.output", "lookat_output"},
			{"lookatJointAnim.dest", "lookat_dest"},
			{"lookatJointAnim.localpos", "lookat_localpos"},

		},
		helpers::makeLookatJointAnimator())
	);

	helpers::plugCamera(p_entitygraph, p_shadows_rendering_params.orthogonal_projection, p_shadows_rendering_params.shadowmap_lookatJoint_entity_id, p_shadows_rendering_params.shadowmap_camera_entity_id);

	p_shadowmap_joints_list.push_back(std::make_pair(p_shadows_rendering_params.shadowmap_lookatJoint_entity_id, 
														p_shadows_rendering_params.shadowmap_camerajoint_lookat_localpos_base));


	/////// II : update rendering graph

	install_shadows_renderer_queues(p_entitygraph,
		p_w_width, p_w_height,
		p_characteristics_v_width, p_characteristics_v_height,
		p_shadows_rendering_params.shadowmap_resol,
		"bufferRendering_Scene_LitChannel_Queue_Entity",
		"bufferRendering_Combiner_Accumulate_Quad_Entity",
		"bufferRendering_Combiner_ModulateLitAndShadows",
		"bufferRendering_Scene_ShadowsChannel_Queue_Entity",
		"bufferRendering_Scene_ShadowMapChannel_Queue_Entity",
		"shadowMap_Texture_Entity"
	);

	/////// III : entities in rendering graph

}