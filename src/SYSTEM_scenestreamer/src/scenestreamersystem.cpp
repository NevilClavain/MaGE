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
#include "animators_helpers.h"
#include "matrixfactory.h"


using namespace mage;
using namespace mage::core;

SceneStreamerSystem::SceneStreamerSystem(Entitygraph& p_entitygraph) : System(p_entitygraph)
{
}

void SceneStreamerSystem::run()
{
}

void SceneStreamerSystem::buildRendergraphPart(const std::string& p_jsonsource, const std::string& p_parentEntityId,
                                                int p_w_width, int p_w_height, float p_characteristics_v_width, float p_characteristics_v_height)
{
    json::RenderingTargetNodesCollection rtc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(rtc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse rendergraph: " + errorStr);
    }

    const std::function<void(const json::RenderingTargetNode&, const std::string&, int)> browseHierarchy
    {
        [&](const json::RenderingTargetNode& p_node, const std::string& p_parent_id, int depth)
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

            for (const auto& input : p_node.inputs)
            {
                int final_w_width = (fillWithWindowDims == input.buffer_texture.width ? p_w_width : input.buffer_texture.width);
                int final_h_width = (fillWithWindowDims == input.buffer_texture.height ? p_w_height : input.buffer_texture.height);

                const auto input_channnel{ Texture(texture_format_translation.at(input.buffer_texture.format_descr), final_w_width, final_h_width) };

                inputs.push_back(std::make_pair(input.stage, input_channnel));
            }

            int final_v_width = (fillWithViewportDims == p_node.width ? p_w_width : p_characteristics_v_width);
            int final_v_height = (fillWithViewportDims == p_node.height ? p_w_height : p_characteristics_v_height);

            const std::string queue_name{ p_node.descr + "_queue" };

            const std::string entity_queue_name{ p_node.descr + "Buffer_queue" };
            const std::string entity_target_name{ p_node.descr + "Buffer_targetquad" };
            const std::string entity_view_name{ p_node.descr + "Buffer_view" };

            mage::helpers::plugRenderingTarget(m_entitygraph,
                queue_name,
                final_v_width, final_v_height,
                p_parent_id,
                entity_queue_name,
                entity_target_name,
                entity_view_name,
                p_node.shaders.at(0).name,
                p_node.shaders.at(1).name,
                inputs,
                p_node.target_stage);


            // plug shaders args

            Entity* quad_ent{ m_entitygraph.node(entity_target_name).data() };
            auto& rendering_aspect{ quad_ent->aspectAccess(core::renderingAspect::id) };

            rendering::DrawingControl& dc{ rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };

            const auto& vshader{ p_node.shaders.at(0) };
            for (const auto& arg : vshader.args)
            {
                dc.vshaders_map.push_back(std::make_pair(arg.source, arg.destination));
            }

            const auto& pshader{ p_node.shaders.at(1) };
            for (const auto& arg : pshader.args)
            {
                dc.vshaders_map.push_back(std::make_pair(arg.source, arg.destination));
            }

            // recursive call
            for (auto& e : p_node.subs)
            {
                browseHierarchy(e, entity_target_name, depth + 1);
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
    json::ScenegraphNodesCollection sgc;

    JS::ParseContext parseContext(p_jsonsource);
    if (parseContext.parseTo(sgc) != JS::Error::NoError)
    {
        const auto errorStr{ parseContext.makeErrorString() };
        _EXCEPTION("Cannot parse scenegraph: " + errorStr);
    }

    const std::function<void(const json::ScenegraphNode&, const std::string&, int)> browseHierarchy
    {
        [&](const json::ScenegraphNode& p_node, const std::string& p_parent_id, int depth)
        {
            if ("" != p_node.helper)
            {
                if ("plugCamera" == p_node.helper)
                {
                    helpers::plugCamera(m_entitygraph, p_perspective_projection, p_parent_id, p_node.descr);
                }
            }
            else
            {
                if (p_node.world_aspect.animators.size() > 0)
                {
                    auto& entityNode{ m_entitygraph.add(m_entitygraph.node(p_parentEntityId), p_node.descr) };
                    const auto entity{ entityNode.data() };

                    auto& time_aspect{ entity->makeAspect(core::timeAspect::id) };

                    auto& world_aspect{ entity->makeAspect(core::worldAspect::id) };

                    for (const auto& animator : p_node.world_aspect.animators)
                    {
                        if ("gimbalLockJoin" == animator.helper)
                        {
                            world_aspect.addComponent<transform::WorldPosition>("gbl_output");

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
                                    {"gimbalLockJointAnim.output", "gbl_output"}

                                }, helpers::makeGimbalLockJointAnimator())
                            );
                        }
                        else if ("matrixFactory" == animator.helper)
                        {
                            mage::transform::MatrixFactory matrix_factory(animator.matrix_factory.type);

                            if ("" != animator.matrix_factory.xyzw_datacloud_value.descr)
                            {
                                world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real4Vector>>(animator.matrix_factory.xyzw_datacloud_value.descr,
                                                                                                                                    animator.matrix_factory.xyzw_datacloud_value.var_name);
                                                            
                                matrix_factory.setXYZWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real4Vector>>(animator.matrix_factory.xyzw_datacloud_value.descr)->getPurpose());
                            }
                            else if("" != animator.matrix_factory.xyzw_direct_value.descr)
                            {
                                world_aspect.addComponent<mage::transform::DirectValueMatrixSource<core::maths::Real4Vector>>(animator.matrix_factory.xyzw_direct_value.descr,
                                                                                                                               core::maths::Real4Vector(animator.matrix_factory.xyzw_direct_value.x,
                                                                                                                                                        animator.matrix_factory.xyzw_direct_value.y,
                                                                                                                                                        animator.matrix_factory.xyzw_direct_value.z,
                                                                                                                                                        animator.matrix_factory.xyzw_direct_value.w));

                                matrix_factory.setXYZWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<core::maths::Real4Vector>>(animator.matrix_factory.xyzw_direct_value.descr)->getPurpose());
                            }
                            else
                            {
                                if ("" != animator.matrix_factory.xyz_datacloud_value.descr)
                                {
                                    world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real3Vector>>(animator.matrix_factory.xyz_datacloud_value.descr,
                                                                                                                                        animator.matrix_factory.xyz_datacloud_value.var_name);

                                    matrix_factory.setXYZSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<core::maths::Real3Vector>>(animator.matrix_factory.xyz_datacloud_value.descr)->getPurpose());

                                    // W
                                    if ("" != animator.matrix_factory.w_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr, animator.matrix_factory.w_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.w_syncvar_value.descr)
                                    {
                                        const auto sync_var{ buildSyncVariableFromJson(animator.matrix_factory.w_syncvar_value.sync_var) };
                                        time_aspect.addComponent<SyncVariable>(animator.matrix_factory.w_syncvar_value.descr + " sync_var", sync_var);

                                        // TO BE CONTINUED....                                     
                                    }
                                    else if ("" != animator.matrix_factory.w_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr, animator.matrix_factory.w_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr)->getPurpose());
                                    }
                                }
                                else if ("" != animator.matrix_factory.xyz_direct_value.descr)
                                {
                                    world_aspect.addComponent<mage::transform::DirectValueMatrixSource<core::maths::Real3Vector>>(animator.matrix_factory.xyz_direct_value.descr,
                                                                                                                                    core::maths::Real3Vector(animator.matrix_factory.xyzw_direct_value.x,
                                                                                                                                                                animator.matrix_factory.xyzw_direct_value.y,
                                                                                                                                                                animator.matrix_factory.xyzw_direct_value.z));

                                    matrix_factory.setXYZSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<core::maths::Real3Vector>>(animator.matrix_factory.xyz_datacloud_value.descr)->getPurpose());

                                    // W
                                    if ("" != animator.matrix_factory.w_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr, animator.matrix_factory.w_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.w_syncvar_value.descr)
                                    {
                                        // TODO
                                    }
                                    else if ("" != animator.matrix_factory.w_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr, animator.matrix_factory.w_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr)->getPurpose());
                                    }
                                }
                                else
                                {
                                    // X
                                    if ("" != animator.matrix_factory.x_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.x_datacloud_value.descr, animator.matrix_factory.x_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.x_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.x_syncvar_value.descr)
                                    {
                                        // TODO
                                    }
                                    else if ("" != animator.matrix_factory.x_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.x_direct_value.descr, animator.matrix_factory.x_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.x_direct_value.descr)->getPurpose());
                                    }

                                    // Y
                                    if ("" != animator.matrix_factory.y_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.y_datacloud_value.descr, animator.matrix_factory.y_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.y_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.y_syncvar_value.descr)
                                    {
                                        // TODO
                                    }
                                    else if ("" != animator.matrix_factory.y_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.y_direct_value.descr, animator.matrix_factory.y_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.y_direct_value.descr)->getPurpose());
                                    }

                                    // Z
                                    if ("" != animator.matrix_factory.z_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.z_datacloud_value.descr, animator.matrix_factory.z_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.z_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.z_syncvar_value.descr)
                                    {
                                        // TODO
                                    }
                                    else if ("" != animator.matrix_factory.z_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.z_direct_value.descr, animator.matrix_factory.z_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.z_direct_value.descr)->getPurpose());
                                    }

                                    // W
                                    if ("" != animator.matrix_factory.w_datacloud_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr, animator.matrix_factory.w_datacloud_value.var_name);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DatacloudValueMatrixSource<double>>(animator.matrix_factory.w_datacloud_value.descr)->getPurpose());
                                    }
                                    else if ("" != animator.matrix_factory.w_syncvar_value.descr)
                                    {
                                        // TODO
                                    }
                                    else if ("" != animator.matrix_factory.w_direct_value.descr)
                                    {
                                        world_aspect.addComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr, animator.matrix_factory.w_direct_value.value);
                                        matrix_factory.setWSource(&world_aspect.getComponent<mage::transform::DirectValueMatrixSource<double>>(animator.matrix_factory.w_direct_value.descr)->getPurpose());
                                    }
                                }
                            }

                            world_aspect.addComponent<mage::transform::MatrixFactory>(animator.matrix_factory.descr, matrix_factory);
                        }
                    }
                }
            }

            // recursive call
            for (auto& e : p_node.subs)
            {
                browseHierarchy(e, p_node.descr, depth + 1);
            }
        }
    };

    for (const auto& e : sgc.subs)
    {
        browseHierarchy(e, p_parentEntityId, 0);
    }
}

core::SyncVariable SceneStreamerSystem::buildSyncVariableFromJson(const json::SyncVariable& p_syncvar)
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