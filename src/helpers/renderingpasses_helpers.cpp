
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

#include "renderingpasses_helpers.h"
#include "entitygraph_helpers.h"

#include "entitygraph.h"
#include "entity.h"
#include "aspects.h"

#include "trianglemeshe.h"

using namespace mage::core;
using namespace mage::helpers;


void RenderingPasses::registerPass(const std::string& p_id, const std::string& queue_entity_id)
{
	PassConfig pc;
	pc.queue_entity_id = queue_entity_id;

	m_configs_table.emplace(p_id, pc);
}

PassConfig RenderingPasses::getPassConfig(const std::string& p_id) const
{
	return m_configs_table.at(p_id);
}

std::unordered_map<std::string, Entity*> RenderingPasses::registerToPasses(mage::core::Entitygraph& p_entitygraph,
									mage::core::Entity* p_entity,
									const std::unordered_map<std::string, PassConfig> p_config,
									const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>& p_vertex_shaders_params,
									const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>& p_pixel_shaders_params)
{
	std::unordered_map<std::string, core::Entity*> proxy_entities;

	const std::string base_entity_id{ p_entity->getId() };

	for (const auto& e : p_config)
	{
		std::string proxy_entity_name = base_entity_id + std::string("_") + e.first + std::string("_ProxyEntity");

		const auto proxy_entity{ helpers::plugRenderingProxyEntity(p_entitygraph, 
																e.second.queue_entity_id, 
																proxy_entity_name,
																e.second.vshader,
																e.second.pshader,
																e.second.rs_list,
																e.second.rendering_order,
																e.second.textures_files_list) };

		///// plug textures references if any
		auto& proxy_entity_resource_aspect{ proxy_entity->aspectAccess(core::resourcesAspect::id) };

		int counter{ 0 };
		for (const auto& t : e.second.textures_ptr_list)
		{
			std::string comp_id{ "texture_ref" + std::to_string(counter++) };
			proxy_entity_resource_aspect.addComponent<std::pair<size_t, Texture>*>(comp_id, t);
		}

		
		///// link triangle meshe to related entity in scenegraph side 
		const auto& base_entity_resource_aspect{ p_entity->aspectAccess(core::resourcesAspect::id) };
		auto* meshe_ref{ &base_entity_resource_aspect.getComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe")->getPurpose() };

		proxy_entity_resource_aspect.addComponent<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>("meshe_ref", meshe_ref);

		
		///// link transforms to related entity in scenegraph side 

		const auto& base_entity_world_aspect{ p_entity->aspectAccess(core::worldAspect::id) };
		transform::WorldPosition* position_ref{ &base_entity_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };

		auto& proxy_entity_world_aspect{ proxy_entity->makeAspect(core::worldAspect::id) };
		proxy_entity_world_aspect.addComponent<transform::WorldPosition*>("position_ref", position_ref);

		// if animation aspect in entity, connect vshader

		if (p_entity->hasAspect(core::animationsAspect::id))
		{
			std::pair<std::string, Shader>* vshader_ref{ &proxy_entity_resource_aspect.getComponent<std::pair<std::string, Shader>>("vertexShader")->getPurpose() };

			auto& entity_animations_aspect{ p_entity->aspectAccess(core::animationsAspect::id) };
			entity_animations_aspect.getComponent<std::vector<std::pair<std::string, Shader>*>>("target_vshaders")->getPurpose().push_back(vshader_ref);
		}

		proxy_entities[e.first] = proxy_entity;
	}

	///// manage shader args

	for (const auto& e : p_vertex_shaders_params)
	{
		const std::string channel_id{ e.first };

		if (proxy_entities.count(channel_id))
		{
			const auto& rendering_aspect{ proxy_entities.at(channel_id)->aspectAccess(core::renderingAspect::id)};
			rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

			const auto& args_cnx{ e.second };
			for (const auto& c : args_cnx)
			{
				drawingControl.vshaders_map.push_back(std::make_pair(c.first, c.second));
			}
		}
		else
		{
			_EXCEPTION("Cannot find " + channel_id + " passes config table");
		}
	}

	for (const auto& e : p_pixel_shaders_params)
	{
		const std::string channel_id{ e.first };

		if (proxy_entities.count(channel_id))
		{
			const auto& rendering_aspect{ proxy_entities.at(channel_id)->aspectAccess(core::renderingAspect::id) };
			rendering::DrawingControl& drawingControl{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

			const auto& args_cnx{ e.second };
			for (const auto& c : args_cnx)
			{
				drawingControl.pshaders_map.push_back(std::make_pair(c.first, c.second));
			}
		}
		else
		{
			_EXCEPTION("Cannot find " + channel_id + " passes config table");
		}
	}

	return proxy_entities;
}