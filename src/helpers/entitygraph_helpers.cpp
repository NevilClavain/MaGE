
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

#include <map>
#include <string>
#include <functional>
#include <utility>

#include "entitygraph.h"
#include "entity.h"
#include "aspects.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"

#include "renderingqueue.h"

#include "syncvariable.h"
#include "worldposition.h"
#include "matrix.h"

#include "shader.h"
#include "texture.h"
#include "trianglemeshe.h"
#include "renderstate.h"

#include "animatorfunc.h"

extern mage::core::logger::Sink localLogger("Helpers", mage::core::logger::Configuration::getInstance());

namespace mage
{
	namespace helpers
	{
		void logEntitygraph(core::Entitygraph& p_eg, bool p_log_entity_id_only)
		{
			_MAGE_DEBUG(localLogger, ">>>>>>>>>>>>>>> ENTITY GRAPH DUMP BEGIN <<<<<<<<<<<<<<<<<<<<<<<<");

			struct ENode
			{
				std::string					 id;
				std::map<std::string, ENode> children;
			};

			ENode root;

			// build node tree that will be dumped to log
			for (auto it = p_eg.preBegin(); it != p_eg.preEnd(); ++it)
			{
				const mage::core::Entity* current_entity { it->data() };
				const std::string currId{ current_entity->getId() };

				const std::function<void(ENode&, const std::string&, const std::string&)> search
				{
					[&](ENode& p_node, const std::string& p_parentId, const std::string& p_id)
					{
						if (p_node.id == p_parentId)
						{
							ENode child;
							child.id = p_id;
							// found parent
							p_node.children[p_id] = child;
						}

						for (auto& e : p_node.children)
						{
							search(e.second, p_parentId, p_id);
						}						
					}
				};

				const auto parent_entity{ current_entity->getParent() };
				if (parent_entity)
				{
					const std::string parentId{ parent_entity->getId() };
					search(root, parentId, currId);
				}
				else
				{
					// create root;
					root.id = currId;
				}
			}

			// dump to log the built node tree

			std::string hierarchy;

			const std::function<void(const ENode&, int, std::string&)> logMe
			{
				[&](const ENode& p_node, int depth, std::string& p_outstr)
				{
					const auto& eg_node { p_eg.node(p_node.id) };

					const core::Entity* curr_entity{ eg_node.data() };
					
					std::string logstr;

					// entity name
					logstr += "\n";
					for (int i = 0; i < depth; i++)
					{
						logstr += "\t";
					}					
					logstr += "ENTITY_ID = " + p_node.id;

					if (!p_log_entity_id_only)
					{
						// aspects of this entity
						const std::map<int, std::string> aspects_translate
						{
							{ core::teapotAspect::id,		"teapotAspect" },
							{ core::renderingAspect::id,	"renderingAspect" },
							{ core::timeAspect::id,			"timeAspect" },
							{ core::resourcesAspect::id,	"resourcesAspect" },
							{ core::cameraAspect::id,		"cameraAspect" },
							{ core::worldAspect::id,		"worldAspect" },
						};

						for (const auto& e : aspects_translate)
						{
							if (curr_entity->hasAspect(e.first))
							{
								// log aspect name

								logstr += "\n";
								for (int i = 0; i < depth + 1; i++)
								{
									logstr += "\t";
								}

								logstr += e.second;

								/////////////////////
								const auto& cc{ curr_entity->aspectAccess(e.first) };
								const std::unordered_map<std::string, std::string>& comp_id_list{ cc.getComponentsIdWithTypeStrList() };

								for (const auto& c : comp_id_list)
								{
									logstr += "\n";
									for (int i = 0; i < depth + 2; i++)
									{
										logstr += "\t";
									}
									logstr += c.first;
									logstr += " ";
									logstr += c.second;
								}
							}
						}
					}

					//_MAGE_DEBUG(localLogger, logstr);

					p_outstr += logstr;

					for (auto& e : p_node.children)
					{
						logMe(e.second, depth+1, p_outstr);
					}
				}
			};

			logMe(root, 0, hierarchy);	

			_MAGE_DEBUG(localLogger, hierarchy);

			_MAGE_DEBUG(localLogger, ">>>>>>>>>>>>>>> ENTITY GRAPH DUMP END <<<<<<<<<<<<<<<<<<<<<<<<");
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		// LEGACY, TO REMOVE

		void plugRenderingQuadView(mage::core::Entitygraph& p_entitygraph,
			float p_characteristics_v_width, float p_characteristics_v_height,
			const std::string& p_parentid,
			const std::string& p_quadEntityid,
			const std::string& p_viewEntityid,
			mage::rendering::Queue* p_windowQueue,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::vector<std::pair<size_t, Texture>>& p_renderTargets
		)
		{
			/////////////// add viewpoint (camera) ////////////////////////

			auto& parentNodeNode{ p_entitygraph.node(p_parentid) };


			auto& viewPointNode{ p_entitygraph.add(parentNodeNode, p_viewEntityid) };
			const auto cameraEntity{ viewPointNode.data() };

			auto& camera_aspect{ cameraEntity->makeAspect(core::cameraAspect::id) };

			core::maths::Matrix projection;

			projection.perspective(p_characteristics_v_width, p_characteristics_v_height, 1.0, 10.00000000000);

			camera_aspect.addComponent<core::maths::Matrix>("projection", projection);

			auto& camera_world_aspect{ cameraEntity->makeAspect(core::worldAspect::id) };

			camera_world_aspect.addComponent<transform::WorldPosition>("camera_position", transform::WorldPosition());

			p_windowQueue->setCurrentView(p_viewEntityid);

			///////////////////////////////////////////////////////////////

			// add target quad aligned with screen

			auto& screenRenderingQuadNode{ p_entitygraph.add(parentNodeNode, p_quadEntityid) };
			const auto screenRenderingQuadEntity{ screenRenderingQuadNode.data() };


			/// RESOURCE ASPECT
			auto& quad_resource_aspect{ screenRenderingQuadEntity->makeAspect(core::resourcesAspect::id) };

			/////////// Add shaders

			quad_resource_aspect.addComponent<std::pair<std::string, Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			quad_resource_aspect.addComponent<std::pair<std::string, Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add trianglemeshe
			TriangleMeshe square;

			square.push(Vertex(-p_characteristics_v_width / 2, -p_characteristics_v_height / 2, 0.0, 0, 1));
			square.push(Vertex(p_characteristics_v_width / 2, -p_characteristics_v_height / 2, 0.0, 1, 1));
			square.push(Vertex(p_characteristics_v_width / 2, p_characteristics_v_height / 2, 0.0, 1, 0));
			square.push(Vertex(-p_characteristics_v_width / 2, p_characteristics_v_height / 2, 0.0, 0, 0));

			const TrianglePrimitive<unsigned int> t1{ 0, 1, 2 };
			square.push(t1);

			const TrianglePrimitive<unsigned int> t2{ 0, 2, 3 };
			square.push(t2);

			square.computeResourceUID();
			square.setSourceID("helpers::plugRenderingQuadView");
			square.setSource(TriangleMeshe::Source::CONTENT_DYNAMIC_INIT);

			square.setState(TriangleMeshe::State::BLOBLOADED);

			quad_resource_aspect.addComponent<TriangleMeshe>("quad", square);

			/// RENDERING ASPECT
			auto& quad_rendering_aspect{ screenRenderingQuadEntity->makeAspect(core::renderingAspect::id) };

			quad_rendering_aspect.addComponent<core::renderingAspect::renderingTarget>("eg.std.renderingTarget", core::renderingAspect::renderingTarget::BUFFER_RENDERINGTARGET);

			/////////// render target Texture

			const std::string texture_name_base{ "renderingquad_texture_" };

			for (size_t i = 0; i < p_renderTargets.size(); i++)
			{
				quad_resource_aspect.addComponent<std::pair<size_t, Texture>>(texture_name_base + std::to_string(p_renderTargets.at(i).first), p_renderTargets.at(i));
			}

			/////////// Add renderstate

			rendering::RenderState rs_noculling(rendering::RenderState::Operation::SETCULLING, "cw");
			rendering::RenderState rs_zbuffer(rendering::RenderState::Operation::ENABLEZBUFFER, "false");
			rendering::RenderState rs_fill(rendering::RenderState::Operation::SETFILLMODE, "solid");

			const std::vector<rendering::RenderState> rs_list = { rs_noculling, rs_zbuffer, rs_fill };

			quad_rendering_aspect.addComponent<std::vector<rendering::RenderState>>("renderStates", rs_list);


			/////////// Draw triangles

			rendering::DrawingControl drawingControl;
			quad_rendering_aspect.addComponent<rendering::DrawingControl>("screenRenderingQuad", drawingControl);


			/////////// time aspect
			// required for animator !

			screenRenderingQuadEntity->makeAspect(core::timeAspect::id);

			/////////// World position

			auto& world_aspect{ screenRenderingQuadEntity->makeAspect(core::worldAspect::id) };

			world_aspect.addComponent<transform::WorldPosition>("position");


			world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
			(
				{},
				[](const core::ComponentContainer& p_world_aspect,
					const core::ComponentContainer& p_time_aspect,
					const transform::WorldPosition&,
					const std::unordered_map<std::string, std::string>&)
				{

					core::maths::Matrix positionmat;
					positionmat.translation(0.0, 0.0, -1.00001);

					transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
					wp.local_pos = wp.local_pos * positionmat;
				}
			));
		}




