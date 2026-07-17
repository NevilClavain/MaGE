/* -*-LIC_BEGIN-*- */
/*
*
* MaGE rendering framework
* Emmanuel Chaumont Copyright (c) 2013-2026
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

#include<utility>
#include <chrono>
#include <string>
#include<map>
#include<vector>

#include "renderingqueuesystem.h"
#include "entity.h"
#include "entitygraph.h"
#include "aspects.h"
#include "ecshelpers.h"
#include "renderingqueue.h"
#include "shader.h"
#include "linemeshe.h"
#include "trianglemeshe.h"
#include "texture.h"
#include "exceptions.h"
#include "worldposition.h"
#include "datacloud.h"
#include "sysengine.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"
#include "scenestreamersystem.h"

#include "entitygraph_helpers.h"

using namespace mage;
using namespace mage::core;

RenderingQueueSystem::RenderingQueueSystem(Entitygraph& p_entitygraph, int p_streamersystem_slot) : System(p_entitygraph),
m_streamersystem_slot(p_streamersystem_slot),
m_localLogger("RenderingQueueSystem", mage::core::logger::Configuration::getInstance())
{
	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	dataCloud->registerData<std::string>("mage.timings.renderingqueuesystem");

}

void RenderingQueueSystem::run()
{
	const auto start_time{ std::chrono::high_resolution_clock::now() };

	std::call_once(m_initialization_once_flag, [this]()
	{
		if (!m_scan_entitygraph )
		{
			auto streamerSystemInstance{ dynamic_cast<mage::SceneStreamerSystem*>(SystemEngine::getInstance()->getSystem(m_streamersystem_slot)) };

			mage::SceneStreamerSystem::Callback streamer_system_cb
			{
				[&, this](mage::SceneStreamerSystemEvent p_event, const std::string& p_entity_id)
				{
					if (mage::SceneStreamerSystemEvent::REGISTER_RENDERING_PROXY == p_event)
					{
						// for rendering entities from entities controlled by scene streamer (scene rendering queues)

						Entity* entity{ m_entitygraph.node(p_entity_id).data() };

						const auto& rendering_aspect{ entity->aspectAccess(mage::core::renderingAspect::id) };
						rendering::Queue* current_queue{ helpers::findRenderingQueueInAncestors(entity) };

						if (current_queue)
						{
							if (entity->hasAspect(mage::core::resourcesAspect::id))
							{
								const auto& resource_aspect{ entity->aspectAccess(mage::core::resourcesAspect::id) };
								checkEntityInsertion(entity, resource_aspect, rendering_aspect, *current_queue);
							}

							// search for text rendering in rendering aspect

							const auto texts{ rendering_aspect.getComponentsByType<rendering::Queue::Text>() };
							if (texts.size() > 0)
							{
								auto& text{ texts.at(0)->getPurpose() };

								bool projected_z_neg{ false };

								if (entity->hasAspect(mage::core::worldAspect::id))
								{
									const auto& world_aspect{ entity->aspectAccess(mage::core::worldAspect::id) };
									const auto wp{ world_aspect.getComponentsByType<mage::transform::WorldPosition>().at(0)->getPurpose() };

									projected_z_neg = wp.projected_z_neg;

									const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
									const auto viewport{ dataCloud->readDataValue<maths::FloatCoords2D>("mage.infos.viewport") };
									const auto window_dims{ dataCloud->readDataValue<mage::core::maths::IntCoords2D>("mage.infos.window_resol") };

									text.position[0] = ((wp.global_pos(3, 0) + (viewport[0] * 0.5f)) * window_dims[0]) / viewport[0];
									text.position[1] = (((viewport[1] * 0.5f) - wp.global_pos(3, 1)) * window_dims[1]) / viewport[1];
								}
								if (!projected_z_neg)
								{
									current_queue->pushText(text);
								}
							}
						}

					}
					else if (mage::SceneStreamerSystemEvent::UNREGISTER_RENDERING_PROXY == p_event)
					{
						Entity* current_entity{ m_entitygraph.node(p_entity_id).data() };
						rendering::Queue* current_queue{ helpers::findRenderingQueueInAncestors(current_entity) };

						removeFromRenderingQueue(p_entity_id, *current_queue);
					}
				}
			};

			streamerSystemInstance->registerSubscriber(streamer_system_cb);
		}
	});

	manageRenderingQueue();

	const auto end_time{ std::chrono::high_resolution_clock::now() };
	const auto duration{ std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time) };
	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
	dataCloud->updateDataValue<std::string>("mage.timings.renderingqueuesystem", std::to_string(duration.count()) + " ms");
}

void RenderingQueueSystem::requestRenderingqueueLogging(const std::string& p_entityid)
{
	m_queuesToLog.emplace(p_entityid);
}

void RenderingQueueSystem::logRenderingqueue(const std::string& p_entity_id, mage::rendering::Queue& p_renderingQueue) const
{
	_MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> QUEUE DUMP BEGIN <<<<<<<<<<<<<<<<<<<<<<<<")
	_MAGE_DEBUG(m_localLogger, "for entity : " + p_entity_id);

	_MAGE_DEBUG(m_localLogger, "name : " + p_renderingQueue.getName())

	const std::map<rendering::Queue::Purpose, std::string> purpose_translate
	{
		{ rendering::Queue::Purpose::UNDEFINED, "UNDEFINED" },
		{ rendering::Queue::Purpose::SCREEN_RENDERING, "SCREEN_RENDERING" },
		{ rendering::Queue::Purpose::BUFFER_RENDERING, "BUFFER_RENDERING" },
	};
	_MAGE_DEBUG(m_localLogger, "purpose : " + purpose_translate.at(p_renderingQueue.getPurpose()))

	const std::map<rendering::Queue::State, std::string> state_translate
	{
		{ rendering::Queue::State::WAIT_INIT, "WAIT_INIT" },
		{ rendering::Queue::State::READY, "READY" }/*,
		{ rendering::Queue::State::ERROR_ORPHAN, "ERROR_ORPHAN" },*/
	};
	_MAGE_DEBUG(m_localLogger, "state : " + state_translate.at(p_renderingQueue.getState()))

	_MAGE_DEBUG(m_localLogger, "clear_target : " + std::to_string(p_renderingQueue.getTargetClearing()))

	if (p_renderingQueue.getTargetClearing())
	{
		const auto clear_color{ p_renderingQueue.getTargetClearColor() };
		_MAGE_DEBUG(m_localLogger, "clear_target_color : " + std::to_string(clear_color.r())
														+ " " + std::to_string(clear_color.g()) 
														+ " " + std::to_string(clear_color.b()) 
														+ " " + std::to_string(clear_color.a()))
	}

	// queue node dump
	const auto qnodes{ p_renderingQueue.getQueueNodes() };

	if (!qnodes.size())
	{
		_MAGE_DEBUG(m_localLogger, "Empty queue")
	}
	else
	{
		for (const auto& qnode : qnodes)
		{
			const int rendering_order{ qnode.first };

			_MAGE_DEBUG(m_localLogger, "\t-> RENDERING ORDER CHANNEL: [" + std::to_string(rendering_order) + "]");

			const rendering::Queue::RenderingOrderChannel rendering_channel{ qnode.second };

			//for (const auto& vshader : rendering_channel.list)

			for(const auto& shaders : rendering_channel.list)
			{
				const mage::rendering::Queue::ShadersPayload shader_payload{ shaders.second };
				const std::vector<std::string>& shaders_id{ shader_payload.shaders_ids };

				_MAGE_DEBUG(m_localLogger, "\t\t-> shader D3D resource id: " + shaders_id.at(0) + " " + shaders_id.at(1));

				for (const auto& rs : shader_payload.list)
				{
					_MAGE_DEBUG(m_localLogger, "\t\t\t-> renderstate : " + rs.first);

					if (rs.second.triangles_dc_list.size() > 0)
					{
						_MAGE_DEBUG(m_localLogger, "\t\t\t\t-> number of triangles_dc : " + std::to_string(rs.second.triangles_dc_list.size()));
					}

					for (const auto& triangles_dc : rs.second.triangles_dc_list)
					{
						_MAGE_DEBUG(m_localLogger, "\t\t\t\t-> triangles_dc : " + triangles_dc.first + " worlds stack size = " + std::to_string(triangles_dc.second.worlds.size()) );
					}

					for (const auto& lines_dc : rs.second.lines_dc_list)
					{
						_MAGE_DEBUG(m_localLogger, "\t\t\t\t-> lines_dc : " + lines_dc.first + " worlds stack size = " + std::to_string(lines_dc.second.worlds.size()));						
					}
				}
			}
		}
	}

	_MAGE_DEBUG(m_localLogger, ">>>>>>>>>>>>>>> QUEUE DUMP END <<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void RenderingQueueSystem::manageRenderingQueue()
{	
	auto entities_with_rendering{ m_entitygraph.getEntitiesListForAspect(core::renderingAspect::id) };
	for (Entity* entity : entities_with_rendering)
	{
		const auto currEntityId{ entity->getId() };
		
		const auto& rendering_aspect{ entity->aspectAccess(mage::core::renderingAspect::id) };

		const auto rendering_queues_list{ rendering_aspect.getComponentsByType<rendering::Queue>() };
		if (rendering_queues_list.size() > 0)
		{				
			auto& renderingQueue{ rendering_queues_list.at(0)->getPurpose() };

			////////Manage Queues states//////////////////////////////////////

			handleRenderingQueuesState(entity, renderingQueue);

			////////Manage Queues main and secondary views//////////////////////////////////////

			for (const auto& vp : m_cameraViewGroups)
			{
				for (const std::string& queue_id : vp.second.queues_id_list)
				{
					if (queue_id == currEntityId)
					{
						if (vp.second.main_view != renderingQueue.getMainView())
						{
							renderingQueue.setMainView(vp.second.main_view);
							for (const auto& call : m_callbacks)
							{
								call(RenderingQueueSystemEvent::MAINVIEW_QUEUE_UPDATED, queue_id + ", " + vp.second.main_view, renderingQueue);
							}
						}
						if (vp.second.secondary_view != renderingQueue.getSecondaryView())
						{
							renderingQueue.setSecondaryView(vp.second.secondary_view);
							for (const auto& call : m_callbacks)
							{
								call(RenderingQueueSystemEvent::SECONDARYVIEW_QUEUE_UPDATED, queue_id + ", " + vp.second.secondary_view, renderingQueue);
							}
						}
					}
				}
			}

			////////Manage Queues log//////////////////////////////////////

			if (m_queuesToLog.count(currEntityId))
			{
				logRenderingqueue(currEntityId, renderingQueue);
				m_queuesToLog.erase(currEntityId);
			}

		}

		{
			////////Manage Queues build/updates//////////////////////////////////////

			// for rendering entities that depends on rendering quads compositions queues (entities not controlled by scene streamer system

			rendering::Queue* current_queue{ helpers::findRenderingQueueInAncestors(entity) };

			if (current_queue)
			{
				if (entity->hasAspect(mage::core::resourcesAspect::id))
				{
					const auto& resource_aspect{ entity->aspectAccess(mage::core::resourcesAspect::id) };
					checkEntityInsertion(entity, resource_aspect, rendering_aspect, *current_queue);
				}

				// search for text rendering in rendering aspect

				const auto texts{ rendering_aspect.getComponentsByType<rendering::Queue::Text>() };
				if (texts.size() > 0)
				{
					auto& text{ texts.at(0)->getPurpose() };

					bool projected_z_neg{ false };

					if (entity->hasAspect(mage::core::worldAspect::id))
					{
						const auto& world_aspect{ entity->aspectAccess(mage::core::worldAspect::id) };
						const auto wp{ world_aspect.getComponentsByType<mage::transform::WorldPosition>().at(0)->getPurpose() };

						projected_z_neg = wp.projected_z_neg;

						const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
						const auto viewport{ dataCloud->readDataValue<maths::FloatCoords2D>("mage.infos.viewport") };
						const auto window_dims{ dataCloud->readDataValue<mage::core::maths::IntCoords2D>("mage.infos.window_resol") };

						text.position[0] = ((wp.global_pos(3, 0) + (viewport[0] * 0.5f)) * window_dims[0]) / viewport[0];
						text.position[1] = (((viewport[1] * 0.5f) - wp.global_pos(3, 1)) * window_dims[1]) / viewport[1];
					}
					if (!projected_z_neg)
					{
						current_queue->pushText(text);
					}
				}
			}
		}
	}
}

void RenderingQueueSystem::handleRenderingQueuesState(Entity* p_entity, rendering::Queue& p_renderingQueue)
{
	switch (p_renderingQueue.getState())
	{
		case rendering::Queue::State::WAIT_INIT:
		{
			const auto purpose{ p_renderingQueue.getPurpose() };

			if (rendering::Queue::Purpose::UNDEFINED == purpose)
			{
				const auto parent_entity{ p_entity->getParent() };

				if (nullptr == parent_entity)
				{
					//p_renderingQueue.setState(rendering::Queue::State::ERROR_ORPHAN);
					_EXCEPTION("Rendering queue set to ERROR_ORPHAN : no parent");
					// log it (WARN)
					//_MAGE_WARN(m_localLogger, "Rendering queue set to ERROR_ORPHAN : no parent")
				}
				else
				{
					if (parent_entity->hasAspect(core::renderingAspect::id))
					{
						const auto& parent_rendering_aspect{ parent_entity->aspectAccess(core::renderingAspect::id) };
						auto parent_rendering_target_comp{ parent_rendering_aspect.getComponent<core::renderingAspect::renderingTarget>("eg.std.renderingTarget") };
						if (parent_rendering_target_comp)
						{
							if (rendering::Queue::State::WAIT_INIT == p_renderingQueue.getState())
							{
								if (core::renderingAspect::renderingTarget::SCREEN_RENDERINGTARGET == parent_rendering_target_comp->getPurpose())
								{
									// SCREEN_RENDERINGTARGET
									// 
									// parent is a screen-target pass 
									// set queue purpose accordingly

									p_renderingQueue.setScreenRenderingPurpose();
									_MAGE_DEBUG(m_localLogger, "rendering queue " + p_renderingQueue.getName() + " set to READY, SCREEN_RENDERING")
								}
								else
								{
									// BUFFER_RENDERINGTARGET
									// 
									// parent is a texture-target pass
									// set queue purpose accordingly

									// parent is a texture-target pass
									// search for a target texture in it

									// search in resource aspect

									const auto& parent_resource_aspect{ parent_entity->aspectAccess(core::resourcesAspect::id) };
									const ComponentList<std::pair<size_t, mage::Texture>> textures_list{ parent_resource_aspect.getComponentsByType<std::pair<size_t,mage::Texture>>() };

									const auto queue_target_stage{ p_renderingQueue.getTargetStage()};

									if (queue_target_stage < textures_list.size())
									{
										p_renderingQueue.setBufferRenderingPurpose(textures_list.at(queue_target_stage)->getPurpose().second);
									}
									else
									{
										_EXCEPTION("Missing rendertarget texture on requested stage for BUFFER_RENDERING queue : " + p_renderingQueue.getName());
									}

									_MAGE_DEBUG(m_localLogger, "rendering queue " + p_renderingQueue.getName() + " set to READY, BUFFER_RENDERING")
								}

								p_renderingQueue.setState(rendering::Queue::State::READY);

								for (const auto& call : m_callbacks)
								{
									const std::string queue_name{ p_renderingQueue.getName() };
									call(RenderingQueueSystemEvent::RENDERINGQUEUE_STATE_READY, queue_name, p_renderingQueue);
								}
							}
						}
						else
						{
							// parent rendering aspect has no renderingTarget component !
							//p_renderingQueue.setState(rendering::Queue::State::ERROR_ORPHAN);

							_EXCEPTION("Rendering queue set to ERROR_ORPHAN : parent rendering aspect has no renderingTarget component : " + p_renderingQueue.getName() + ", parent is " + parent_entity->getId());
							// log it (WARN)
							//_MAGE_WARN(m_localLogger, "Rendering queue set to ERROR_ORPHAN : parent rendering aspect has no renderingTarget component")							
						}
					}
					else
					{
						// parent has no rendering aspect 
						//p_renderingQueue.setState(rendering::Queue::State::ERROR_ORPHAN);

						_EXCEPTION("Rendering queue set to ERROR_ORPHAN : parent has no rendering aspect : " + p_renderingQueue.getName() + ", parent is " + parent_entity->getId());

						// log it (WARN)
						//_MAGE_WARN(m_localLogger, "Rendering queue set to ERROR_ORPHAN : parent has no rendering aspect")
					}
				}
			}
		}
		break;
	}
}

static std::string build_rs_list_id(const std::vector<mage::rendering::RenderState>& p_rs_list)
{
	std::string rs_set_signature;
	for (const auto& e : p_rs_list)
	{
		rs_set_signature += e.toString() + "; ";
	}

	return rs_set_signature;
}

static void const connect_shaders_args(/*mage::core::logger::Sink& p_localLogger,*/
	const rendering::DrawingControl& p_drawingControl, 
	rendering::QueueDrawingControl& p_queueDrawingControl,
	const mage::Shader& p_vshader, const mage::Shader& p_pshader)
{
	///////////////////// connect vertex shader args
	const auto vshaders_current_args{ p_vshader.getGenericArguments() };

	//vshader arguments id match loop
	for (const auto& current_arg : vshaders_current_args)
	{
		const auto argument_id{ current_arg.argument_id };
		for (const auto& connection_pair : p_drawingControl.vshaders_map)
		{
			if (argument_id == connection_pair.second)
			{
				p_queueDrawingControl.vshaders_map_cnx.push_back(std::make_pair(connection_pair.first, current_arg));
			}
		}
	}

	///////////////////// connect pixel shader args
	const auto pshaders_current_args{ p_pshader.getGenericArguments() };

	//pshader arguments id match loop
	for (const auto& current_arg : pshaders_current_args)
	{
		const auto argument_id{ current_arg.argument_id };
		for (const auto& connection_pair : p_drawingControl.pshaders_map)
		{
			if (argument_id == connection_pair.second)
			{
				p_queueDrawingControl.pshaders_map_cnx.push_back(std::make_pair(connection_pair.first, current_arg));
			}
		}
	}
	///////////////////////////////////
}


void RenderingQueueSystem::checkEntityInsertion(mage::core::Entity* p_entity, 
												const mage::core::ComponentContainer& p_resourceAspect,
												const mage::core::ComponentContainer& p_renderingAspect, 
												mage::rendering::Queue& p_renderingQueue)
{	
	const auto drawingControls{ p_renderingAspect.getComponentsByType<rendering::DrawingControl>() };

	if (drawingControls.size() > 0)
	{
		bool notAllReady{ false };

		for (const auto& dc : drawingControls)
		{
			auto& drawingControl{ dc->getPurpose() };

			if (!drawingControl.ready)
			{
				notAllReady = true;
				drawingControl.owner_entity_id = p_entity->getId();
			}
		}

		if (notAllReady)
		{			
			auto queueNodes{ p_renderingQueue.getQueueNodes() };

			// search for lineMeshe
			LineMeshe* line_meshe_ref{ nullptr };
			{
				const auto lineMeshes{ p_resourceAspect.getComponentsByType<LineMeshe>() };

				if (lineMeshes.size() > 0)
				{
					line_meshe_ref = &lineMeshes.at(0)->getPurpose();
				}
				else
				{
					const auto lineMeshesRef{ p_resourceAspect.getComponentsByType<LineMeshe*>() };

					if (lineMeshesRef.size() > 0)
					{
						line_meshe_ref = lineMeshesRef.at(0)->getPurpose();
					}
				}
			}

			// search for plain triangleMeshe
			TriangleMeshe* triangle_meshe_ref{ nullptr };
			{
				const auto triangleMeshes{ p_resourceAspect.getComponentsByType<TriangleMeshe>() };

				if (triangleMeshes.size() > 0)
				{
					triangle_meshe_ref = &triangleMeshes.at(0)->getPurpose();
				}
				else
				{
					const auto triangleMeshesRef{ p_resourceAspect.getComponentsByType<TriangleMeshe*>() };

					if (triangleMeshesRef.size() > 0)
					{
						triangle_meshe_ref = triangleMeshesRef.at(0)->getPurpose();
					}
				}
			}

			// search for triangleMeshe loaded from files
			std::pair<std::pair<std::string, std::string>, TriangleMeshe>* file_triangle_meshe_ref{ nullptr };						
			{
				const auto fromFileTriangleMeshes{ p_resourceAspect.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>() };

				if (fromFileTriangleMeshes.size() > 0)
				{
					file_triangle_meshe_ref = &fromFileTriangleMeshes.at(0)->getPurpose();
				}
				else
				{
					const auto fromFileTriangleMeshesRef{ p_resourceAspect.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>*>() };

					if (fromFileTriangleMeshesRef.size() > 0)
					{
						file_triangle_meshe_ref = fromFileTriangleMeshesRef.at(0)->getPurpose();
					}
				}
			}
			
			
			// search rendering states
			const auto rsStates{ p_renderingAspect.getComponentsByType<std::vector<mage::rendering::RenderState>>() };

			// search for shaders
			const auto shaders{ p_resourceAspect.getComponentsByType<std::pair<std::string,Shader>>() };

			// search for textures set
			
			const auto renderingTexturesSet{ p_resourceAspect.getComponentsByType<std::pair<size_t,Texture>>() };
			const auto renderingTexturesSetPtr{ p_resourceAspect.getComponentsByType<std::pair<size_t,Texture>*>() };
			const auto texturesFromFileSet{ p_resourceAspect.getComponentsByType<std::pair<size_t,std::pair<std::string,Texture>>>() };
			
			ComponentList<std::pair<size_t, Texture>> texturesSet;
			ComponentContainer cc;

			if (renderingTexturesSet.size() > 0)
			{
				//texturesSet = renderingTexturesSet;

				int index = 0;
				for (auto& tc : renderingTexturesSet)
				{
					cc.addComponent<std::pair<size_t, Texture>>("texturesFromRendering" + std::to_string(index++), tc->getPurpose() );
				}
			}

			if(renderingTexturesSetPtr.size() > 0)
			{
				// uniformize texture set format : convert from std::pair<size_t,std::pair<std::string,Texture>> to std::pair<size_t,Texture>

				int index = 0;
				for (auto& tc : renderingTexturesSetPtr)
				{
					const std::pair<size_t, Texture>* t{ tc->getPurpose() };

					const size_t stage_copy{ t->first };
					Texture texture_copy{ t->second };

					cc.addComponent<std::pair<size_t, Texture>>("texturesFromPtr" + std::to_string(index++), std::make_pair(stage_copy, texture_copy));
				}
			}

			if (texturesFromFileSet.size() > 0)
			{
				// uniformize texture set format : convert from std::pair<size_t,std::pair<std::string,Texture>> to std::pair<size_t,Texture>

				int index = 0;
				for (auto& tc : texturesFromFileSet)
				{
					const auto t{ tc->getPurpose() };

					cc.addComponent<std::pair<size_t, Texture>>("texturesFromFileList" + std::to_string(index++), std::make_pair(t.first, t.second.second));
				}
			}


			texturesSet = cc.getComponentsByType<std::pair<size_t, Texture>>();
					
			// rendering order channel : 0 by default
			int rendering_channel{ 0 };

			const auto rocs{ p_renderingAspect.getComponentsByType<int>() };
			if (rocs.size() > 0)
			{
				rendering_channel = rocs.at(0)->getPurpose();
			}

			if (1 < shaders.size())
			{
				const auto& vshader{ shaders.at(vertexShader)->getPurpose().second };
				const auto& pshader{ shaders.at(pixelShader)->getPurpose().second };

				if (vertexShader == vshader.getType() && pixelShader == pshader.getType())
				{
					bool resources_D3D11ready{ true };

					//////////////////////////////// check shaders are D3D11 ready

					if (Shader::State::RENDERERLOADED != vshader.getState() || Shader::State::RENDERERLOADED != pshader.getState())
					{
						resources_D3D11ready = false;
					}

					//////////////////////////////// check meshes are D3D11 ready

					if (line_meshe_ref)
					{
						if (LineMeshe::State::RENDERERLOADED != line_meshe_ref->getState())
						{
							resources_D3D11ready = false;
						}
					}

					if(triangle_meshe_ref)
					{
						if (TriangleMeshe::State::RENDERERLOADED != triangle_meshe_ref->getState())
						{
							resources_D3D11ready = false;
						}
					}

					if (file_triangle_meshe_ref)
					{						
						TriangleMeshe& tm{ file_triangle_meshe_ref->second };
						const auto state{ tm.getState() };

						if (TriangleMeshe::State::RENDERERLOADED != state)
						{
							resources_D3D11ready = false;
						}
					}

					//////////////////////////////// check textures are D3D11 ready

					for (const auto& e : texturesSet)
					{
						const auto& staged_texture{ e->getPurpose() };

						if (Texture::State::RENDERERLOADED != staged_texture.second.getState())
						{
							resources_D3D11ready = false;
						}
					}

					if (resources_D3D11ready && rsStates.size() > 0 && (line_meshe_ref || triangle_meshe_ref || file_triangle_meshe_ref))
					{
						// ok, can update queue
						
						if (!queueNodes.count(rendering_channel)) 
						{
							rendering::Queue::RenderingOrderChannel renderingOrderChannel;
							queueNodes[rendering_channel] = renderingOrderChannel;
						}

						const std::string shaders_pair_id{ vshader.getResourceUID() + "//" + pshader.getResourceUID()};

						mage::rendering::Queue::ShadersPayload newShaderPayload;
						mage::rendering::Queue::ShadersPayload* shaderPayloadPtr{ nullptr };

						if (queueNodes.at(rendering_channel).list.count(shaders_pair_id))
						{
							shaderPayloadPtr = &queueNodes.at(rendering_channel).list.at(shaders_pair_id);
						}
						else
						{
							shaderPayloadPtr = &newShaderPayload;
						}

						const auto rs_list_id{ build_rs_list_id(rsStates.at(0)->getPurpose()) };

						mage::rendering::Queue::RenderStatePayload newRenderStatePayload;
						mage::rendering::Queue::RenderStatePayload* renderStatePayloadPtr{ nullptr };

						if (shaderPayloadPtr->list.count(rs_list_id))
						{
							renderStatePayloadPtr = &shaderPayloadPtr->list.at(rs_list_id);
						}
						else
						{
							renderStatePayloadPtr = &newRenderStatePayload;
						}

						if (line_meshe_ref)
						{	
							for (const auto& ldc : drawingControls)
							{
								auto& linesDrawingControl{ ldc->getPurpose() };

								linesDrawingControl.ready = true;

								rendering::QueueLinesDrawingControl linesQueueDrawingControl;

								/// common parts
										
								linesQueueDrawingControl.owner_entity_id = linesDrawingControl.owner_entity_id;

								pushWorldOutputToQueueDrawingControl(p_entity, linesQueueDrawingControl);

								connect_shaders_args(linesDrawingControl, linesQueueDrawingControl, vshader, pshader);

								/////////////// HERE manage vector array for shaders
								linesQueueDrawingControl.vshaders_vector_array = &vshader.getVectorArrayArguments();
								linesQueueDrawingControl.pshaders_vector_array = &pshader.getVectorArrayArguments();

								linesQueueDrawingControl.draw = &linesDrawingControl.draw;

								/// specific part

								linesQueueDrawingControl.meshe_id = line_meshe_ref->getResourceUID();

								/// register
										
								renderStatePayloadPtr->lines_dc_list[linesDrawingControl.owner_entity_id] = linesQueueDrawingControl;
							}
						}
						else if (triangle_meshe_ref)
						{																						
							std::unordered_map<size_t, std::string> textures;

							for (const auto& e : texturesSet)
							{
								const auto& staged_texture{ e->getPurpose() };

								const size_t stage{ staged_texture.first };
								const Texture& texture{ staged_texture.second };

								textures[stage] = texture.getResourceUID();
							}
									
							for (const auto& tdc : drawingControls)
							{
								auto& trianglesDrawingControl{ tdc->getPurpose() };

								trianglesDrawingControl.ready = true;

								rendering::QueueTrianglesDrawingControl trianglesQueueDrawingControl;

								/// common parts

								trianglesQueueDrawingControl.owner_entity_id = trianglesDrawingControl.owner_entity_id;

								pushWorldOutputToQueueDrawingControl(p_entity, trianglesQueueDrawingControl);
	

								trianglesQueueDrawingControl.projected_z_neg = &trianglesDrawingControl.projected_z_neg;

								connect_shaders_args(trianglesDrawingControl, trianglesQueueDrawingControl, vshader, pshader);

								/////////////// HERE manage vector array for shaders
								trianglesQueueDrawingControl.vshaders_vector_array = &vshader.getVectorArrayArguments();
								trianglesQueueDrawingControl.pshaders_vector_array = &pshader.getVectorArrayArguments();

								trianglesQueueDrawingControl.draw = &trianglesDrawingControl.draw;

								/// specific part

								trianglesQueueDrawingControl.meshe_id = triangle_meshe_ref->getResourceUID();
								trianglesQueueDrawingControl.textures = textures;

								/// register
								//
								renderStatePayloadPtr->triangles_dc_list[trianglesDrawingControl.owner_entity_id] = trianglesQueueDrawingControl;

							}
						}
						else if (file_triangle_meshe_ref)
						{							
							std::unordered_map<size_t, std::string> textures;

							for (const auto& e : texturesSet)
							{
								const auto& staged_texture{ e->getPurpose() };

								const size_t stage{ staged_texture.first };
								const Texture& texture{ staged_texture.second };

								textures[stage] = texture.getResourceUID();
							}

							for (const auto& tdc : drawingControls)
							{
								auto& trianglesDrawingControl{ tdc->getPurpose() };

								trianglesDrawingControl.ready = true;

								rendering::QueueTrianglesDrawingControl trianglesQueueDrawingControl;

								/// common parts

								trianglesQueueDrawingControl.owner_entity_id = trianglesDrawingControl.owner_entity_id;
				
								pushWorldOutputToQueueDrawingControl(p_entity, trianglesQueueDrawingControl);

								trianglesQueueDrawingControl.projected_z_neg = &trianglesDrawingControl.projected_z_neg;

								connect_shaders_args(trianglesDrawingControl, trianglesQueueDrawingControl, vshader, pshader);

								/////////////// HERE manage vector array for shaders
								trianglesQueueDrawingControl.vshaders_vector_array = &vshader.getVectorArrayArguments();
								trianglesQueueDrawingControl.pshaders_vector_array = &pshader.getVectorArrayArguments();

								trianglesQueueDrawingControl.draw = &trianglesDrawingControl.draw;

								/// specific part

								trianglesQueueDrawingControl.meshe_id = file_triangle_meshe_ref->second.getResourceUID();
								trianglesQueueDrawingControl.textures = textures;

								/// register
								
								if (0 == renderStatePayloadPtr->triangles_dc_list.size())
								{
									renderStatePayloadPtr->triangles_dc_list[trianglesDrawingControl.owner_entity_id] = trianglesQueueDrawingControl;
								}
								else
								{
									// POUR LA GESTION DU DRAWINDEXEDINSTANCED !!! -> push les matrices worlds sur le meme QueueDrawingControl !!!!

									bool found = false;
									std::string found_trianglesQueueDrawingControl_owner_entity_id;

									for (const auto& qtdc : renderStatePayloadPtr->triangles_dc_list)
									{										
										if (qtdc.second == trianglesQueueDrawingControl) // cf bool QueueTrianglesDrawingControl::operator==(const QueueTrianglesDrawingControl& p_other) const -> même meshe id et textures !!
										{
											found = true;
											found_trianglesQueueDrawingControl_owner_entity_id = qtdc.first;
											break;
										}
									}

									if (!found)
									{
										renderStatePayloadPtr->triangles_dc_list[trianglesDrawingControl.owner_entity_id] = trianglesQueueDrawingControl;
									}
									else
									{
										auto& qtdc = renderStatePayloadPtr->triangles_dc_list.at(found_trianglesQueueDrawingControl_owner_entity_id);

										pushWorldOutputToQueueDrawingControl(p_entity, qtdc);
									}
								}
							}
						}

						if (!shaderPayloadPtr->list.count(rs_list_id))
						{
							// new renderStatePayload
							newRenderStatePayload.description = rsStates.at(0)->getPurpose();
							shaderPayloadPtr->list[rs_list_id] = newRenderStatePayload;
						}

						if (!queueNodes.at(rendering_channel).list.count(shaders_pair_id))
						{
							// new shaderPayload

							shaderPayloadPtr->shaders_ids.push_back(vshader.getResourceUID());
							shaderPayloadPtr->shaders_ids.push_back(pshader.getResourceUID());

							queueNodes.at(rendering_channel).list[vshader.getResourceUID() + "//" + pshader.getResourceUID()] = newShaderPayload;
						}
					}
				}
			}
			p_renderingQueue.setQueueNodes(queueNodes);
		}
	}	
}


void RenderingQueueSystem::removeFromRenderingQueue(const std::string& p_entity_id, mage::rendering::Queue& p_renderingQueue)
{
	auto queueNodes{ p_renderingQueue.getQueueNodes() };

	std::vector<int> roc_to_remove;

	for (auto& qnode : queueNodes)
	{
		rendering::Queue::RenderingOrderChannel& rendering_channel{ qnode.second };

		std::vector<std::string> shaders_pair_to_remove;
		for (auto& shaders : rendering_channel.list)
		{
			std::vector<std::string> rs_to_remove;

			for (auto& rs : shaders.second.list)
			{

				//// line meshes
				std::vector<std::string> ldc_to_remove;
		
				for (auto& ldc : rs.second.lines_dc_list)
				{
					if (ldc.second.owner_entity_id == p_entity_id)
					{
						ldc_to_remove.push_back(p_entity_id);
					}
				}

				for (const std::string& id : ldc_to_remove)
				{
					rs.second.lines_dc_list.erase(id);
				}


				//// triangle meshes
				std::vector<std::string> tdc_to_remove;
				
				for (auto& tdc : rs.second.triangles_dc_list)
				{
					if (tdc.second.owner_entity_id == p_entity_id)
					{						
						// remove this triangle dc
						tdc_to_remove.push_back(p_entity_id);
					}
				}

				for (const std::string& id : tdc_to_remove)
				{
					rs.second.triangles_dc_list.erase(id);
				}

				////////////////////

				if (0 == rs.second.lines_dc_list.size() && 0 == rs.second.triangles_dc_list.size())
				{

					rs_to_remove.push_back(rs.first);
				}				
			}

			for (const std::string& id : rs_to_remove)
			{
				shaders.second.list.erase(id);
			}

			if (0 == shaders.second.list.size())
			{
				
				shaders_pair_to_remove.push_back(shaders.first);
			}

		}

		for (const std::string& id : shaders_pair_to_remove)
		{
			rendering_channel.list.erase(id);
		}

		////////////////////////////////////////////////

		if (0 == rendering_channel.list.size())
		{			
			roc_to_remove.push_back(qnode.first);
		}
	}

	for (const int chan : roc_to_remove)
	{
		queueNodes.erase(chan);
	}

	p_renderingQueue.setQueueNodes(queueNodes);
}

void RenderingQueueSystem::createViewGroup(const std::string& p_viewGroupId)
{
	if (!m_cameraViewGroups.count(p_viewGroupId))
	{
		m_cameraViewGroups[p_viewGroupId] = {};
	}
	else
	{
		_EXCEPTION("ViewGroupId already exists ! -> " + p_viewGroupId);
	}
}

void RenderingQueueSystem::setViewGroupMainView(const std::string& p_viewGroupId, const std::string& p_mainview)
{
	if (m_cameraViewGroups.count(p_viewGroupId))
	{
		m_cameraViewGroups.at(p_viewGroupId).main_view = p_mainview;
	}
	else
	{
		_EXCEPTION("Unknow viewGroupId : " + p_viewGroupId);
	}
}

void RenderingQueueSystem::setViewGroupSecondaryView(const std::string& p_viewGroupId, const std::string& p_secondaryview)
{
	if (m_cameraViewGroups.count(p_viewGroupId))
	{
		m_cameraViewGroups.at(p_viewGroupId).secondary_view = p_secondaryview;
	}
	else
	{
		_EXCEPTION("Unknow viewGroupId : " + p_viewGroupId);
	}
}

std::pair<std::string, std::string> RenderingQueueSystem::getViewGroupCurrentViews(const std::string& p_viewGroupId) const
{
	if (m_cameraViewGroups.count(p_viewGroupId))
	{
		return { m_cameraViewGroups.at(p_viewGroupId).main_view, m_cameraViewGroups.at(p_viewGroupId).secondary_view };
	}
	else
	{
		_EXCEPTION("Unknow viewGroupId : " + p_viewGroupId);
	}
}

void RenderingQueueSystem::addQueuesToViewGroup(const std::string& p_viewGroupId, const std::unordered_set<std::string>& p_queues_id_list)
{
	if (m_cameraViewGroups.count(p_viewGroupId))
	{		
		for (const std::string& queue_id : p_queues_id_list)
		{
			m_cameraViewGroups.at(p_viewGroupId).queues_id_list.insert(queue_id);
		}
	}
	else
	{
		_EXCEPTION("Unknow viewGroupId : " + p_viewGroupId);
	}
}

void RenderingQueueSystem::pushWorldOutputToQueueDrawingControl(mage::core::Entity* p_entity, rendering::QueueDrawingControl& p_outqtdc)
{
	auto& world_aspect{ p_entity->aspectAccess(mage::core::worldAspect::id) };

	// search if world aspect is delegated to a separated scene entity, represented by a Entity* component in this local entity world aspect

	const auto& scene_entity_list{ world_aspect.getComponentsByType<Entity*>() };
	if (scene_entity_list.size() > 0)
	{
		// separated entity for scene -> connect to world global output of this scene entity

		const Entity* scene_entity{ scene_entity_list.at(0)->getPurpose() };
		const auto& scene_entity_world_aspect{ scene_entity->aspectAccess(worldAspect::id) };
		const auto& scene_entity_worldpositions_list{ scene_entity_world_aspect.getComponentsByType<transform::WorldPosition>() };
		if (0 == scene_entity_worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position on entity " + scene_entity->getId());
		}
		const transform::WorldPosition& scene_entity_worldposition{ scene_entity_worldpositions_list.at(0)->getPurpose() };

		p_outqtdc.worlds.push_back(&scene_entity_worldposition.global_pos);

	}
	else
	{
		// no separated scene entity

		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };
		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position on entity " + p_entity->getId());
		}
		const transform::WorldPosition& worldposition{ worldpositions_list.at(0)->getPurpose() };

		p_outqtdc.worlds.push_back(&worldposition.global_pos);
	}
}

void RenderingQueueSystem::enableRendergraphScan(bool p_enable)
{
	m_scan_entitygraph = p_enable;
}