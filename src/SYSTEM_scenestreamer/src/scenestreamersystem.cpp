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
#include <unordered_set>

#include <json_struct/json_struct.h>

#include "scenestreamersystem.h"
#include "renderingqueuesystem.h"

#include "sysengine.h"
#include "entity.h"
#include "entitygraph.h"
#include "aspects.h"
#include "ecshelpers.h"
#include "exceptions.h"
#include "texture.h"
#include "entitygraph_helpers.h"
#include "renderingpasses_helpers.h"
#include "animators_helpers.h"
#include "trianglemeshe.h"
#include "matrixchain.h"

#include "filesystem.h"

using namespace mage;
using namespace mage::core;
using namespace mage::core::maths;

SceneStreamerSystem::SceneStreamerSystem(Entitygraph& p_entitygraph) : System(p_entitygraph),
m_localLogger("SceneStreamerSystem", mage::core::logger::Configuration::getInstance())
{
}

void SceneStreamerSystem::enableSystem(bool p_enabled)
{
    m_enabled = p_enabled;
}

void SceneStreamerSystem::configure(const Configuration& p_config)
{
    if (!m_configured)
    {
        m_configuration = p_config;
        m_configured = true;
    }
}

void SceneStreamerSystem::init_XTree(RendergraphPartData& p_rgpd)
{
    if (!m_configured)
    {
        _EXCEPTION("Not configured");
    }

    const SceneQuadTreeNode root_node{ m_configuration.scene_size, Real2Vector(0, 0),
                                        Real2Vector(-m_configuration.scene_size / 2, -m_configuration.scene_size / 2),
                                        Real2Vector(m_configuration.scene_size / 2, m_configuration.scene_size / 2) };

    
    p_rgpd.quadtree_root = std::make_unique<core::QuadTreeNode<SceneQuadTreeNode>>(root_node);

    const std::function<void(QuadTreeNode<SceneQuadTreeNode>*, int)> expand
    {
        [&](QuadTreeNode<SceneQuadTreeNode>* p_current_node, int p_max_depth)
        {
            if (p_max_depth == p_current_node->getDepth())
            {
                return;
            }

            p_current_node->split();

            const auto curr_node_pos{ p_current_node->getData().position };

            for (int i = 0; i < mage::core::QuadTreeNode<SceneQuadTreeNode>::ChildCount; i++)
            {
                auto child { p_current_node->getChild(i) };

                SceneQuadTreeNode childNodeContent;
                childNodeContent.side_length = p_current_node->getData().side_length / 2;

                switch (i)
                {
                    case QuadTreeNode<SceneQuadTreeNode>::L0_UP_LEFT_INDEX:

                        childNodeContent.position[0] = curr_node_pos[0] - childNodeContent.side_length / 2;
                        childNodeContent.position[1] = curr_node_pos[1] - childNodeContent.side_length / 2;
                        break;

                    case QuadTreeNode<SceneQuadTreeNode>::L0_UP_RIGHT_INDEX:

                        childNodeContent.position[0] = curr_node_pos[0] + childNodeContent.side_length / 2;
                        childNodeContent.position[1] = curr_node_pos[1] - childNodeContent.side_length / 2;
                        break;

                    case QuadTreeNode<SceneQuadTreeNode>::L0_DOWN_RIGHT_INDEX:

                        childNodeContent.position[0] = curr_node_pos[0] + childNodeContent.side_length / 2;
                        childNodeContent.position[1] = curr_node_pos[1] + childNodeContent.side_length / 2;
                        break;

                    case QuadTreeNode<SceneQuadTreeNode>::L0_DOWN_LEFT_INDEX:

                        childNodeContent.position[0] = curr_node_pos[0] - childNodeContent.side_length / 2;
                        childNodeContent.position[1] = curr_node_pos[1] + childNodeContent.side_length / 2;
                        break;
                }

                childNodeContent.xz_min[0] = childNodeContent.position[0] - childNodeContent.side_length / 2;
                childNodeContent.xz_min[1] = childNodeContent.position[1] - childNodeContent.side_length / 2;

                childNodeContent.xz_max[0] = childNodeContent.position[0] + childNodeContent.side_length / 2;
                childNodeContent.xz_max[1] = childNodeContent.position[1] + childNodeContent.side_length / 2;


                child->setData(childNodeContent);

                expand(child, p_max_depth);
            }
        }
    };

    expand(p_rgpd.quadtree_root.get(), m_configuration.xtree_max_depth);
}