		////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		rendering::Queue& plugRenderingQueue(mage::core::Entitygraph& p_entitygraph, const rendering::Queue& p_renderingqueue, const std::string& p_parentid, const std::string& p_entityid)
		{
			core::Entitygraph::Node& parentNode{ p_entitygraph.node(p_parentid) };
			auto& renderingQueueNode{ p_entitygraph.add(parentNode, p_entityid) };
			const auto renderingQueueNodeEntity{ renderingQueueNode.data() };

			auto& renderingAspect{ renderingQueueNodeEntity->makeAspect(core::renderingAspect::id) };

			renderingAspect.addComponent<rendering::Queue>("renderingQueue", p_renderingqueue);
			auto& stored_rendering_queue{ renderingAspect.getComponent<rendering::Queue>("renderingQueue")->getPurpose() };

			return stored_rendering_queue;
		}
		///////////////////////////////////////////////////////////////////////////////////////////////////////////

		rendering::Queue& plugRenderingQuad(mage::core::Entitygraph& p_entitygraph,
											const std::string& p_queue_debug_name,
											float p_characteristics_v_width, float p_characteristics_v_height,
											const std::string& p_parentid,
											const std::string& p_queueEntityid,
											const std::string& p_quadEntityid,
											const std::string& p_viewEntityid,
											const std::string& p_vshader,
											const std::string& p_pshader,
											const std::vector<std::pair<size_t, Texture>>& p_renderTargets,
											size_t p_target_stage)
		{

			rendering::Queue rendering_queue(p_queue_debug_name);
			rendering_queue.setTargetStage(p_target_stage);

			auto& rendering_queue_ref{ mage::helpers::plugRenderingQueue(p_entitygraph, rendering_queue, p_parentid, p_queueEntityid) };

			/////////////// add viewpoint (camera) ////////////////////////

			auto& parentNodeNode{ p_entitygraph.node(p_queueEntityid) };


			auto& viewPointNode{ p_entitygraph.add(parentNodeNode, p_viewEntityid) };
			const auto cameraEntity{ viewPointNode.data() };

			auto& camera_aspect{ cameraEntity->makeAspect(core::cameraAspect::id) };

			core::maths::Matrix projection;

			projection.perspective(p_characteristics_v_width, p_characteristics_v_height, 1.0, 10.00000000000);

			camera_aspect.addComponent<core::maths::Matrix>("projection", projection);

			auto& camera_world_aspect{ cameraEntity->makeAspect(core::worldAspect::id) };

			camera_world_aspect.addComponent<transform::WorldPosition>("camera_position", transform::WorldPosition());

			//p_queue->setCurrentView(p_viewEntityid);
			rendering_queue_ref.setCurrentView(p_viewEntityid);

			///////////////////////////////////////////////////////////////

			// add target quad aligned with screen

			auto& screenRenderingQuadNode{ p_entitygraph.add(parentNodeNode, p_quadEntityid) };
			const auto screenRenderingQuadEntity{ screenRenderingQuadNode.data() };


			/// RESOURCE ASPECT
			auto& quad_resource_aspect{ screenRenderingQuadEntity->makeAspect(core::resourcesAspect::id) };

			/////////// Add shaders

			quad_resource_aspect.addComponent<std::pair<std::string,Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			quad_resource_aspect.addComponent<std::pair<std::string,Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add trianglemeshe
			TriangleMeshe square;

			square.push(Vertex(-p_characteristics_v_width / 2, -p_characteristics_v_height / 2, 0.0, 0, 1));
			square.push(Vertex(p_characteristics_v_width / 2, -p_characteristics_v_height / 2, 0.0, 1, 1));
			square.push(Vertex(p_characteristics_v_width / 2, p_characteristics_v_height / 2, 0.0, 1, 0));
			square.push(Vertex(-p_characteristics_v_width / 2, p_characteristics_v_height / 2, 0.0, 0, 0));

			const TrianglePrimitive<unsigned int> t1{ 0, 1, 2 };
			square.push(t1);

			const TrianglePrimitive<unsigned int> t2{ 0, 2, 3 };
			square.push(t2);

			square.computeResourceUID();
			square.setSourceID("helpers::plugRenderingQuadView");
			square.setSource(TriangleMeshe::Source::CONTENT_DYNAMIC_INIT);
			
			square.setState(TriangleMeshe::State::BLOBLOADED);

			quad_resource_aspect.addComponent<TriangleMeshe>("quad", square);

			/// RENDERING ASPECT
			auto& quad_rendering_aspect{ screenRenderingQuadEntity->makeAspect(core::renderingAspect::id) };

			quad_rendering_aspect.addComponent<core::renderingAspect::renderingTarget>("eg.std.renderingTarget", core::renderingAspect::renderingTarget::BUFFER_RENDERINGTARGET);

			/////////// render target Texture

			const std::string texture_name_base{ "renderingquad_texture_" };

			for (size_t i = 0; i < p_renderTargets.size(); i++)
			{
				quad_resource_aspect.addComponent<std::pair<size_t, Texture>>(texture_name_base + std::to_string(p_renderTargets.at(i).first), p_renderTargets.at(i));
			}

			/////////// Add renderstate

			rendering::RenderState rs_noculling(rendering::RenderState::Operation::SETCULLING, "cw");
			rendering::RenderState rs_zbuffer(rendering::RenderState::Operation::ENABLEZBUFFER, "false");
			rendering::RenderState rs_fill(rendering::RenderState::Operation::SETFILLMODE, "solid");

			const std::vector<rendering::RenderState> rs_list = { rs_noculling, rs_zbuffer, rs_fill };

			quad_rendering_aspect.addComponent<std::vector<rendering::RenderState>>("renderStates", rs_list);


			/////////// Draw triangles

			rendering::DrawingControl drawingControl;
			quad_rendering_aspect.addComponent<rendering::DrawingControl>("drawingControl", drawingControl);


			/////////// time aspect
			// required for animator !

			screenRenderingQuadEntity->makeAspect(core::timeAspect::id);

			/////////// World position

			auto& world_aspect{ screenRenderingQuadEntity->makeAspect(core::worldAspect::id) };

			world_aspect.addComponent<transform::WorldPosition>("position");


			world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
			(
				{},
				[](const core::ComponentContainer& p_world_aspect,
					const core::ComponentContainer& p_time_aspect,
					const transform::WorldPosition&,
					const std::unordered_map<std::string, std::string>&)
				{

					core::maths::Matrix positionmat;
					positionmat.translation(0.0, 0.0, -1.00001);

					transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
					wp.local_pos = wp.local_pos * positionmat;
				}
			));

