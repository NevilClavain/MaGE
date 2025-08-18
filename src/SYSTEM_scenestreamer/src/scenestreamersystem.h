
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
#include <set>
#include <json_struct/json_struct.h>

#include "system.h"

namespace mage
{
    namespace json
    {
        struct BufferTexture
        {
            std::string format_descr; // ex : "TEXTURE_RGB" for Texture::Format::TEXTURE_RGB, "TEXTURE_FLOAT32" for Texture(Texture::Format::TEXTURE_FLOAT32
            int         width;
            int         height;

            JS_OBJ(format_descr, width, height);
        };

        struct StagedBufferTexture
        {
            int             stage_num;
            BufferTexture   buffer_texture;

            JS_OBJ(stage_num, buffer_texture);
        };

        struct RenderingTarget
        {
            std::string                         id;
            std::vector<std::string>            shaders;
            std::vector<StagedBufferTexture>    inputs;
            int                                 target_stage_num;

            std::vector<RenderingTarget>        sub_rendering_targets;
            
            // also : sub scenes rendering vector

            JS_OBJ(id, shaders, inputs, target_stage_num);
        };
    }

    //fwd decl
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
   
    class SceneStreamerSystem : public core::System
    {
    public:
        SceneStreamerSystem() = delete;
        SceneStreamerSystem(core::Entitygraph& p_entitygraph);
        ~SceneStreamerSystem() = default;

        void run();


    private:

    
    };
}