void SceneStreamerSystem::run()
{
    if (!m_enabled)
    {
        return;
    }

    /////////////////////////////////////////////////////////
    // detect new entities to insert in the XTree
    const auto forEachWorldAspect
    {
        [&](Entity* p_entity, const ComponentContainer& p_world_components)
        {
            ///// compute matrix hierarchy

            const auto& entity_worldposition_list { p_world_components.getComponentsByType<transform::WorldPosition>() };
            if (0 == entity_worldposition_list.size())
            {
                return;
            }

            const auto& tagsAspect{ p_entity->aspectAccess(mage::core::tagsAspect::id)};

            const auto& entity_domains_list{ tagsAspect.getComponentsByType<mage::core::tagsAspect::GraphDomain>() };
            const auto& str_tags_list{ tagsAspect.getComponentsByType<std::unordered_set<std::string>>() };

            if (0 == entity_domains_list.size())
            {
                return;
            }

            if (entity_domains_list.at(0)->getPurpose() != mage::core::tagsAspect::GraphDomain::SCENEGRAPH)
            {
                return;
            }

            if (m_scene_entities_rg_parts.count(p_entity->getId()))
            {
                const std::unordered_set<std::string> scene_entity_rg_parts{ m_scene_entities_rg_parts.at(p_entity->getId()) };

                for (auto& rgpd : m_rendergraphpart_data)
                {
                    for (const std::string& rendering_queue_id : rgpd.second.viewgroup.queue_entities)
                    {
                        if (scene_entity_rg_parts.count(rendering_queue_id))
                        {
                            // can add this entity in this viewgroup/rgpd xtree
                            if (!rgpd.second.xtree_entities.count(p_entity->getId()))
                            {
                                XTreeEntity xtreeEnt;
                                xtreeEnt.entity = p_entity;

                                 if (str_tags_list.size() > 0)
                                 {
                                    const auto str_tags{ str_tags_list.at(0)->getPurpose() };
                                    if (str_tags.count("#static"))
                                    {
                                        xtreeEnt.is_static = true;
                                    }
                                 }
                                 rgpd.second.xtree_entities[p_entity->getId()] = xtreeEnt;
                            }
                        }
                    }
                }
            }

        }
    };
    mage::helpers::extractAspectsTopDown<mage::core::worldAspect>(m_entitygraph, forEachWorldAspect);

    /////////////////////////////////////////////////////////
    // XTrees updating
    // 
    for (auto& rgpd : m_rendergraphpart_data)
    {
        auto& rgpd_data = rgpd.second;

        if (XtreeType::QUADTREE == m_configuration.xtree_type)
        {
            /////////////////////////////////////////////////////////////////////////////////
            // place cam in appropriate xtree leaf : utility lambda
            const std::function<void(core::QuadTreeNode<SceneQuadTreeNode>*, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)> place_cam_on_leaf
            {
                [&](core::QuadTreeNode<SceneQuadTreeNode>* p_current_node, const core::maths::Matrix& p_global_pos, core::Entity* p_entity, SceneStreamerSystem::XTreeEntity& p_xtreeEntity)
                {
                    if (p_current_node->isLeaf())
                    {
                        p_current_node->dataAccess().entities.insert(p_entity);
                        p_xtreeEntity.quadtree_node = p_current_node;
                    }
                    else
                    {
                        for (int i = 0; i < core::QuadTreeNode<SceneQuadTreeNode>::ChildCount; i++)
                        {
                            auto child { p_current_node->getChild(i) };

                            if (SceneStreamerSystem::is_inside_quadtreenode(child->getData(), p_global_pos))
                            {
                                place_cam_on_leaf(child, p_global_pos, p_entity, p_xtreeEntity);
                            }
                        }
                    }
                }
            };
            /////////////////////////////////////////////////////////////////////////////////

            /////////////////////////////////////////////////////////////////////////////////
            // place 3D object in appropriate xtree leaf : utility lambda

            const std::function<void(core::QuadTreeNode<SceneQuadTreeNode>*, double, const core::maths::Matrix&, core::Entity*, SceneStreamerSystem::XTreeEntity&)> place_obj_on_leaf
            {
                [&](core::QuadTreeNode<SceneQuadTreeNode>* p_current_node, double p_obj_size, const core::maths::Matrix& p_global_pos, core::Entity* p_entity, SceneStreamerSystem::XTreeEntity& p_xtreeEntity)
                {
                    if (p_current_node->isLeaf())
                    {
                        // leaf reached, cannt go beyond, so place it anyway
                        p_current_node->dataAccess().entities.insert(p_entity);
                        p_xtreeEntity.quadtree_node = p_current_node;
                    }
                    else
                    {
                        const double node_size { p_current_node->getData().side_length };

                        if (p_obj_size / node_size > m_configuration.object_xtreenode_ratio)
                        {
                            //place it
                            p_current_node->dataAccess().entities.insert(p_entity);
                            p_xtreeEntity.quadtree_node = p_current_node;
                        }
                        else
                        {
                            for (int i = 0; i < core::QuadTreeNode<SceneQuadTreeNode>::ChildCount; i++)
                            {
                                auto child{ p_current_node->getChild(i) };

                                if (SceneStreamerSystem::is_inside_quadtreenode(child->getData(), p_global_pos))
                                {
                                    place_obj_on_leaf(child, p_obj_size, p_global_pos, p_entity, p_xtreeEntity);
                                }
                            }
                        }
                    }
                }
            };
            /////////////////////////////////////////////////////////////////////////////////

            update_QuadTree<core::QuadTreeNode<SceneQuadTreeNode>>(rgpd_data.quadtree_root.get(), rgpd_data.xtree_entities, place_cam_on_leaf, place_obj_on_leaf);
        }
        else // XtreeType::OCTREE
        {
            // To be continued...
        }
    }

    /////////////////////////////////////////////////////////
    // XTree check
    //
    for (auto& rgpd : m_rendergraphpart_data)
    {
        auto& rgpd_data = rgpd.second;

        if (XtreeType::QUADTREE == m_configuration.xtree_type)
        {
            const std::function<core::QuadTreeNode<SceneQuadTreeNode>* (const XTreeEntity&)> get_quadtree_node_func
            {
                [](const XTreeEntity& p_xe) -> core::QuadTreeNode<SceneQuadTreeNode>*
                {
                    return p_xe.quadtree_node;
                }
            };

            check_XTree<SceneQuadTreeNode, core::QuadTreeNode<SceneQuadTreeNode>>(rgpd_data.xtree_entities, rgpd_data.viewgroup, get_quadtree_node_func);
        }
        else // XtreeType::OCTREE
        {
            const std::function<core::OctreeNode<SceneOctreeNode>* (const XTreeEntity&)> get_octree_node_func
            {
                [](const XTreeEntity& p_xe) -> core::OctreeNode<SceneOctreeNode>*
                {
                    return p_xe.octree_node;
                }
            };

            check_XTree<SceneOctreeNode, core::OctreeNode<SceneOctreeNode>>(rgpd_data.xtree_entities, rgpd_data.viewgroup, get_octree_node_func);
        }
    }

    /////////////////////////////////////////////////////////
    // loop on entity rendering entries
    /////////////////////////////////////////////////////////
    for (auto& e : m_entity_renderings)
    {
        if (e.second.m_request_rendering && !e.second.m_rendered)
        {            
            register_to_queues(e.second.m_channels, m_scene_entities.at(e.first));
            e.second.m_rendered = true;

            for (const auto& call : m_callbacks)
            {
                call(SceneStreamerSystemEvent::RENDERING_ENABLED, e.first);
            }
            _MAGE_DEBUG(m_localLogger, "SceneStreamerSystemEvent::RENDERING_ENABLED for " + e.first);
        }
        else if (!e.second.m_request_rendering && e.second.m_rendered)
        {
            unregister_from_queues(m_scene_entities.at(e.first));
            e.second.m_rendered = false;

            for (const auto& call : m_callbacks)
            {
                call(SceneStreamerSystemEvent::RENDERING_DISABLED, e.first);
            }
            _MAGE_DEBUG(m_localLogger, "SceneStreamerSystemEvent::RENDERING_DISABLED for " + e.first);
        }
    }
}