			//////////////////////////////////////

			return rendering_queue_ref;			
		}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////

		void plugCamera(mage::core::Entitygraph& p_entitygraph,
			const core::maths::Matrix& p_projection,
			const std::string& p_parentid, const std::string& p_entityid)
		{
			core::Entitygraph::Node& parentNode{ p_entitygraph.node(p_parentid) };
			auto& viewPointNode{ p_entitygraph.add(parentNode, p_entityid) };
			const auto cameraEntity{ viewPointNode.data() };

			auto& camera_aspect{ cameraEntity->makeAspect(core::cameraAspect::id) };

			camera_aspect.addComponent<core::maths::Matrix>("projection", p_projection);
			auto& camera_world_aspect{ cameraEntity->makeAspect(core::worldAspect::id) };

			camera_world_aspect.addComponent<transform::WorldPosition>("camera_position", transform::WorldPosition());
		}

		void updateCameraProjection(mage::core::Entitygraph& p_entitygraph, const std::string& p_entityid, const core::maths::Matrix& p_projection)
		{
			core::Entitygraph::Node& viewPointNode{ p_entitygraph.node(p_entityid) };
			const auto cameraEntity{ viewPointNode.data() };
			auto& camera_aspect{ cameraEntity->aspectAccess(core::cameraAspect::id) };
			auto& stored_projection{ camera_aspect.getComponent<core::maths::Matrix>("projection")->getPurpose() };

			// update
			stored_projection = p_projection;
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////

		rendering::Queue* getRenderingQueue(mage::core::Entitygraph& p_entitygraph, const std::string& p_entityId)
		{
			core::Entitygraph::Node& node{ p_entitygraph.node(p_entityId) };
			const auto entity{ node.data() };
			const auto& renderingAspect{ entity->aspectAccess(core::renderingAspect::id) };

			auto renderingQueue { &renderingAspect.getComponent<rendering::Queue>("renderingQueue")->getPurpose() };
			return renderingQueue;
		}


		core::Entity* plug2DSpriteWithSyncVariables(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_spriteEntityid,
			const double p_spriteWidth,
			const double p_spriteHeight,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::string& p_texture,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			transform::WorldPosition::TransformationComposition p_composition_operation
		)
		{
			auto& parentNode{ p_entitygraph.node(p_parentid) };

			auto& sprite2DNode{ p_entitygraph.add(parentNode, p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			auto& resource_aspect{ sprite2DEntity->makeAspect(core::resourcesAspect::id) };

			/////////// Add shaders

			resource_aspect.addComponent<std::pair<std::string, Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			resource_aspect.addComponent<std::pair<std::string, Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add trianglemeshe
			TriangleMeshe sprite2D_square;


			sprite2D_square.push(Vertex(-p_spriteWidth / 2.0, -p_spriteHeight / 2.0, 0.0, 0.0f, 1.0f));
			sprite2D_square.push(Vertex(p_spriteWidth / 2.0, -p_spriteHeight / 2.0, 0.0, 1.0f, 1.0f));
			sprite2D_square.push(Vertex(p_spriteWidth / 2.0, p_spriteHeight / 2.0, 0.0, 1.0f, 0.0f));
			sprite2D_square.push(Vertex(-p_spriteWidth / 2.0, p_spriteHeight / 2.0, 0.0, 0.0f, 0.0f));

			const TrianglePrimitive<unsigned int> t1{ 0, 1, 2 };
			sprite2D_square.push(t1);

			const TrianglePrimitive<unsigned int> t2{ 0, 2, 3 };
			sprite2D_square.push(t2);

			sprite2D_square.computeResourceUID();
			sprite2D_square.setSourceID("sprite2DEntity");
			sprite2D_square.setSource(TriangleMeshe::Source::CONTENT_DYNAMIC_INIT);

			sprite2D_square.setState(TriangleMeshe::State::BLOBLOADED);

			resource_aspect.addComponent<TriangleMeshe>("sprite2D_square", sprite2D_square);

			/////////// Add texture
			resource_aspect.addComponent<std::pair<size_t, std::pair<std::string, Texture>>>("texture", std::make_pair(Texture::STAGE_0, std::make_pair(p_texture, Texture())));


			/////////// Add renderstate
			auto& rendering_aspect{ sprite2DEntity->makeAspect(core::renderingAspect::id) };

			rendering_aspect.addComponent<std::vector<rendering::RenderState>>("renderStates", p_renderstates_list);

			////////// Drawing control

			rendering::DrawingControl dc;
			rendering_aspect.addComponent<rendering::DrawingControl>("sprite2D_dc", dc);

			rendering_aspect.addComponent<int>("renderingorder", p_rendering_order);

			/////////// time aspect

			auto& time_aspect{ sprite2DEntity->makeAspect(core::timeAspect::id) };

			time_aspect.addComponent<core::SyncVariable>("x_pos", core::SyncVariable(core::SyncVariable::Type::POSITION, 0.0));
			time_aspect.addComponent<core::SyncVariable>("y_pos", core::SyncVariable(core::SyncVariable::Type::POSITION, 0.0));
			time_aspect.addComponent<core::SyncVariable>("z_rot", core::SyncVariable(core::SyncVariable::Type::ANGLE, 0.0));

			///////// world aspect

			auto& world_aspect{ sprite2DEntity->makeAspect(core::worldAspect::id) };
			transform::WorldPosition wp;
			wp.composition_operation = p_composition_operation;
			world_aspect.addComponent<transform::WorldPosition>("position", wp);


			world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
			(
				{},
				[](const core::ComponentContainer& p_world_aspect,
					const core::ComponentContainer& p_time_aspect,
					const transform::WorldPosition&,
					const std::unordered_map<std::string, std::string>&)
				{

					const auto& x_pos{ p_time_aspect.getComponent<core::SyncVariable>("x_pos")->getPurpose() };
					const auto& y_pos{ p_time_aspect.getComponent<core::SyncVariable>("y_pos")->getPurpose() };
					const auto& z_rot{ p_time_aspect.getComponent<core::SyncVariable>("z_rot")->getPurpose() };

					core::maths::Matrix positionmat;
					positionmat.translation(x_pos.value, y_pos.value, 0.0);

					core::maths::Matrix rotationmat;
					rotationmat.rotation(core::maths::Real3Vector(0, 0, 1), z_rot.value);

					transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
					wp.local_pos = wp.local_pos * rotationmat * positionmat;
				}
			));

			return sprite2DEntity;
		}

		core::SyncVariable& getXPosSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& time_aspect{ sprite2DEntity->aspectAccess(core::timeAspect::id) };

			core::SyncVariable& x_pos{ time_aspect.getComponent< core::SyncVariable>("x_pos")->getPurpose() };
			return x_pos;
		}

		core::SyncVariable& getYPosSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& time_aspect{ sprite2DEntity->aspectAccess(core::timeAspect::id) };

			core::SyncVariable& y_pos{ time_aspect.getComponent< core::SyncVariable>("y_pos")->getPurpose() };
			return y_pos;
		}

