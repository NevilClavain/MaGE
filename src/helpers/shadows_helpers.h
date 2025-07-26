
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

#pragma once

#include <vector>
#include <string>

#include "maths_helpers.h"
#include "matrix.h"
#include "tvector.h"

#include "renderingpasses_helpers.h"

namespace mage
{
	// fwd decl
	namespace core
	{
		class Entity;
		class Entitygraph;
	}

	namespace helpers
	{

		struct ShadowSourceEntity
		{
			mage::core::Entity* entity;
			PassesDescriptors   passesDescriptors;
		};

		struct ShadowsRenderingParams
		{
			mage::core::maths::Matrix		orthogonal_projection;

			int								shadowmap_resol;

			std::string						queue_to_move;
			std::string						rootpass_queue;
			std::string						combiner_entities_prefix;
			std::string						shadows_scene_entity_id;
			std::string						shadowmap_scene_entity_id;
			std::string						shadowmap_target_entity_id;
			std::string						shadowmap_lookatJoint_entity_id;
			std::string						shadowmap_camera_entity_id;

			mage::core::maths::Real3Vector	shadowmap_camerajoint_lookat_localpos;
			mage::core::maths::Real3Vector	shadowmap_camerajoint_lookat_dest;
		};

		void updateShadowMapDirection(mage::core::Entity* p_shadowmap_lookatJoint_Entity, 
										const mage::core::maths::Real4Vector& p_light_vector, 
										const mage::core::maths::Real4Vector& p_base_vector, 
										double p_vectorscale);

		void install_shadows_renderer_queues(mage::core::Entitygraph& p_entitygraph,
										int p_w_width, int p_w_height,
										float p_characteristics_v_width, float p_characteristics_v_height,
										int p_shadowmap_resol,
										const std::string& p_queue_to_move,
										const std::string& p_rootpass_queue,
										const std::string& p_combiner_entities_prefix,
										const std::string& p_shadows_scene_entity_id,
										const std::string& p_shadowmap_scene_entity_id,
										const std::string& p_shadowmap_target_entity_id
										);

		void install_shadows_rendering(mage::core::Entitygraph& p_entitygraph,
										int p_w_width, int p_w_height,
										float p_characteristics_v_width, float p_characteristics_v_height,
										const std::string p_appwindows_entityname,
										const ShadowsRenderingParams& p_shadows_rendering_params
										);
	}
}