void SceneStreamerSystem::buildRendergraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId,
                                                int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height)
{
    json::RendergraphNodesCollection rtc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(rtc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse rendergraph: " + errorStr);
    }

    const std::function<void(const json::RendergraphNode&, const std::string&, int)> browseHierarchy
    {
        [&](const json::RendergraphNode& p_node, const std::string& p_parent_id, int depth)
        {
            if ("" != p_node.target.descr)
            {
                // add rendering target

                const std::unordered_map<std::string, mage::Texture::Format> texture_format_translation
                {
                    { "TEXTURE_RGB", mage::Texture::Format::TEXTURE_RGB },
                    { "TEXTURE_FLOAT", mage::Texture::Format::TEXTURE_FLOAT },
                    { "TEXTURE_FLOAT32", mage::Texture::Format::TEXTURE_FLOAT32 },
                    { "TEXTURE_FLOATVECTOR", mage::Texture::Format::TEXTURE_FLOATVECTOR },
                    { "TEXTURE_FLOATVECTOR32", mage::Texture::Format::TEXTURE_FLOATVECTOR32 },
                };

                std::vector<std::pair<size_t, Texture>> inputs;

                for (const auto& input : p_node.target.inputs)
                {
                    int final_w_width = (json::fillWithWindowDims == input.buffer_texture.width ? p_w_width : input.buffer_texture.width);
                    int final_h_width = (json::fillWithWindowDims == input.buffer_texture.height ? p_w_height : input.buffer_texture.height);

                    const auto input_channnel{ Texture(texture_format_translation.at(input.buffer_texture.format_descr), final_w_width, final_h_width) };

                    inputs.push_back(std::make_pair(input.stage, input_channnel));
                }

                int final_v_width = (json::fillWithViewportDims == p_node.target.width ? p_w_width : p_characteristics_v_width);
                int final_v_height = (json::fillWithViewportDims == p_node.target.height ? p_w_height : p_characteristics_v_height);

                const std::string queue_name{ p_node.target.descr + "_queue" };

                const std::string entity_queue_name{ p_node.target.descr + "Target_queue_Entity" };
                const std::string entity_target_name{ p_node.target.descr + "Target_quad_Entity" };
                const std::string entity_view_name{ p_node.target.descr + "Target_view_Entity" };

                mage::helpers::plugRenderingTarget(m_entitygraph,
                    queue_name,
                    final_v_width, final_v_height,
                    p_parent_id,
                    entity_queue_name,
                    entity_target_name,
                    entity_view_name,
                    p_node.target.shaders.at(0).name,
                    p_node.target.shaders.at(1).name,
                    inputs,
                    p_node.target.destination_stage);

                // plug shaders args

                Entity* quad_ent{ m_entitygraph.node(entity_target_name).data() };
                auto& rendering_aspect{ quad_ent->aspectAccess(core::renderingAspect::id) };

                rendering::DrawingControl& dc{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

                const auto& vshader{ p_node.target.shaders.at(0) };
                for (const auto& arg : vshader.args)
                {
                    dc.vshaders_map.push_back(std::make_pair(arg.source, arg.destination));
                }

                const auto& pshader{ p_node.target.shaders.at(1) };
                for (const auto& arg : pshader.args)
                {
                    dc.pshaders_map.push_back(std::make_pair(arg.source, arg.destination));
                }

                // recursive call
                for (auto& e : p_node.target.subs)
                {
                    browseHierarchy(e, entity_target_name, depth + 1);
                }
            }

            if ("" != p_node.queue.id)
            {
                // add rendering queue

                rendering::Queue renderingQueue(p_node.queue.id);

                renderingQueue.setTargetClearColor({ p_node.queue.target_clear_color.r, 
                                                    p_node.queue.target_clear_color.g, 
                                                    p_node.queue.target_clear_color.b, 
                                                    p_node.queue.target_clear_color.a });

                renderingQueue.enableTargetClearing(p_node.queue.target_clear);
                renderingQueue.enableTargetDepthClearing(p_node.queue.target_depth_clear);
                renderingQueue.setTargetStage(p_node.queue.target_stage);

                mage::helpers::plugRenderingQueue(m_entitygraph, renderingQueue, p_parent_id, p_node.queue.id);


                // register passe default configs
                const auto renderingHelper{ mage::helpers::RenderingChannels::getInstance() };
                renderingHelper->createDefaultChannelConfig(p_node.queue.id, p_node.queue.rendering_channel_type);
            }
        }
    };

    for (const auto& e : rtc.subs)
    {
        browseHierarchy(e, p_parentEntityId, 0);
    }
}


void SceneStreamerSystem::buildScenegraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId, const mage::core::maths::Matrix p_perspective_projection)
{
    json::Scenegraph sg;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(sg) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse scenegraph: " + errorStr);
    }

    for (const auto& e : sg.entities)
    {
        mage::core::FileContent<char> entityFileContent("./module_streamed_anims_config/" + e.file + ".json");
        entityFileContent.load();

        std::unordered_map<std::string, std::string> file_args;
        for (const json::FileArgument& file_arg : e.file_args)
        {
            file_args.emplace(file_arg.key, file_arg.value);
        }

        buildScenegraphEntity(entityFileContent.getData(), e.rendergraph_parts, e.animator, e.tags, p_parentEntityId, p_perspective_projection, file_args);
    }
}

