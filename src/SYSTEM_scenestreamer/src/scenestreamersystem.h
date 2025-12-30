
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
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <memory>

#include <json_struct/json_struct.h>

#include "eventsource.h"

#include "system.h"
#include "matrix.h"
#include "tvector.h"

#include "syncvariable.h"

#include "matrixfactory.h"
#include "xtree.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"


namespace mage
{
    //fwd decl
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
    namespace core { class ComponentContainer; }

    enum class SceneStreamerSystemEvent
    {
        RENDERING_ENABLED,
        RENDERING_DISABLED,
    };

    namespace json
    {
        // JSON struct for rendergraph part

        static constexpr int fillWithWindowDims{ -1 };
        static constexpr int fillWithViewportDims{ -1 };

        struct BufferTexture
        {
            std::string format_descr; // ex : "TEXTURE_RGB" for Texture::Format::TEXTURE_RGB, "TEXTURE_FLOAT32" for Texture(Texture::Format::TEXTURE_FLOAT32
            int         width{ fillWithWindowDims };
            int         height{ fillWithWindowDims };

            JS_OBJ(format_descr, width, height);
        };

        struct StagedBufferTexture
        {
            int             stage{ 0 };
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

        struct RGBA8
        {
            int r;
            int g;
            int b;
            int a;

            JS_OBJ(r, g, b, a);
        };

        // fwd decl
        struct RendergraphNode;

        struct Target
        {
            std::string                         descr;

            int                                 width{ fillWithViewportDims };
            int                                 height{ fillWithViewportDims };

            std::vector<ShaderWithArgs>         shaders;

            std::vector<StagedBufferTexture>    inputs;
            int                                 destination_stage{ -1 };

            std::vector<RendergraphNode>        subs;

            JS_OBJ(descr, width, height, shaders, inputs, destination_stage, subs);
        };

        struct Queue
        {
            std::string                         id;

            std::string                         rendering_channel_type;

            bool                                target_clear;
            RGBA8                               target_clear_color;            
            bool                                target_depth_clear;
            int                                 target_stage{ -1 };

            JS_OBJ(id, rendering_channel_type, target_clear, target_clear_color, target_depth_clear, target_stage);
        };

        struct RendergraphNode
        {
            Target target;
            Queue  queue;

            JS_OBJ(target, queue);
        };

        struct RendergraphNodesCollection
        {
            std::vector<RendergraphNode>     subs;

            JS_OBJ(subs);
        };

        ///////////////////////////////////////////////////////////////////

        struct ViewGroup
        {
            std::string name;

            std::vector<std::string> queue_entities;

            JS_OBJ(name, queue_entities);
        };

        ///////////////////////////////////////////////////////////////////

        // JSON struct for scenegraph part

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

            JS_OBJ(type, 
                        x_syncvar_value, y_syncvar_value, z_syncvar_value, 
                        xyzw_datacloud_value, xyz_datacloud_value, x_datacloud_value, y_datacloud_value, z_datacloud_value, w_datacloud_value,
                        xyzw_direct_value, xyz_direct_value, x_direct_value, y_direct_value, z_direct_value, w_direct_value);
        };

        struct Animator
        {            
            std::string                 descr;
            std::string                 helper;

            std::vector<MatrixFactory>  matrix_factory_chain;

            JS_OBJ(descr, helper, matrix_factory_chain);
        };

        struct WorldAspect
        {            
            // world aspect can have only 1 animator
            Animator animator;
            JS_OBJ(animator);
        };

        struct Meshe
        {            
            std::string                         descr;
            std::string                         filename;
            std::string                         meshe_id;

            JS_OBJ(descr, filename, meshe_id);
        };

        struct TextureFile
        {
            int             stage{ -1 };
            std::string     filename;

            JS_OBJ(stage, filename);
        };

        struct RenderState
        {
            std::string operation;
            std::string argument;

            JS_OBJ(operation, argument);
        };

        struct ChannelConfig
        {           
            std::string	                rendering_channel_type;
            std::vector<RenderState>	rs_list;

            int	                        rendering_order{ -1 };

            std::vector<TextureFile>    textures_files_list;

            std::string                 vshader;
            std::string                 pshader;

            JS_OBJ(rendering_channel_type, rs_list, rendering_order, textures_files_list, vshader, pshader);
        };

        struct ShaderParam
        {          
            std::string datacloud_name;
            std::string param_name;

            JS_OBJ(datacloud_name, param_name);
        };

        struct PassShadersParams
        {           
            std::string	                rendering_channel_type;
            std::vector<ShaderParam>    shaders_params;

            JS_OBJ(rendering_channel_type, shaders_params);
        };

        struct Channels
        {           
            std::vector<ChannelConfig>      configs;

            std::vector<PassShadersParams>  vertex_shaders_params;
            std::vector<PassShadersParams>  pixel_shaders_params;

            JS_OBJ(configs, vertex_shaders_params, pixel_shaders_params);
        };

        struct ResourceAspect
        {            
            Meshe meshe;

            JS_OBJ(meshe);
        };

        struct ScenegraphEntity
        {            
            std::string                 id;
            WorldAspect                 world_aspect;
            ResourceAspect              resource_aspect;
            std::string                 helper;
            Channels                    channels;

            std::vector<ScenegraphEntity> subs;
            
            JS_OBJ(id, world_aspect, resource_aspect, helper, channels, subs);
        };

        struct ScenegraphEntitiesCollection
        {            
            std::vector<ScenegraphEntity>     subs;

            JS_OBJ(subs);
        };

        ///////////////////////////////////

        struct FileArgument
        {
            std::string key;
            std::string value;

            JS_OBJ(key, value);
        };

        struct ScenegraphNode
        {
            std::string                 file;

