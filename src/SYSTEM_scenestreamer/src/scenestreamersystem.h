
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

#include <json_struct/json_struct.h>

#include "system.h"

namespace mage
{
    static constexpr int fillWithWindowDims{ -1 };
    static constexpr int fillWithViewportDims{ -1 };

    namespace json
    {
        // JSON struct for rendergraph nodes

        struct BufferTexture
        {
            std::string format_descr; // ex : "TEXTURE_RGB" for Texture::Format::TEXTURE_RGB, "TEXTURE_FLOAT32" for Texture(Texture::Format::TEXTURE_FLOAT32
            int         width{ fillWithWindowDims };
            int         height{ fillWithWindowDims };

            JS_OBJ(format_descr, width, height);
        };

        struct StagedBufferTexture
        {
            int             stage;
            BufferTexture   buffer_texture;

            JS_OBJ(stage, buffer_texture);
        };

        struct ShaderArg
        {
            std::string source;
            std::string destination;

            JS_OBJ(source, destination);
        };

        struct ShaderWithArgs
        {
            std::string             name;
            std::vector<ShaderArg>  args;

            JS_OBJ(name, args);
        };

        struct RenderingTargetNode
        {
            std::string                         descr;

            int                                 width{ fillWithViewportDims };
            int                                 height{ fillWithViewportDims };

            std::vector<ShaderWithArgs>         shaders;

            std::vector<StagedBufferTexture>    inputs;
            int                                 target_stage;

            std::vector<RenderingTargetNode>    subs;
            
            // also : sub scenes rendering vector

            JS_OBJ(descr, width, height, shaders, inputs, target_stage, subs);
        };

        struct RenderingTargetNodesCollection
        {
            std::vector<RenderingTargetNode>     subs;
            JS_OBJ(subs);
        };


        ///////////////////////////////////////////////////////////////////

        struct MatrixBuilder
        {
            std::string type;

            std::string datacloud_xyz;

            std::string datacloud_x;
            double x;
            
            std::string datacloud_y;
            double y;
            
            std::string datacloud_z;
            double z;
            
            std::string datacloud_w;
            double w;

            JS_OBJ(type, datacloud_xyz, datacloud_x, x, datacloud_y, y, datacloud_z, z, datacloud_w, w);
        };


        struct WorldAspect
        {
            std::string                 type;

            std::vector<MatrixBuilder>  matrix_chain;

            JS_OBJ(type, matrix_chain);
        };

        struct ScenegraphNode
        {
            std::string                 descr;

            WorldAspect                 world_aspect;

            std::vector<ScenegraphNode> subs;

            JS_OBJ(descr, world_aspect, subs);
        };

        struct ScenegraphNodesCollection
        {
            std::vector<ScenegraphNode>     subs;
            JS_OBJ(subs);
        };
    }

    //fwd decl
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
   
    class SceneStreamerSystem : public mage::core::System
    {
    public:
        SceneStreamerSystem() = delete;
        SceneStreamerSystem(core::Entitygraph& p_entitygraph);
        ~SceneStreamerSystem() = default;

        void run();

        void buildRendergraphPart(const std::string& p_jsonsource, const std::string p_parentEntityId,
                                    int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);


        void buildScenegraphPart(const std::string& p_jsonsource, const std::string p_parentEntityId);

    private:

    
    };
}
