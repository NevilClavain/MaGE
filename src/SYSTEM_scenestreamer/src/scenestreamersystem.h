
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
#include <algorithm>
#include <random>
#include <limits>
#include <cmath>


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
            int                                 target_stage{ -1 };

            std::vector<RendergraphNode>        subs_nodes;

            JS_OBJ(descr, width, height, shaders, inputs, target_stage, subs_nodes);
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
            std::vector<RendergraphNode>     subs_nodes;

            JS_OBJ(subs_nodes);
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

        struct ValueGenerator
        {
            std::string         type;  // "increment", "rand"

            double              increment_init_value;
            double              increment_step;

            int                 rand_seed;
            double              rand_interval_min;
            std::string         rand_distribution_type;
            std::vector<double> rand_distribution_arguments;

            JS_OBJ(type, increment_init_value, increment_step, rand_seed, rand_interval_min, rand_distribution_type, rand_distribution_arguments);
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


        struct ValueGeneratorSource
        {
            std::string         descr;
            ValueGenerator      value_generator;

            JS_OBJ(descr, value_generator);
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

            ValueGeneratorSource                x_generated_value;
            ValueGeneratorSource                y_generated_value;
            ValueGeneratorSource                z_generated_value;
            ValueGeneratorSource                w_generated_value;


            JS_OBJ(type, x_syncvar_value, y_syncvar_value, z_syncvar_value, w_syncvar_value,
                         x_generated_value, y_generated_value, z_generated_value, w_generated_value,
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
            std::string                     id;
            WorldAspect                     world_aspect;
            ResourceAspect                  resource_aspect;
            std::string                     helper;
            Channels                        channels;

            std::vector<ScenegraphEntity>   subs;
            
            JS_OBJ(id, world_aspect, resource_aspect, helper, channels, subs);
        };

        struct ScenegraphEntitiesCollection
        {            
            std::vector<ScenegraphEntity>               subs;

            JS_OBJ(subs);
        };

        ///////////////////////////////////

        struct FileArgument
        {
            std::string                                 key;
            std::string                                 value;

            JS_OBJ(key, value);
        };

        struct AnimatorRepeat
        {
            int                                         number;
            Animator                                    animator;

            JS_OBJ(number, animator);
        };

        struct InstancesFactory
        {                        
            std::vector<Animator>                       animators;
            std::vector<AnimatorRepeat>                 animator_repeat;

            JS_OBJ(animators, animator_repeat);
        };

        struct ScenegraphNode
        {
            std::string                 file;

            std::vector<FileArgument>   file_args;

            std::vector<std::string>    tags;

            std::vector<std::string>    rendergraph_parts;

            InstancesFactory            instances_factory;
            
            JS_OBJ(file, file_args, tags, rendergraph_parts, /*animator,*/ instances_factory);
        };

        struct Scenegraph
        {
            std::vector<ScenegraphNode> entities;
            JS_OBJ(entities);
        };

    } // json

    class IValueGenerator
    {
    public:
        virtual double generateValue() = 0;
    };

    class IncrementalValueGenerator : public IValueGenerator
    {
    public:

        IncrementalValueGenerator() = delete;

        IncrementalValueGenerator(double p_initial_value, double p_increment):
        m_value(p_initial_value),
        m_increment(p_increment)
        {
        }

        double generateValue() override
        {
            const double value = m_value;
            m_value += m_increment;
            return value;
        }

    private:

        double m_increment{ 0 };
        double m_value{ 0 };

    };

    class RandomValueGenerator : public IValueGenerator
    {
    public:

        RandomValueGenerator() = delete;

        RandomValueGenerator(int p_seed, double p_interval_min, const std::string& p_distribution_type, const std::vector<double>& p_distribution_arguments)
            : m_seed(p_seed)
            , m_interval_min(p_interval_min)
            , m_distribution_type(p_distribution_type)
            , m_distribution_arguments(p_distribution_arguments)
            , m_generator(p_seed)
        {
            if (m_distribution_type == "uniform") 
            {
                const double a = m_distribution_arguments.size() > 0 ? m_distribution_arguments[0] : 0.0;
                const double b = m_distribution_arguments.size() > 1 ? m_distribution_arguments[1] : 1.0;
                m_uniform = std::uniform_real_distribution<double>(a, b);
            }
            else if (m_distribution_type == "normal") 
            {
                const double mean = m_distribution_arguments.size() > 0 ? m_distribution_arguments[0] : 0.0;
                const double stddev = m_distribution_arguments.size() > 1 ? m_distribution_arguments[1] : 1.0;
                m_normal = std::normal_distribution<double>(mean, stddev);
            }
            else if (m_distribution_type == "exponential") 
            {
                const double lambda = m_distribution_arguments.size() > 0 ? m_distribution_arguments[0] : 1.0;
                m_exponential = std::exponential_distribution<double>(lambda);
            }
            else if (m_distribution_type == "poisson") 
            {
                const double mean = m_distribution_arguments.size() > 0 ? m_distribution_arguments[0] : 1.0;
                m_poisson = std::poisson_distribution<int>(mean);
            }
        }

        double generateValue() override
        {
            double value = 0.0;
            bool valid = false;
            constexpr int max_attempts = 10000;

            for (int attempt = 0; attempt < max_attempts; ++attempt)
            {
                if (m_distribution_type == "uniform") 
                {
                    value = m_uniform(m_generator);
                }
                else if (m_distribution_type == "normal") 
                {
                    value = m_normal(m_generator);
                }
                else if (m_distribution_type == "exponential") 
                {
                    value = m_exponential(m_generator);
                }
                else if (m_distribution_type == "poisson") 
                {
                    value = static_cast<double>(m_poisson(m_generator));
                }
                else 
                {
                    value = std::uniform_real_distribution<double>(0.0, 1.0)(m_generator);
                }

                // Vérifie la distance minimale avec toutes les valeurs déjà générées
                valid = std::all_of(
                    m_generated_values.begin(),
                    m_generated_values.end(),
                    [&](double v) { return std::abs(value - v) >= m_interval_min; }
                );

                if (valid) 
                {
                    m_generated_values.push_back(value);
                    return value;
                }
            }

            // Si on ne trouve pas de valeur valide, on retourne la dernière générée ou 0
            if (!m_generated_values.empty())
                return m_generated_values.back();
            return 0.0;
        }


    private:
        double                                      m_seed;
        double                                      m_interval_min;
        std::string                                 m_distribution_type;
        std::vector<double>                         m_distribution_arguments;

        std::default_random_engine                  m_generator;
        std::uniform_real_distribution<double>      m_uniform;
        std::normal_distribution<double>            m_normal;
        std::exponential_distribution<double>       m_exponential;
        std::poisson_distribution<int>              m_poisson;

        std::vector<double>                         m_generated_values;
    };

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
            core::maths::Real2Vector                            local_position;

            core::maths::Real2Vector                            xz_min;
            core::maths::Real2Vector                            xz_max;

            std::unordered_set<mage::core::Entity*>             entities;
        };

        // node for Octree
        struct SceneOctreeNode
        {
            double                                              side_length{ 0 };
            core::maths::Real3Vector                            local_position;

            core::maths::Real3Vector                            xyz_min;
            core::maths::Real3Vector                            xyz_max;

            std::unordered_set<mage::core::Entity*>             entities;
        };

        struct XTreeEntity
        {
            core::Entity* entity{ nullptr };
            core::QuadTreeNode<SceneQuadTreeNode>*              quadtree_node{ nullptr };
            core::OctreeNode<SceneOctreeNode>*                  octree_node{ nullptr };
            bool is_static{ false };
        };


        // regroup main infos related to a rendergraph part:
        // 
        //  > associated viewgroup
        //  > xtree stuff

        struct RendergraphPartData
        {
            json::ViewGroup                                                                         viewgroup;

            std::unique_ptr<core::QuadTreeNode<SceneQuadTreeNode>>                                  quadtree_root;
            std::unique_ptr<core::OctreeNode<SceneOctreeNode>>                                      octree_root;

            // regrouping here all entities dispatched in m_xtree_root above
            std::unordered_map<std::string, XTreeEntity>                                            xtree_entities;
        };


    public:

        enum class XtreeType
        {
            QUADTREE,
            OCTREE
        };

        struct Configuration
        {
            Configuration()
            {
                center[0] = 0.0;
                center[1] = 0.0;
                center[2] = 0.0;
            }

            double                      scene_size                  { 5000 };
            int                         xtree_max_depth             { 6 };
            int                         max_neighbourood_depth      { 2 };
            double                      object_xtreenode_ratio      { 0.1 };            
            XtreeType                   xtree_type                  { XtreeType::QUADTREE };
            core::maths::Real3Vector    center;
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
                                    const mage::core::maths::Matrix p_perspective_projection, const std::unordered_map<std::string, std::string> p_file_args, const std::unordered_map<std::string, std::unique_ptr<IValueGenerator>>& p_generators);


        void buildViewgroup(const std::string& p_jsonsource, int p_renderingQueueSystemSlot);

        void requestEntityRendering(const std::string& p_entity_id, bool p_render_it);


        void dumpXTree();
        void dumpXTreeEntities();

        void enableSystem(bool p_enabled);

    private:


        static bool is_inside_quadtreenode(const SceneQuadTreeNode& p_qtn, const core::maths::Matrix& p_global_pos);
        static bool is_inside_octreenode(const SceneOctreeNode& p_otn, const core::maths::Matrix& p_global_pos);

        void register_to_queues(const json::Channels& p_channels, mage::core::Entity* p_entity);

        void unregister_from_queues(mage::core::Entity* p_entity);

        void init_XTree(RendergraphPartData& p_rgpd);

        template<typename SceneXTreeNode, typename XTreeType>
        void update_XTree(XTreeType* p_xtree_root,
                            std::unordered_map<std::string,
                            XTreeEntity>& p_xtree_entities,
                            const std::function<void(XTreeType*, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)>& p_place_cam_on_leaf_func,
                            const std::function<void(XTreeType*, double, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)>& p_place_obj_on_leaf_func,
                            const std::function<bool(const SceneStreamerSystem::XTreeEntity&)> p_hasnode_func,
                            const std::function<bool(const SceneStreamerSystem::XTreeEntity&, const core::maths::Matrix&)> p_is_inside_func);

        template<typename SceneXTreeNode, typename XTreeType>
        void check_XTree(std::unordered_map<std::string, SceneStreamerSystem::XTreeEntity>& p_xtree_entities, 
                            const json::ViewGroup& p_viewgroup, 
                            const std::function<XTreeType* (const SceneStreamerSystem::XTreeEntity&)>& p_get_node_func);


        bool check_tag(core::Entity* p_entity, const std::string& p_tag);


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

        mage::transform::MatrixFactory process_matrixfactory_fromjson(const json::MatrixFactory& p_json_matrix_factory, 
                                                                        mage::core::ComponentContainer& p_world_aspect, 
                                                                        mage::core::ComponentContainer& p_time_aspect, 
                                                                        const std::unordered_map<std::string, std::unique_ptr<IValueGenerator>>& p_generators);

        void init_values_generator_from_matrix_factory(const std::vector<json::MatrixFactory>& p_mfs_chain, std::unordered_map<std::string, std::unique_ptr<IValueGenerator>>& p_generators);

        std::string filter_arguments_stack(const std::string p_input, const std::unordered_map<std::string, std::string> p_file_args);
    };


    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////

    template<typename SceneXTreeNode, typename XTreeType>
    void SceneStreamerSystem::update_XTree(XTreeType* p_xtree_root, 
                                                std::unordered_map<std::string, SceneStreamerSystem::XTreeEntity>& p_xtree_entities,
                                                const std::function<void(XTreeType*, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)>& p_place_cam_on_leaf_func,
                                                const std::function<void(XTreeType*, double, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)>& p_place_obj_on_leaf_func,
                                                const std::function<bool(const SceneStreamerSystem::XTreeEntity&)> p_hasnode_func,
                                                const std::function<bool(const SceneStreamerSystem::XTreeEntity&, const core::maths::Matrix&)> p_is_inside_func)
    {
        for (auto& xe : p_xtree_entities)
        {
            core::Entity* entity{ xe.second.entity };
            const auto& world_aspect{ entity->aspectAccess(worldAspect::id) };

            const auto& entity_worldposition_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };
            auto& entity_worldposition{ entity_worldposition_list.at(0)->getPurpose() };
            const auto global_pos = entity_worldposition.global_pos;

            // check tags
            if (check_tag(entity, "#alwaysRendered"))
            {
                if (!m_entity_renderings.at(entity->getId()).m_rendered)
                {
                    m_entity_renderings.at(entity->getId()).m_request_rendering = true;
                }
                continue;
            }

            ///////////////////////////////////////////////

            if (!p_hasnode_func(xe.second))
            {
                //// PLACE NEW

                if (entity->hasAspect(cameraAspect::id))
                {
                    // camera

                    p_place_cam_on_leaf_func(p_xtree_root, global_pos, entity, xe.second);
                }
                else if (entity->hasAspect(resourcesAspect::id))
                {
                    const auto& resources_aspect{ entity->aspectAccess(resourcesAspect::id) };

                    const auto meshes_list{ resources_aspect.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>() };
                    if (meshes_list.size() > 0)
                    {
                        auto& meshe_descr{ meshes_list.at(0)->getPurpose() };
                        TriangleMeshe& meshe{ meshe_descr.second };

                        if (TriangleMeshe::State::RENDERERLOADED == meshe.getState())
                        {
                            const double meshe_size{ meshe.getSize() };
                            p_place_obj_on_leaf_func(p_xtree_root, meshe_size, global_pos, entity, xe.second);
                        }
                    }
                }
            }
            else
            {
                //// UPDATE

                if (entity->hasAspect(cameraAspect::id))
                {
                    const bool is_inside{ p_is_inside_func(xe.second, global_pos) };
                    if (!is_inside)
                    {
                        // update location in xtree
                        p_place_cam_on_leaf_func(p_xtree_root, global_pos, entity, xe.second);
                    }
                }
                else if (entity->hasAspect(resourcesAspect::id) && !xe.second.is_static)
                {
                    const bool is_inside{ p_is_inside_func(xe.second, global_pos) };

                    if (!is_inside)
                    {
                        // update location in xtree
                        const auto& resources_aspect{ entity->aspectAccess(resourcesAspect::id) };

                        const auto meshes_list{ resources_aspect.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>() };
                        if (meshes_list.size() > 0)
                        {
                            auto& meshe_descr{ meshes_list.at(0)->getPurpose() };
                            TriangleMeshe& meshe{ meshe_descr.second };

                            if (TriangleMeshe::State::BLOBLOADED == meshe.getState())
                            {
                                const double meshe_size{ meshe.getSize() };
                                p_place_obj_on_leaf_func(p_xtree_root, meshe_size, global_pos, entity, xe.second);
                            }
                        }
                    }
                }
            }
        }
    }


    template<typename SceneXTreeNode, typename XTreeType>
    void SceneStreamerSystem::check_XTree(std::unordered_map<std::string, SceneStreamerSystem::XTreeEntity>& p_xtree_entities, 
                                            const json::ViewGroup& p_viewgroup, 
                                            const std::function<XTreeType* (const SceneStreamerSystem::XTreeEntity&)>& p_get_node_func)
    {
        // for current view group, find current camera id 

        auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(m_renderingQueueSystemSlot)) };
        const auto current_views{ renderingQueueSystemInstance->getViewGroupCurrentViews(p_viewgroup.name) };

        const std::string main_camera_id{ current_views.first };

        XTreeEntity xe{ p_xtree_entities.at(main_camera_id) };

        std::unordered_set<mage::core::Entity*> found_entities; // search entities in camera's neighbourood

        const std::function<void(std::unordered_set<mage::core::Entity*>&, XTreeType*, int)> search_near_entities
        {
            [&](std::unordered_set<mage::core::Entity*>& p_found_entities, XTreeType* p_node, int p_neighbourood_depth)
            {
                if (p_neighbourood_depth > m_configuration.max_neighbourood_depth)
                {
                    return;
                }

                ////////// search in current node
                const SceneXTreeNode& scene_xtree_node{ p_node->getData() };
                for (mage::core::Entity* e : scene_xtree_node.entities)
                {
                    if (m_entity_renderings.count(e->getId()) > 0)
                    {
                        // store only those than can be rendered
                        p_found_entities.insert(e);
                    }
                }

                ////////// search in current node neighbours

                std::vector<XTreeType*> neighbours{ p_node->getNeighbours() };
                for (XTreeType* n : neighbours)
                {
                    if (nullptr != n)
                    {
                        const SceneXTreeNode& n_scene_xtree_node{ n->getData() };
                        for (mage::core::Entity* e : n_scene_xtree_node.entities)
                        {
                            if (m_entity_renderings.count(e->getId()) > 0)
                            {
                                // store only those than can be rendered
                                p_found_entities.insert(e);
                            }
                        }
                        search_near_entities(p_found_entities, n, p_neighbourood_depth + 1);
                    }
                }
            }
        };


        XTreeType* curr = p_get_node_func(xe); // get xe.quadtree or xe.octree regarding XTreeType used :)

        if (curr)
        {
            while (1)
            {
                search_near_entities(found_entities, curr, 0);

                curr = curr->getParent();
                if (nullptr == curr)
                {
                    break;
                }
            }

            // new entities discovered, to render
            for (mage::core::Entity* entity : found_entities)
            {
                if (!m_found_entities_to_render.count(entity))
                {
                    // just discovered -> ask for rendering
                    if (!m_entity_renderings.at(entity->getId()).m_rendered)
                    {
                        m_entity_renderings.at(entity->getId()).m_request_rendering = true;
                    }
                }
            }

            // entities not in neigbourood no more, to remove from rendering...
            for (mage::core::Entity* rendered_entity : m_found_entities_to_render)
            {
                if (!found_entities.count(rendered_entity))
                {
                    // not found no more -> ask to stop rendering

                    if (m_entity_renderings.at(rendered_entity->getId()).m_rendered)
                    {
                        m_entity_renderings.at(rendered_entity->getId()).m_request_rendering = false;
                    }
                }
            }

            // update...
            m_found_entities_to_render = found_entities;
        }
    };
}