		core::SyncVariable& getZRotSync(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& time_aspect{ sprite2DEntity->aspectAccess(core::timeAspect::id) };

			core::SyncVariable& z_rot{ time_aspect.getComponent< core::SyncVariable>("z_rot")->getPurpose() };
			return z_rot;
		}

		core::Entity* plug2DSpriteWithPosition(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_spriteEntityid,
			const double p_spriteWidth,
			const double p_spriteHeight,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::string& p_texture,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			transform::WorldPosition::TransformationComposition p_composition_operation,
			float p_xpos = 0,
			float p_ypos = 0,
			float p_rot_radians = 0
		)
		{
			auto& parentNode{ p_entitygraph.node(p_parentid) };

			auto& sprite2DNode{ p_entitygraph.add(parentNode, p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			auto& resource_aspect{ sprite2DEntity->makeAspect(core::resourcesAspect::id) };

			/////////// Add shaders

			resource_aspect.addComponent<std::pair<std::string, Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			resource_aspect.addComponent<std::pair<std::string, Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add trianglemeshe
			TriangleMeshe sprite2D_square;


			sprite2D_square.push(Vertex(-p_spriteWidth / 2.0, -p_spriteHeight / 2.0, 0.0, 0.0f, 1.0f));
			sprite2D_square.push(Vertex(p_spriteWidth / 2.0, -p_spriteHeight / 2.0, 0.0, 1.0f, 1.0f));
			sprite2D_square.push(Vertex(p_spriteWidth / 2.0, p_spriteHeight / 2.0, 0.0, 1.0f, 0.0f));
			sprite2D_square.push(Vertex(-p_spriteWidth / 2.0, p_spriteHeight / 2.0, 0.0, 0.0f, 0.0f));

			const TrianglePrimitive<unsigned int> t1{ 0, 1, 2 };
			sprite2D_square.push(t1);

			const TrianglePrimitive<unsigned int> t2{ 0, 2, 3 };
			sprite2D_square.push(t2);

			sprite2D_square.computeResourceUID();
			sprite2D_square.setSourceID("sprite2DEntity");
			sprite2D_square.setSource(TriangleMeshe::Source::CONTENT_DYNAMIC_INIT);

			sprite2D_square.setState(TriangleMeshe::State::BLOBLOADED);

			resource_aspect.addComponent<TriangleMeshe>("sprite2D_square", sprite2D_square);

			/////////// Add texture
			resource_aspect.addComponent<std::pair<size_t, std::pair<std::string, Texture>>>("texture", std::make_pair(Texture::STAGE_0, std::make_pair(p_texture, Texture())));


			/////////// Add renderstate
			auto& rendering_aspect{ sprite2DEntity->makeAspect(core::renderingAspect::id) };

			rendering_aspect.addComponent<std::vector<rendering::RenderState>>("renderStates", p_renderstates_list);

			////////// Drawing control

			rendering::DrawingControl dc;
			rendering_aspect.addComponent<rendering::DrawingControl>("sprite2D_dc", dc);

			rendering_aspect.addComponent<int>("renderingorder", p_rendering_order);


			///////// world aspect

			auto& world_aspect{ sprite2DEntity->makeAspect(core::worldAspect::id) };

			transform::WorldPosition wp;
			wp.composition_operation = p_composition_operation;
			world_aspect.addComponent<transform::WorldPosition>("position", wp);


			world_aspect.addComponent<double>("x_pos", p_xpos);
			world_aspect.addComponent<double>("y_pos", p_ypos);
			world_aspect.addComponent<double>("z_rot", p_rot_radians);


			/////////// time aspect
			sprite2DEntity->makeAspect(core::timeAspect::id);

			world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
			(
				{},
				[](const core::ComponentContainer& p_world_aspect,
					const core::ComponentContainer& p_time_aspect,
					const transform::WorldPosition& p_wp,
					const std::unordered_map<std::string, std::string>&)
				{

					const auto& x_pos{ p_world_aspect.getComponent<double>("x_pos")->getPurpose() };
					const auto& y_pos{ p_world_aspect.getComponent<double>("y_pos")->getPurpose() };
					const auto& z_rot_rad{ p_world_aspect.getComponent<double>("z_rot")->getPurpose() };

					core::maths::Matrix positionmat;
					positionmat.translation(x_pos, y_pos, 0);

					core::maths::Matrix rotationmat;
					rotationmat.rotation(core::maths::Real3Vector(0, 0, 1), z_rot_rad);

					transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
					wp.local_pos = wp.local_pos * rotationmat * positionmat;
				}
			));

			return sprite2DEntity;
		}


		core::Entity* plugTextWithPosition(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_textEntityid,
			const mage::rendering::Queue::Text& p_queue_text,
			transform::WorldPosition::TransformationComposition p_composition_operation,
			float p_xpos = 0,
			float p_ypos = 0)
		{
			auto& parentNode{ p_entitygraph.node(p_parentid) };

			auto& textNode{ p_entitygraph.add(parentNode, p_textEntityid) };
			const auto textEntity{ textNode.data() };

			auto& rendering_aspect{ textEntity->makeAspect(core::renderingAspect::id) };

			rendering_aspect.addComponent<mage::rendering::Queue::Text>("queue_text", p_queue_text);

			auto& world_aspect{ textEntity->makeAspect(core::worldAspect::id) };
			transform::WorldPosition wp;
			wp.composition_operation = p_composition_operation;
			world_aspect.addComponent<transform::WorldPosition>("position", wp);

			world_aspect.addComponent<double>("x_pos", p_xpos);
			world_aspect.addComponent<double>("y_pos", p_ypos);


			world_aspect.addComponent<transform::Animator>("animator_positioning", transform::Animator
			(
				{},
				[](const core::ComponentContainer& p_world_aspect,
					const core::ComponentContainer& p_time_aspect,
					const transform::WorldPosition&,
					const std::unordered_map<std::string, std::string>&)
				{
					const auto& x_pos{ p_world_aspect.getComponent<double>("x_pos")->getPurpose() };
					const auto& y_pos{ p_world_aspect.getComponent<double>("y_pos")->getPurpose() };

					core::maths::Matrix positionmat;
					positionmat.translation(x_pos, y_pos, 0);

					transform::WorldPosition& wp{ p_world_aspect.getComponent<transform::WorldPosition>("position")->getPurpose() };
					wp.local_pos = wp.local_pos * positionmat;
				}
			));

			// time aspect
			textEntity->makeAspect(core::timeAspect::id);

			return textEntity;
		}

		double& getXPos(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& world_aspect{ sprite2DEntity->aspectAccess(core::worldAspect::id) };

			double& x_pos{ world_aspect.getComponent<double>("x_pos")->getPurpose() };
			return x_pos;
		}

		double& getYPos(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& world_aspect{ sprite2DEntity->aspectAccess(core::worldAspect::id) };

			double& y_pos{ world_aspect.getComponent<double>("y_pos")->getPurpose() };
			return y_pos;
		}

		double& getZRot(mage::core::Entitygraph& p_entitygraph, const std::string& p_spriteEntityid)
		{
			auto& sprite2DNode{ p_entitygraph.node(p_spriteEntityid) };
			const auto sprite2DEntity{ sprite2DNode.data() };

			const auto& world_aspect{ sprite2DEntity->aspectAccess(core::worldAspect::id) };

			double& z_rot{ world_aspect.getComponent<double>("z_rot")->getPurpose() };
			return z_rot;
		}



		core::Entity* plugRenderingProxyEntity(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_entityid,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			const std::vector< std::pair<size_t, std::pair<std::string, Texture>>>& p_textures)
		{
			auto& parentNode{ p_entitygraph.node(p_parentid) };
			auto& mesheNode{ p_entitygraph.add(parentNode, p_entityid) };
			const auto entity{ mesheNode.data() };

			auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
			auto& rendering_aspect{ entity->makeAspect(core::renderingAspect::id) };

			resource_aspect.addComponent<std::pair<std::string, Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			resource_aspect.addComponent<std::pair<std::string, Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add textures

			int index{ 0 };
			for (const auto& texture_descr : p_textures)
			{
				const std::string comp_id{ "texture" + std::to_string(index++) };

				resource_aspect.addComponent<std::pair<size_t, std::pair<std::string, Texture>>>(comp_id, texture_descr);
			}

			/////////// Add renderstate
			rendering_aspect.addComponent<std::vector<mage::rendering::RenderState>>("renderStates", p_renderstates_list);

			/////////// Draw triangles
			rendering::DrawingControl drawingControl;
			rendering_aspect.addComponent<mage::rendering::DrawingControl>("drawingControl", drawingControl);

			/////////// Rendering Order
			rendering_aspect.addComponent<int>("renderingOrder", p_rendering_order);

			return entity;
		}



		core::Entity* plugRenderingProxyEntityWithRenderedTargets(mage::core::Entitygraph& p_entitygraph,
			const std::string& p_parentid,
			const std::string& p_entityid,
			const std::string& p_vshader,
			const std::string& p_pshader,
			const std::vector<rendering::RenderState>& p_renderstates_list,
			int p_rendering_order,
			const std::vector<std::pair<size_t, Texture>>& p_textures)
		{
			auto& parentNode{ p_entitygraph.node(p_parentid) };
			auto& mesheNode{ p_entitygraph.add(parentNode, p_entityid) };
			const auto entity{ mesheNode.data() };

			auto& resource_aspect{ entity->makeAspect(core::resourcesAspect::id) };
			auto& rendering_aspect{ entity->makeAspect(core::renderingAspect::id) };

			resource_aspect.addComponent<std::pair<std::string, Shader>>("vertexShader", std::make_pair(p_vshader, Shader(vertexShader)));
			resource_aspect.addComponent<std::pair<std::string, Shader>>("pixelShader", std::make_pair(p_pshader, Shader(pixelShader)));

			/////////// Add textures

			int index{ 0 };
			for (const auto& texture_descr : p_textures)
			{
				const std::string comp_id{ "texture" + std::to_string(index++) };

				resource_aspect.addComponent<std::pair<size_t, Texture>>(comp_id, texture_descr);
			}

			/////////// Add renderstate
			rendering_aspect.addComponent<std::vector<mage::rendering::RenderState>>("renderStates", p_renderstates_list);

			/////////// Draw triangles
			rendering::DrawingControl drawingControl;
			rendering_aspect.addComponent<mage::rendering::DrawingControl>("drawingControl", drawingControl);

			/////////// Rendering Order
			rendering_aspect.addComponent<int>("renderingOrder", p_rendering_order);

			return entity;
		}

	}
}