void SceneStreamerSystem::buildScenegraphEntity(const std::string& p_jsonsource, const std::vector<std::string>& p_rendergraph_parts,
                                                                                    const json::Animator& p_animator, 
                                                                                    const std::vector<std::string>& p_tags, 
                                                                                    const std::string& p_parentEntityId, 
                                                                                    const mage::core::maths::Matrix p_perspective_projection,
                                                                                    const std::unordered_map<std::string, std::string> p_file_args)
{
    json::ScenegraphEntitiesCollection sgc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(sgc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse scenegraph entity: " + errorStr);
    }

    const std::function<void(const json::ScenegraphEntity&, const std::string&, const json::Animator&, int)> browseHierarchy
    {
        [&](const json::ScenegraphEntity& p_node, const std::string& p_parent_id, const json::Animator& p_animator, int depth)
        {
            const std::string entity_id { filter_arguments_stack(p_node.id, p_file_args) };
            
            for (size_t i = 0; i < p_rendergraph_parts.size(); i++)
            {
                m_scene_entities_rg_parts[entity_id].insert(p_rendergraph_parts[i]);
            }

            if ("" != p_node.helper)
            {
                if ("plugCamera" == p_node.helper)
                {
                    core::Entity* camera_entity{ helpers::plugCamera(m_entitygraph, p_perspective_projection, p_parent_id, entity_id) };
                    register_scene_entity(camera_entity);
                }
            }
            else
            {
                auto& entityNode{ m_entitygraph.add(m_entitygraph.node(p_parent_id), entity_id) };
                const auto entity{ entityNode.data() };

                // create aspects
                auto& time_aspect{ entity->makeAspect(core::timeAspect::id) };
                auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };
                auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
                auto& tags_aspect{ entity->makeAspect(core::tagsAspect::id) };

                // tags
                ///////////////////////////////////////////////
                
                tags_aspect.addComponent<core::tagsAspect::GraphDomain>("domain", core::tagsAspect::GraphDomain::SCENEGRAPH);

                std::unordered_set<std::string> str_tags;
                for (const auto& tag : p_tags)
                {
                    str_tags.insert(tag);                    
                }
                tags_aspect.addComponent<std::unordered_set<std::string>>("string_tags", str_tags);

                // World Aspect

                if ("gimbalLockJoin" == p_animator.helper)
                {
                    world_aspect.addComponent<transform::WorldPosition>("position");

                    world_aspect.addComponent<double>("gbl_theta", 0);
                    world_aspect.addComponent<double>("gbl_phi", 0);
                    world_aspect.addComponent<double>("gbl_speed", 0);

                    core::maths::Matrix positionmat;
                    world_aspect.addComponent<core::maths::Real3Vector>("gbl_pos", maths::Real3Vector(0.0, 0.0, 0.0));

                    world_aspect.addComponent<transform::Animator>("animator", transform::Animator(
                        {
                            // input-output/components keys id mapping
                            {"gimbalLockJointAnim.theta", "gbl_theta"},
                            {"gimbalLockJointAnim.phi", "gbl_phi"},
                            {"gimbalLockJointAnim.pos", "gbl_pos"},
                            {"gimbalLockJointAnim.speed", "gbl_speed"},
                            {"gimbalLockJointAnim.output", "position"}

                        }, helpers::makeGimbalLockJointAnimator())
                    );
                }
                // if no helper, decode matrix_factory
                else if ("" == p_animator.helper)
                {
                    world_aspect.addComponent<transform::WorldPosition>("position");

                    std::vector<mage::transform::MatrixFactory> mf_stack;

                    for (const auto& json_mf : p_animator.matrix_factory_chain)
                    {
                        const auto mf{ process_matrixfactory_fromjson(json_mf, world_aspect, time_aspect) };
                        mf_stack.push_back(mf);
                    }

                    if (0 == mf_stack.size())
                    {
                        _EXCEPTION("need some matrix factory in animator");
                    }

                    world_aspect.addComponent<std::vector<mage::transform::MatrixFactory>>("mf_stack", mf_stack);

                    world_aspect.addComponent<transform::Animator>(p_animator.descr, transform::Animator
                    (
                        {},
                        [=](const core::ComponentContainer& p_world_aspect,
                            const core::ComponentContainer& p_time_aspect,
                            const transform::WorldPosition&,
                            const std::unordered_map<std::string, std::string>&)
                        {
                            auto& mf_stack{ p_world_aspect.getComponent<std::vector<mage::transform::MatrixFactory>>("mf_stack")->getPurpose()};
                            transform::MatrixChain mc;

                            for (auto& mf : mf_stack)
                            {
                                const auto result_mat{ mf.getResult() };
                                mc.pushMatrix(result_mat);
                            }

                            mc.buildResult();

                            transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
                            wp.local_pos = wp.local_pos * mc.getResultTransform();
                        }
                    ));
                }

                // Resource Aspect

                // meshe resource ?
                if ("" != p_node.resource_aspect.meshe.descr)
                {
                    resource_aspect.addComponent< std::pair<std::pair<std::string, std::string>, TriangleMeshe>>("meshe", std::make_pair(std::make_pair(p_node.resource_aspect.meshe.meshe_id, p_node.resource_aspect.meshe.filename), TriangleMeshe()));
                }

                register_scene_entity(entity);

    
                if (p_node.channels.configs.size() > 0) // store only entites that can be "rendered" -> those with number of channels > 0
                {
                    if (m_entity_renderings.count(entity_id) > 0)
                    {
                        _EXCEPTION("Already registered " + entity_id);
                    }
                    else
                    {
                        EntityRendering rendering_infos(p_node.channels);
                        m_entity_renderings[entity_id] = rendering_infos;
                    }
                }
            }

            // recursive call
            for (auto& e : p_node.subs)
            {
                browseHierarchy(e, entity_id, e.world_aspect.animator, depth + 1);
            }
        }
    };

    for (const auto& e : sgc.subs)
    {
        browseHierarchy(e, p_parentEntityId, p_animator, 0);
    }

}

