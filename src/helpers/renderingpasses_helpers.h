
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
#include <unordered_map>
#include <utility>  

#include "singleton.h"
#include "renderstate.h"
#include "texture.h"

namespace mage
{
	//fwd decl
	namespace core
	{
		class Entitygraph;
		class Entity;
	};

	namespace helpers
	{
		struct PassConfig
		{
			std::string	queue_entity_id;
			std::vector<rendering::RenderState>	rs_list
			{
				{ rendering::RenderState::Operation::SETCULLING, "cw" },
				{ rendering::RenderState::Operation::ENABLEZBUFFER, "true" },
				{ rendering::RenderState::Operation::SETFILLMODE, "solid" },
				{ rendering::RenderState::Operation::SETTEXTUREFILTERTYPE, "linear" },
				{ rendering::RenderState::Operation::ALPHABLENDENABLE, "false" }
			};

			int	rendering_order{ 1000 };

			std::vector<std::pair<size_t, std::pair<std::string, Texture>>> textures_files_list;
			std::vector<std::pair<size_t, Texture>*>						textures_ptr_list;

			std::string vshader;
			std::string pshader;
		};

		struct PassesDescriptors
		{
			std::unordered_map< std::string, PassConfig>										configs;
			std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>	vertex_shaders_params;
			std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>>	pixel_shaders_params;
		};

		// rendering passes helper struct
		struct RenderingPasses : public property::Singleton<RenderingPasses>
		{
		public:
			RenderingPasses() = default;
			~RenderingPasses() = default;

			void registerPass(const std::string& p_queue_entity_id);

			PassConfig getPassConfig(const std::string& p_queue_entity_id) const;

			std::unordered_map<std::string, core::Entity*> registerToPasses(mage::core::Entitygraph& p_entitygraph,
									mage::core::Entity* p_entity, 
									const PassesDescriptors& p_passesdescriptors);

		private:

			std::unordered_map<std::string, PassConfig> m_configs_table;

		};
	}
}
