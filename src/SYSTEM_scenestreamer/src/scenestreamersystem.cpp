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

#include <string>
#include <unordered_map>
#include <sstream>  
#include <utility>

#include <json_struct/json_struct.h>

#include "scenestreamersystem.h"

#include "entity.h"
#include "entitygraph.h"
#include "aspects.h"
#include "ecshelpers.h"
#include "exceptions.h"
#include "texture.h"
#include "entitygraph_helpers.h"


using namespace mage;
using namespace mage::core;

SceneStreamerSystem::SceneStreamerSystem(Entitygraph& p_entitygraph) : System(p_entitygraph)
{
}

void SceneStreamerSystem::run()
{
}

void SceneStreamerSystem::buildRendergraphPart(const std::string& p_jsonsource, const std::string p_parentEntityId,
                                                int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height)
{
    json::RenderingTargetNodesCollection rtc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(rtc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse rendergraph: " + errorStr);
    }


    for (const auto& rt : rtc.subs)
    {
        const std::unordered_map<std::string, mage::Texture::Format> texture_format_translation
        {
            { "TEXTURE_RGB", mage::Texture::Format::TEXTURE_RGB },
            { "TEXTURE_FLOAT", mage::Texture::Format::TEXTURE_FLOAT },
            { "TEXTURE_FLOAT32", mage::Texture::Format::TEXTURE_FLOAT32 },
            { "TEXTURE_FLOATVECTOR", mage::Texture::Format::TEXTURE_FLOATVECTOR },
            { "TEXTURE_FLOATVECTOR32", mage::Texture::Format::TEXTURE_FLOATVECTOR32 },
        };

        std::vector<std::pair<size_t, Texture>> inputs;

        for (const auto& input : rt.inputs)
        {
            int final_w_width = (fillWithWindowDims == input.buffer_texture.width ? p_w_width : input.buffer_texture.width);
            int final_h_width = (fillWithWindowDims == input.buffer_texture.height ? p_w_height : input.buffer_texture.height);

            const auto input_channnel{ Texture(texture_format_translation.at(input.buffer_texture.format_descr), final_w_width, final_h_width) };

            inputs.push_back(std::make_pair(input.stage, input_channnel));
        }

        int final_v_width = (fillWithViewportDims == rt.width ? p_w_width : p_characteristics_v_width);
        int final_v_height = (fillWithViewportDims == rt.height ? p_w_height : p_characteristics_v_height);

        const std::string queue_name{ rt.descr + "_queue" };

        const std::string entity_queue_name{ rt.descr + "Buffer_queue" };
        const std::string entity_target_name{ rt.descr + "Buffer_targetquad" };
        const std::string entity_view_name{ rt.descr + "Buffer_view" };

        mage::helpers::plugRenderingTarget(m_entitygraph,
            queue_name,
            final_v_width, final_v_height,
            p_parentEntityId,
            entity_queue_name,
            entity_target_name,
            entity_view_name,
            rt.shaders.at(0).name,
            rt.shaders.at(1).name,
            inputs,
            rt.target_stage);


        // plug shaders args

        Entity* quad_ent{ m_entitygraph.node(entity_target_name).data() };
        auto& rendering_aspect{ quad_ent->aspectAccess(core::renderingAspect::id) };

        rendering::DrawingControl& dc{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

        const auto& vshader{ rt.shaders.at(0) };
        for (const auto& arg : vshader.args)
        {
            dc.vshaders_map.push_back(std::make_pair(arg.source, arg.destination));
        }

        const auto& pshader{ rt.shaders.at(1) };
        for (const auto& arg : pshader.args)
        {
            dc.vshaders_map.push_back(std::make_pair(arg.source, arg.destination));
        }

        // TODO : RECURSIVE CALL ON "subs"
    }
}

void SceneStreamerSystem::buildScenegraphPart(const std::string& p_jsonsource, const std::string p_parentEntityId)
{
    json::ScenegraphNodesCollection sgc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(sgc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse scenegraph: " + errorStr);
    }

    const std::function<void(const json::ScenegraphNode&, int)> browseHierarchy
    {
        [&](const json::ScenegraphNode& p_node, int depth)
        {

            // recursive call
            for (auto& e : p_node.subs)
            {
                browseHierarchy(e, depth + 1);
            }
        }
    };

    for (const auto& e : sgc.subs)
    {
        browseHierarchy(e, 0);
    }
}