void SceneStreamerSystem::register_scene_entity(mage::core::Entity* p_entity)
{
    const auto entity_id{ p_entity->getId() };

    if (m_scene_entities.count(entity_id) > 0)
    {
        _EXCEPTION("Already registered " + entity_id);
    }
    else
    {
        m_scene_entities[entity_id] = p_entity;
    }
}

void SceneStreamerSystem::buildViewgroup(const std::string& p_jsonsource, int p_renderingQueueSystemSlot)
{
    json::ViewGroup vg;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(vg) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse scenegraph: " + errorStr);
    }

    if (m_rendergraphpart_data.count(vg.name) > 0)
    {
        _EXCEPTION("Already registered " + vg.name);
    }

    auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(p_renderingQueueSystemSlot))};

    renderingQueueSystemInstance->createViewGroup(vg.name);

    std::unordered_set<std::string> queues_id_list;
    for (const auto& e : vg.queue_entities)
    {
        queues_id_list.insert(e);
    }

    renderingQueueSystemInstance->addQueuesToViewGroup(vg.name, queues_id_list);

    m_rendergraphpart_data[vg.name].viewgroup = vg;

    init_XTree(m_rendergraphpart_data.at(vg.name));

    m_renderingQueueSystemSlot = p_renderingQueueSystemSlot;
}

void SceneStreamerSystem::setViewgroupMainview(const std::string& p_vg_id, const std::string& p_mainview)
{
    if (0 == m_rendergraphpart_data.count(p_vg_id))
    {
        _EXCEPTION("Unknown view group id " + p_vg_id);
    }

    auto& rgdata{ m_rendergraphpart_data.at(p_vg_id) };

    auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(m_renderingQueueSystemSlot)) };
    renderingQueueSystemInstance->setViewGroupMainView(rgdata.viewgroup.name, p_mainview);
}

core::SyncVariable SceneStreamerSystem::build_syncvariable_fromjson(const json::SyncVariable& p_syncvar)
{
    std::unordered_map<std::string, core::SyncVariable::State> aig_state
    {
        {"OFF", core::SyncVariable::State::OFF },
        {"ON", core::SyncVariable::State::ON }
    };

    std::unordered_map<std::string, core::SyncVariable::Type> aig_type
    {
        {"ANGLE", core::SyncVariable::Type::ANGLE },
        {"POSITION", core::SyncVariable::Type::POSITION }
    };

    std::unordered_map<std::string, core::SyncVariable::Direction> aig_direction
    {
        {"INC", core::SyncVariable::Direction::INC },
        {"DEC", core::SyncVariable::Direction::DEC },
        {"ZERO", core::SyncVariable::Direction::ZERO }
    };

    std::unordered_map<std::string, core::SyncVariable::BoundariesManagement> aig_management
    {
        {"MIRROR", core::SyncVariable::BoundariesManagement::MIRROR },
        {"STOP", core::SyncVariable::BoundariesManagement::STOP },
        {"WRAP", core::SyncVariable::BoundariesManagement::WRAP },
    };

    core::SyncVariable sync_variable(aig_type.at(p_syncvar.type), p_syncvar.step, aig_direction.at(p_syncvar.direction),
                                        p_syncvar.initial_value, core::SyncVariable::Boundaries{ p_syncvar.min, p_syncvar.max },
                                        aig_management.at(p_syncvar.management));

    sync_variable.state = aig_state.at(p_syncvar.initial_state);

    return sync_variable;
}

