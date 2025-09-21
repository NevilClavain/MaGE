

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

#include <string>
#include <vector>
#include <utility>

#include "worldposition.h"

#include "renderingqueue.h"

namespace mage
{
	// fwd decl
	namespace core
	{
		class Entitygraph;
		class Entity;
		struct SyncVariable;

		namespace maths
		{
			class Matrix;
		}
	}

	namespace rendering
	{
		class RenderState;

		/*
		struct Queue
		{
			struct Text;
		};
		*/
	}

	class Texture;

	namespace helpers
	{
		void logEntitygraph(core::Entitygraph& p_eg, bool p_log_entity_id_only = false);

		// LEGACY, TO REMOVE
		void plugRenderingQuadView(mage::core::Entitygraph& p_entitygraph,
									float p_characteristics_v_width, float p_characteristics_v_height,
									const std::string& p_parentid,
									const std::string& p_quadEntityid,
									const std::string& p_viewEntityid,
									mage::rendering::Queue* p_queue,
									const std::string& p_vshader,
									const std::string& p_pshader,
									const std::vector<std::pair<size_t, Texture>>& p_renderTargets);

		rendering::Queue& plugRenderingQueue(mage::core::Entitygraph& p_entitygraph,
												const rendering::Queue& p_renderingqueue,
												const std::string& p_parentid, const std::string& p_entityid);

		rendering::Queue& plugRenderingTarget( mage::core::Entitygraph& p_entitygraph,
										const std::string& p_queue_debug_name,
										float p_characteristics_v_width, float p_characteristics_v_height, 
										const std::string& p_parentid,
										const std::string& p_queueEntityid,
										const std::string& p_quadEntityid,
										const std::string& p_viewEntityid,
										const std::string& p_vshader,
										const std::string& p_pshader,
										const std::vector<std::pair<size_t, Texture>>& p_inputs,
										size_t p_target_stage);

		core::Entity* plugCamera(mage::core::Entitygraph& p_entitygraph,
						const core::maths::Matrix& p_projection,
						const std::string& p_parentid, const std::string& p_entityid);

		void updateCameraProjection(mage::core::Entitygraph& p_entitygraph, const std::string& p_entityid, const core::maths::Matrix& p_projection);

		rendering::Queue* getRenderingQueue(mage::core::Entitygraph& p_entitygraph, const std::string& p_entityId);


		core::Entity* plug2DSpriteWithSyncVariables(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_spriteEntityid,
			const double p_spriteWidth,
			const double p_spriteHeight,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::string& p_texture,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			transform::WorldPosition::TransformationComposition p_composition_operation
		);

		core::SyncVariable& getXPosSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);
		core::SyncVariable& getYPosSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);
		core::SyncVariable& getZRotSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);

		core::Entity* plug2DSpriteWithPosition(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_spriteEntityid,
			const double p_spriteWidth,
			const double p_spriteHeight,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::string& p_texture,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			transform::WorldPosition::TransformationComposition p_composition_operation,
			float p_xpos = 0,
			float p_ypos = 0,
			float p_rot_radians = 0
		);


		core::Entity* plugTextWithPosition(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_textEntityid,
			const mage::rendering::Queue::Text& p_queue_text,
			transform::WorldPosition::TransformationComposition p_composition_operation,
			float p_xpos = 0,
			float p_ypos = 0);


		double& getXPos(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);
		double& getYPos(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);
		double& getZRot(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid);


		core::Entity* plugRenderingProxyEntity(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_entityid,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			const std::vector< std::pair<size_t, std::pair<std::string, Texture>>>& p_textures = {});


		core::Entity* plugTargetTexture(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_entityid,
			const std::pair<size_t, Texture>& p_renderTargetTexture);
	}
}



