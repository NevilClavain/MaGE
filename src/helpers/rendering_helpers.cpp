
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

#include "rendering_helpers.h"
#include "entitygraph_helpers.h"

#include "entitygraph.h"
#include "entity.h"
#include "aspects.h"

using namespace mage::core;
using namespace mage::helpers;

void Rendering::registerPass(const std::string& p_id)
{
	m_configs_table.emplace(p_id, PassConfig());
}

void Rendering::registerPassDefaultConfig(const std::string& p_id, const PassConfig& p_config)
{
	m_configs_table.emplace(p_id, p_config);
}

PassConfig Rendering::getPassConfig(const std::string& p_id) const
{
	return m_configs_table.at(p_id);
}

std::vector<Entity*> Rendering::registerToPasses(mage::core::Entitygraph& p_entitygraph,
									mage::core::Entity* p_entity,
									const std::vector<std::pair<std::string, PassConfig>> p_config,
									const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>& p_vertex_shaders_params,
									const std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>& p_pixel_shaders_params)
{
	std::vector<Entity*> entities;

	const std::string base_entity_id{ p_entity->getId() };

	for (const auto& e : p_config)
	{
		std::string proxy_entity_name = base_entity_id + std::string("_") + e.first + std::string("_ProxyEntity");

		const auto entity{ helpers::plugRenderingProxyEntity(p_entitygraph, 
																e.second.queue_entity_id, 
																proxy_entity_name,
																e.second.vshader,
																e.second.pshader,
																e.second.rs_list,
																e.second.rendering_order,
																e.second.textures_files_list) };

		entities.push_back(entity);


	}
	return entities;
}