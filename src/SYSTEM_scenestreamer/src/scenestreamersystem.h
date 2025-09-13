
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
#include "matrix.h"

#include "syncvariable.h"

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

        struct SyncVariable
        {
            std::string type;
            double      step;
            std::string direction;
            double      initial_value;
            double      min;
            double      max;
            std::string management;
            std::string initial_state;

            JS_OBJ(type, step, direction, initial_value, min, max, management, initial_state);
        };


        struct ScalarDirectValueMatrixSource
        {
            std::string         descr;
            double              value;

            JS_OBJ(descr, value);
        };

        struct Vector3DirectValueMatrixSource
        {
            std::string         descr;
            double              x;
            double              y;
            double              z;

            JS_OBJ(descr, x, y, z);
        };

        struct Vector4DirectValueMatrixSource
        {
            std::string         descr;

            double              x;
            double              y;
            double              z;
            double              w;

            JS_OBJ(descr, x, y, z, w);
        };

        struct SyncVarValueMatrixSource
        {
            std::string         descr;
            SyncVariable        sync_var;

            JS_OBJ(descr, sync_var);
        };

        struct ScalarDatacloudValueMatrixSource
        {
            std::string         descr;
            std::string         var_name;

            JS_OBJ(descr, var_name);
        };

        struct Vector3DatacloudValueMatrixSource
        {
            std::string         descr;
            std::string         var_name;

            JS_OBJ(descr, var_name);
        };

        struct Vector4DatacloudValueMatrixSource
        {
            std::string         descr;
            std::string         var_name;

            JS_OBJ(descr, var_name);
        };

        struct MatrixFactory
        {
            std::string                         type; //"translation", "rotation", "scaling"

            std::string                         descr;

            SyncVarValueMatrixSource            x_syncvar_value;
            SyncVarValueMatrixSource            y_syncvar_value;
            SyncVarValueMatrixSource            z_syncvar_value;
            SyncVarValueMatrixSource            w_syncvar_value;

            Vector4DatacloudValueMatrixSource   xyzw_datacloud_value;
            Vector3DatacloudValueMatrixSource   xyz_datacloud_value;
            ScalarDatacloudValueMatrixSource    x_datacloud_value;
            ScalarDatacloudValueMatrixSource    y_datacloud_value;
            ScalarDatacloudValueMatrixSource    z_datacloud_value;
            ScalarDatacloudValueMatrixSource    w_datacloud_value;

            Vector4DirectValueMatrixSource      xyzw_direct_value;
            Vector3DirectValueMatrixSource      xyz_direct_value;
            ScalarDirectValueMatrixSource       x_direct_value;
            ScalarDirectValueMatrixSource       y_direct_value;
            ScalarDirectValueMatrixSource       z_direct_value;
            ScalarDirectValueMatrixSource       w_direct_value;

            JS_OBJ(type, descr, 
                        x_syncvar_value, y_syncvar_value, z_syncvar_value, 
                        xyzw_datacloud_value, xyz_datacloud_value, x_datacloud_value, y_datacloud_value, z_datacloud_value, w_datacloud_value,
                        xyzw_direct_value, xyz_direct_value, x_direct_value, y_direct_value, z_direct_value, w_direct_value);
        };


        struct Animator
        {
            std::string                 helper;

            MatrixFactory               matrix_factory; // remplacer par plusieurs MatrixFactory attributs nommés selon le animator helper

            JS_OBJ(helper, matrix_factory);
        };


        struct WorldAspect
        {
            // world aspect can have N animators
            std::vector<Animator>       animators;

            JS_OBJ(animators);
        };

        struct ScenegraphNode
        {
            std::string                 descr;

            WorldAspect                 world_aspect;

            std::string                 helper;

            std::vector<ScenegraphNode> subs;

            JS_OBJ(descr, world_aspect, helper, subs);
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

        void buildRendergraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId,
                                    int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);


        void buildScenegraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId, 
                                    const mage::core::maths::Matrix p_perspective_projection);

    private:    

        core::SyncVariable buildSyncVariableFromJson(const json::SyncVariable& p_syncvar);


    };
}