mage::transform::MatrixFactory SceneStreamerSystem::process_matrixfactory_fromjson(const json::MatrixFactory& p_json_matrix_factory, mage::core::ComponentContainer& p_world_aspect, mage::core::ComponentContainer& p_time_aspect)
{
    mage::transform::MatrixFactory matrix_factory(p_json_matrix_factory.type);

    if ("" != p_json_matrix_factory.xyzw_datacloud_value.descr)
    {
        p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real4Vector>>(p_json_matrix_factory.xyzw_datacloud_value.descr,
            p_json_matrix_factory.xyzw_datacloud_value.var_name);

        matrix_factory.setXYZWSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real4Vector>>(p_json_matrix_factory.xyzw_datacloud_value.descr)->getPurpose());
    }
    else if ("" != p_json_matrix_factory.xyzw_direct_value.descr)
    {
        p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<core::maths::Real4Vector>>(p_json_matrix_factory.xyzw_direct_value.descr,
            core::maths::Real4Vector(p_json_matrix_factory.xyzw_direct_value.x,
                p_json_matrix_factory.xyzw_direct_value.y,
                p_json_matrix_factory.xyzw_direct_value.z,
                p_json_matrix_factory.xyzw_direct_value.w));

        matrix_factory.setXYZWSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<core::maths::Real4Vector>>(p_json_matrix_factory.xyzw_direct_value.descr)->getPurpose());
    }
    else
    {
        if ("" != p_json_matrix_factory.xyz_datacloud_value.descr)
        {
            p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real3Vector>>(p_json_matrix_factory.xyz_datacloud_value.descr,
                p_json_matrix_factory.xyz_datacloud_value.var_name);

            matrix_factory.setXYZSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real3Vector>>(p_json_matrix_factory.xyz_datacloud_value.descr)->getPurpose());

            // W
            if ("" != p_json_matrix_factory.w_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr, p_json_matrix_factory.w_datacloud_value.var_name);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.w_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.w_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr, p_json_matrix_factory.w_direct_value.value);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr)->getPurpose());
            }
        }
        else if ("" != p_json_matrix_factory.xyz_direct_value.descr)
        {
            p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<core::maths::Real3Vector>>(p_json_matrix_factory.xyz_direct_value.descr,
                core::maths::Real3Vector(p_json_matrix_factory.xyzw_direct_value.x,
                    p_json_matrix_factory.xyzw_direct_value.y,
                    p_json_matrix_factory.xyzw_direct_value.z));

            matrix_factory.setXYZSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<core::maths::Real3Vector>>(p_json_matrix_factory.xyz_datacloud_value.descr)->getPurpose());

            // W
            if ("" != p_json_matrix_factory.w_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr, p_json_matrix_factory.w_datacloud_value.var_name);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.w_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.w_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr, p_json_matrix_factory.w_direct_value.value);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr)->getPurpose());
            }
        }
        else
        {
            // X
            if ("" != p_json_matrix_factory.x_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.x_datacloud_value.descr, p_json_matrix_factory.x_datacloud_value.var_name);
                matrix_factory.setXSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.x_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.x_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.x_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.x_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.x_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setXSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.x_syncvar_value.descr)->getPurpose());

            }
            else if ("" != p_json_matrix_factory.x_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.x_direct_value.descr, p_json_matrix_factory.x_direct_value.value);
                matrix_factory.setXSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.x_direct_value.descr)->getPurpose());
            }

            // Y
            if ("" != p_json_matrix_factory.y_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.y_datacloud_value.descr, p_json_matrix_factory.y_datacloud_value.var_name);
                matrix_factory.setYSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.y_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.y_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.y_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.y_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.y_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setYSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.y_syncvar_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.y_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.y_direct_value.descr, p_json_matrix_factory.y_direct_value.value);
                matrix_factory.setYSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.y_direct_value.descr)->getPurpose());
            }

            // Z
            if ("" != p_json_matrix_factory.z_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.z_datacloud_value.descr, p_json_matrix_factory.z_datacloud_value.var_name);
                matrix_factory.setZSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.z_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.z_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.z_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.z_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.z_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setZSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.z_syncvar_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.z_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.z_direct_value.descr, p_json_matrix_factory.z_direct_value.value);
                matrix_factory.setZSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.z_direct_value.descr)->getPurpose());
            }

            // W
            if ("" != p_json_matrix_factory.w_datacloud_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr, p_json_matrix_factory.w_datacloud_value.var_name);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(p_json_matrix_factory.w_datacloud_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_syncvar_value.descr)
            {
                const auto sync_var{ build_syncvariable_fromjson(p_json_matrix_factory.w_syncvar_value.sync_var) };

                const std::string sync_var_name{ p_json_matrix_factory.w_syncvar_value.descr + " sync_var" };
                p_time_aspect.addComponent<SyncVariable>(sync_var_name, sync_var);

                p_world_aspect.addComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr, &p_time_aspect.getComponent<SyncVariable>(sync_var_name)->getPurpose());
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::SyncVarValueMatrixSource>(p_json_matrix_factory.w_syncvar_value.descr)->getPurpose());
            }
            else if ("" != p_json_matrix_factory.w_direct_value.descr)
            {
                p_world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr, p_json_matrix_factory.w_direct_value.value);
                matrix_factory.setWSource(&p_world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(p_json_matrix_factory.w_direct_value.descr)->getPurpose());
            }
        }
    }
    return matrix_factory;
}