            std::vector<FileArgument>   file_args;

            std::vector<std::string>    tags;

            std::vector<std::string>    rendergraph_parts;

            Animator                    animator;
            
            JS_OBJ(file, file_args, tags, rendergraph_parts, animator);
        };

        struct Scenegraph
        {
            std::vector<ScenegraphNode> entities;
            JS_OBJ(entities);
        };

    } // json

    class EntityRendering
    {
    public:
        EntityRendering() = default;
        
        EntityRendering(const json::Channels& p_channels) :
            m_channels(p_channels)
        {
        }

        bool isRendered() const
        {
            return m_rendered;
        }

    private:
        json::Channels  m_channels;
        bool            m_request_rendering         { false };
        bool            m_rendered                  { false }; // if true, passes are actually mapped in rendergraph side and so entity is normally rendered

        friend class SceneStreamerSystem;
    };
   
    class SceneStreamerSystem : public mage::core::System, public mage::property::EventSource<SceneStreamerSystemEvent, const std::string&>
    {
    private:

        // node for quadtree
        struct SceneQuadTreeNode
        {
            double                                              side_length{ 0 };
            core::maths::Real2Vector                            position;

            core::maths::Real2Vector                            xz_min;
            core::maths::Real2Vector                            xz_max;

            std::unordered_set<mage::core::Entity*>             entities;
        };

        struct XTreeEntity
        {
            core::Entity* entity{ nullptr };
            core::QuadTreeNode<SceneQuadTreeNode>*              quadtree_node{ nullptr };
            bool is_static{ false };
        };


        // regroup main infos related to a rendergraph part:
        // 
        //  > associated viewgroup
        //  > xtree stuff

        struct RendergraphPartData
        {
            json::ViewGroup                                                                        viewgroup;

            std::unique_ptr<core::QuadTreeNode<SceneQuadTreeNode>>                                 quadtree_root;
            // regrouping here all entities dispatched in m_xtree_root above
            std::unordered_map<std::string, XTreeEntity>                                           xtree_entities;
        };


    public:

        struct Configuration
        {
            double      scene_size                  { 5000 };
            int         xtree_max_depth             { 6 };
            int         max_neighbourood_depth      { 2 };
            double      object_xtreenode_ratio      { 0.1 };
        };

        SceneStreamerSystem() = delete;
        SceneStreamerSystem(core::Entitygraph& p_entitygraph);
        ~SceneStreamerSystem() = default;

        void run();

        void configure(const Configuration& p_config);

        void buildRendergraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId,
                                    int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height);


        void buildScenegraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId, const mage::core::maths::Matrix p_perspective_projection);


        void buildScenegraphEntity(const std::string& p_jsonsource, const std::vector<std::string>& p_rendergraph_parts, const json::Animator& p_animator, const std::vector<std::string>& p_tags, const std::string& p_parentEntityId,
                                    const mage::core::maths::Matrix p_perspective_projection, const std::unordered_map<std::string, std::string> p_file_args);


        void buildViewgroup(const std::string& p_jsonsource, int p_renderingQueueSystemSlot);

        void setViewgroupMainview(const std::string& p_vg_id, const std::string& p_mainview);

        void requestEntityRendering(const std::string& p_entity_id, bool p_render_it);


        void dumpXTree();
        void dumpXTreeEntities();

        void enableSystem(bool p_enabled);

    private:


        bool is_inside_quadtreenode(const SceneQuadTreeNode& p_qtn, const core::maths::Matrix& p_global_pos);

        void register_to_queues(const json::Channels& p_channels, mage::core::Entity* p_entity);

        void unregister_from_queues(mage::core::Entity* p_entity);

        void init_XTree(RendergraphPartData& p_rgpd);

        void update_XTree(core::QuadTreeNode<SceneQuadTreeNode>* p_xtree_root, std::unordered_map<std::string, XTreeEntity>& p_xtree_entities);
        void check_XTree(core::QuadTreeNode<SceneQuadTreeNode>* p_xtree_root, std::unordered_map<std::string, XTreeEntity>& p_xtree_entities, const json::ViewGroup& p_viewgroup);

        bool                                                                                    m_enabled{ false };

        mutable mage::core::logger::Sink                                                        m_localLogger;


        ///////////////////////////// TABLES
        std::unordered_map<std::string, mage::core::Entity*>                                    m_scene_entities;

        std::unordered_map<std::string, std::unordered_map<std::string, mage::core::Entity*>>   m_rendering_proxies; // i.e rendering_entites ;-)

        std::unordered_map<std::string, std::unordered_set<std::string>>                        m_scene_entities_rg_parts; // rendergraph parts for each scene entity (defined in json as "rendergraph_parts" array)

        std::unordered_map<std::string, EntityRendering>                                        m_entity_renderings;        // entities rendering infos (channels, etc...)

        std::unordered_map<std::string, RendergraphPartData>                                    m_rendergraphpart_data;

        std::unordered_set<mage::core::Entity*>                                                 m_found_entities_to_render;   // entities actually rendered
        /////////////////////////////////
      
        Configuration                                                                           m_configuration;
        bool                                                                                    m_configured{ false };


        int                                                                                     m_renderingQueueSystemSlot{ -1 };
        
        void register_scene_entity(mage::core::Entity* p_entity);

        core::SyncVariable build_syncvariable_fromjson(const json::SyncVariable& p_syncvar);

        mage::transform::MatrixFactory process_matrixfactory_fromjson(const json::MatrixFactory& p_json_matrix_factory, mage::core::ComponentContainer& p_world_aspect, mage::core::ComponentContainer& p_time_aspect);

        std::string filter_arguments_stack(const std::string p_input, const std::unordered_map<std::string, std::string> p_file_args);
    };
}