void SceneStreamerSystem::requestEntityRendering(const std::string& p_entity_id, bool p_render_it)
{
    if (m_entity_renderings.count(p_entity_id))
    {
        m_entity_renderings.at(p_entity_id).m_request_rendering = p_render_it;
    }
    else
    {
        _EXCEPTION("Unknown entity id : " + p_entity_id);
    }
}

void SceneStreamerSystem::register_to_queues(const json::Channels& p_channels, mage::core::Entity* p_entity)
{
    const auto renderingHelper{ mage::helpers::RenderingChannels::getInstance() };

    const std::unordered_map<std::string, rendering::RenderState::Operation> rs_op_aig
    {
        { "SETCULLING", rendering::RenderState::Operation::SETCULLING },
        { "ENABLEZBUFFER", rendering::RenderState::Operation::ENABLEZBUFFER },
        { "SETTEXTUREFILTERTYPE", rendering::RenderState::Operation::SETTEXTUREFILTERTYPE },
        { "SETVERTEXTEXTUREFILTERTYPE", rendering::RenderState::Operation::SETVERTEXTEXTUREFILTERTYPE },
        { "SETFILLMODE", rendering::RenderState::Operation::SETFILLMODE },
        { "ALPHABLENDENABLE", rendering::RenderState::Operation::ALPHABLENDENABLE },
        { "ALPHABLENDOP", rendering::RenderState::Operation::ALPHABLENDOP },
        { "ALPHABLENDFUNC", rendering::RenderState::Operation::ALPHABLENDFUNC },
        { "ALPHABLENDDEST", rendering::RenderState::Operation::ALPHABLENDDEST },
        { "ALPHABLENDSRC", rendering::RenderState::Operation::ALPHABLENDSRC },
    };

    helpers::ChannelsRendering channelsRendering;

    const std::unordered_map<std::string, helpers::ChannelConfig>& default_channel_configs_list{ renderingHelper->getPassConfigsList() };

    for (const auto& config : p_channels.configs)
    {       
        for (const auto& e : default_channel_configs_list)
        {
            if (m_scene_entities_rg_parts.at(p_entity->getId()).count(e.first) > 0 && e.second.rendering_channel_type == config.rendering_channel_type)
            {
                helpers::ChannelConfig default_channel_config{ renderingHelper->getChannelConfig(e.second.queue_entity_id) };

                // manage shaders
                default_channel_config.vshader = config.vshader;
                default_channel_config.pshader = config.pshader;

                // manage renderstates
                // TODO : extended arguments management missing !!!!

                for (const auto& rs_json : config.rs_list)
                {
                    const rendering::RenderState::Operation rs_json_ope{ rs_op_aig.at(rs_json.operation) };

                    bool found_ope{ false };

                    for (auto& rs : default_channel_config.rs_list)
                    {
                        if (rs.getOperation() == rs_json_ope)
                        {
                            found_ope = true;

                            // if exists by default and arg is different, update it
                            if (rs.getArg() != rs_json.argument)
                            {
                                rs.setArg(rs_json.argument);
                            }
                            break;
                        }
                    }

                    if (!found_ope)
                    {
                        // rs ope defined from json not found in default config, so add it

                        rendering::RenderState rs;

                        rs.setOperation(rs_json_ope);
                        rs.setArg(rs_json.argument);

                        default_channel_config.rs_list.push_back(rs);
                    }
                }

                // manage textures files
                for (const auto& texturefile_json : config.textures_files_list)
                {
                    default_channel_config.textures_files_list.push_back(std::make_pair(texturefile_json.stage, std::make_pair(texturefile_json.filename, Texture())));
                }

                // manage rendering order
                if (config.rendering_order != -1)
                {
                    default_channel_config.rendering_order = config.rendering_order;
                }

                default_channel_config.queue_entity_id = e.second.queue_entity_id;
                channelsRendering.configs[e.second.queue_entity_id] = default_channel_config;
            }
        }
    }
   
    for (const auto& pass_vshader_param : p_channels.vertex_shaders_params)
    {
        for (const auto& e : default_channel_configs_list)
        {
            if (e.second.rendering_channel_type == pass_vshader_param.rendering_channel_type)
            {
                for (const auto vshader_param : pass_vshader_param.shaders_params)
                {
                    channelsRendering.vertex_shaders_params[e.second.queue_entity_id].push_back(std::make_pair(vshader_param.datacloud_name, vshader_param.param_name));
                }
            }
        }
    }

    for (const auto& pass_pshader_param : p_channels.pixel_shaders_params)
    {

        for (const auto& e : default_channel_configs_list)
        {
            if (e.second.rendering_channel_type == pass_pshader_param.rendering_channel_type)
            {
                for (const auto pshader_param : pass_pshader_param.shaders_params)
                {
                    channelsRendering.pixel_shaders_params[e.second.queue_entity_id].push_back(std::make_pair(pshader_param.datacloud_name, pshader_param.param_name));
                }
            }
        }
    }

    const auto rendering_proxies{ renderingHelper->registerToQueues(m_entitygraph, p_entity, channelsRendering) };

    m_rendering_proxies[p_entity->getId()] = rendering_proxies;
}

void SceneStreamerSystem::unregister_from_queues(mage::core::Entity* p_entity)
{
    const auto rendering_proxies = m_rendering_proxies.at(p_entity->getId());

    const auto renderingHelper{ mage::helpers::RenderingChannels::getInstance() };
    renderingHelper->unregisterFromQueues(m_entitygraph, p_entity, rendering_proxies);

    m_rendering_proxies.erase(p_entity->getId());
}


bool SceneStreamerSystem::is_inside_quadtreenode(const SceneQuadTreeNode& p_qtn, const core::maths::Matrix& p_global_pos)
{
    bool inside{ false };

    Real2Vector object_pos(p_global_pos(3, 0), p_global_pos(3, 2));

    if (p_qtn.xz_min.x() <= object_pos.x() && object_pos.x() < p_qtn.xz_max.x() &&
        p_qtn.xz_min.y() <= object_pos.y() && object_pos.y() < p_qtn.xz_max.y())
    {
        inside = true;
    }
    return inside;
}

bool SceneStreamerSystem::is_inside_quadtreenode(const SceneOctreeNode& p_otn, const core::maths::Matrix& p_global_pos)
{
    bool inside{ false };

    Real3Vector object_pos(p_global_pos(3, 0), p_global_pos(3, 1), p_global_pos(3, 2));

    if (p_otn.xyz_min.x() <= object_pos.x() && object_pos.x() < p_otn.xyz_max.x() &&
        p_otn.xyz_min.y() <= object_pos.y() && object_pos.y() < p_otn.xyz_max.y() &&
        p_otn.xyz_min.z() <= object_pos.z() && object_pos.z() < p_otn.xyz_max.z())
    {
        inside = true;
    }
    return inside;
}

void SceneStreamerSystem::dumpXTree()
{
    _MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> XTREE DUMP BEGIN <<<<<<<<<<<<<<<<<<<<<<<<")

    for (const auto& rgpd : m_rendergraphpart_data)
    {
        _MAGE_DEBUG(m_localLogger, "for ViewGroup " + rgpd.first)

        rgpd.second.quadtree_root->traverse([&](const SceneQuadTreeNode& p_data, size_t p_depth)
        {
            std::string tab;
            for (size_t i = 0; i < p_depth; i++) tab = tab + " ";

            _MAGE_DEBUG(m_localLogger, tab + "depth = " + std::to_string(p_depth)

                + " side_length = " + std::to_string(p_data.side_length)
                + " position = " + std::to_string(p_data.position[0]) + " " + std::to_string(p_data.position[1])

                + " xz min = " + std::to_string(p_data.xz_min[0]) + " " + std::to_string(p_data.xz_min[1])
                + " xz max = " + std::to_string(p_data.xz_max[0]) + " " + std::to_string(p_data.xz_max[1]))
                for (const auto& e : p_data.entities)
                {
                    const auto& world_aspect{ e->aspectAccess(worldAspect::id) };

                    const auto& entity_worldposition_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };
                    auto& entity_worldposition{ entity_worldposition_list.at(0)->getPurpose() };
                    const auto global_pos = entity_worldposition.global_pos;

                    _MAGE_DEBUG(m_localLogger, tab + e->getId() + " position = " + std::to_string(global_pos(3, 0)) + " " + std::to_string(global_pos(3, 2)));
                }
        });   
    }
    _MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> XTREE DUMP END <<<<<<<<<<<<<<<<<<<<<<<<")
}

void SceneStreamerSystem::dumpXTreeEntities()
{
    _MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> XTREE ENTITIES BEGIN <<<<<<<<<<<<<<<<<<<<<<<<")

    for (const auto& rgpd : m_rendergraphpart_data)
    {
        _MAGE_DEBUG(m_localLogger, "for ViewGroup " + rgpd.first)

        for (const auto& e : rgpd.second.xtree_entities)
        {
            const core::Entity* entity{ e.second.entity };
            const auto& world_aspect{ entity->aspectAccess(worldAspect::id) };

            const auto& entity_worldposition_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };
            auto& entity_worldposition{ entity_worldposition_list.at(0)->getPurpose() };
            const auto global_pos = entity_worldposition.global_pos;


            if (e.second.quadtree_node)
            {
                const SceneQuadTreeNode data{ e.second.quadtree_node->getData() };

                _MAGE_DEBUG(m_localLogger, e.first + " position = " + std::to_string(global_pos(3, 0)) + " " + std::to_string(global_pos(3, 2))
                    + " tree -> xz min = " + std::to_string(data.xz_min[0]) + " " + std::to_string(data.xz_min[1])
                    + " xz max = " + std::to_string(data.xz_max[0]) + " " + std::to_string(data.xz_max[1]) + " depth = " 
                    + std::to_string(e.second.quadtree_node->getDepth()) + " side length = " + std::to_string(data.side_length) )
            }
            else
            {
                _MAGE_DEBUG(m_localLogger, e.first + " position = " + std::to_string(global_pos(3, 0)) + " " + std::to_string(global_pos(3, 2))
                    + " NO SceneQuadTreeNode !!!")
            }
        }
    }

    _MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> XTREE ENTITIES END <<<<<<<<<<<<<<<<<<<<<<<<")
}

std::string SceneStreamerSystem::filter_arguments_stack(const std::string p_input, const std::unordered_map<std::string, std::string> p_file_args)
{
    if (p_file_args.find(p_input) == p_file_args.end())
    {
        return p_input;
    }
    else
    {
        return p_file_args.at(p_input);
    }
}