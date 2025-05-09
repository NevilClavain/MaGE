
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

#pragma warning( disable : 4005 4838 )

#include <utility>

#include "d3d11system.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"

#include "aspects.h"
#include "entity.h"
#include "entitygraph.h"

#include "exceptions.h"
#include "d3d11systemimpl.h"
#include "renderingqueue.h"

#include "ecshelpers.h"
#include "shader.h"

#include "logger_service.h"

#include "linemeshe.h"
#include "trianglemeshe.h"

#include "texture.h"

#include "datacloud.h"

#include "worldposition.h"


using namespace mage;
using namespace mage::core;

static const auto d3dimpl{ D3D11SystemImpl::getInstance() };

D3D11System::D3D11System(Entitygraph& p_entitygraph) : System(p_entitygraph)
{
	m_shadercompilation_invocation_cb = [&, this](const std::string& p_includePath,
		const mage::core::FileContent<const char>& p_src,		
		int p_shaderType,
		std::unique_ptr<char[]>& p_shaderBytes,
		size_t& p_shaderBytesLength,
		bool& p_status)
	{
		p_status = d3dimpl->createShaderBytesOnFile(p_shaderType, p_includePath, p_src, p_shaderBytes, p_shaderBytesLength);
	};

	m_texturecontentcopy_invocation_cb = [&, this](const std::string& p_textureId, void** p_data, size_t* p_dataSize)
	{
		d3dimpl->copyTextureContent(p_textureId, p_data, p_dataSize);
	};

	////// Register callback to runner
	
	const Runner::Callback runner_cb
	{
		[&, this](mage::core::RunnerEvent p_event, const std::string& p_target_descr, const std::string& p_action_descr)
		{		
			if (mage::core::RunnerEvent::TASK_ERROR == p_event)
			{
				// rethrow in current thread
				_EXCEPTION(std::string("failed action ") + p_action_descr + " on target " + p_target_descr);
			}
			else if (mage::core::RunnerEvent::TASK_DONE == p_event)
			{
				auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

				_MAGE_DEBUG(d3dimpl->logger(), std::string("TASK_DONE ") + p_target_descr + " " + p_action_descr);

				if ("load_shader_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_SHADER_CREATION_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_SHADER_CREATION_SUCCESS, p_target_descr);
					}
				}
				else if ("release_shader_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_SHADER_RELEASE_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_SHADER_RELEASE_SUCCESS, p_target_descr);
					}
				}
				else if ("load_linemeshe_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_LINEMESHE_CREATION_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_LINEMESHE_CREATION_SUCCESS, p_target_descr);
					}
				}
				else if ("release_linemeshe_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_LINEMESHE_RELEASE_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_LINEMESHE_RELEASE_SUCCESS, p_target_descr);
					}
				}
				else if ("load_trianglemeshe_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TRIANGLEMESHE_CREATION_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_TRIANGLEMESHE_CREATION_SUCCESS, p_target_descr);
					}
				}
				else if ("release_trianglemeshe_d3d11" == p_action_descr)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TRIANGLEMESHE_RELEASE_SUCCESS : " + p_target_descr);
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_TRIANGLEMESHE_RELEASE_SUCCESS, p_target_descr);
					}
				}
			}
		}
	};
	
	m_runner.registerSubscriber(runner_cb);
	m_runner.startup();

	////// Register callback to entitygraph

	const Entitygraph::Callback eg_cb
	{
		[&, this](mage::core::EntitygraphEvents p_event, const core::Entity& p_entity)
		{
			auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

			if (mage::core::EntitygraphEvents::ENTITYGRAPHNODE_REMOVED == p_event)
			{
				_MAGE_DEBUG(eventsLogger, "RECV EVENT -> ENTITYGRAPHNODE_REMOVED : " + p_entity.getId());

				/// no, DO NOT RELEASE RESOURCE IN D3D !!
				/*
				if (p_entity.hasAspect(core::resourcesAspect::id))
				{
					auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };
				
					const auto& resources{ p_entity.aspectAccess(core::resourcesAspect::id) };


					const auto s_list{ resources.getComponentsByType<Shader>() };
					for (auto& e : s_list)
					{
						auto& shader{ e->getPurpose() };

						const auto state{ shader.getState() };
						if (Shader::State::RENDERERLOADED == state)
						{
							_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_SHADER_RELEASE_BEGIN : " + shader.getName());
							for (const auto& call : m_callbacks)
							{
								call(D3D11SystemEvent::D3D11_SHADER_RELEASE_BEGIN, shader.getName());
							}
							handleShaderRelease(shader, shader.getType());
						}
					}

					//search for linemeshe
					const auto lm_list{ resources.getComponent<std::vector<LineMeshe>>("lineMeshes") };
					if (lm_list)
					{
						for (auto& lm : lm_list->getPurpose())
						{
							const auto state{ lm.getState() };
							if (LineMeshe::State::RENDERERLOADED == state)
							{
								_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_LINEMESHE_RELEASE_BEGIN : " + lm.getName());
								for (const auto& call : m_callbacks)
								{
									call(D3D11SystemEvent::D3D11_LINEMESHE_RELEASE_BEGIN, lm.getName());
								}

								handleLinemesheRelease(lm);
							}
						}
					}
					//search for trianglemeshe
					const auto tm_list{ resources.getComponent<std::vector<TriangleMeshe>>("triangleMeshes") };
					if (tm_list)
					{
						for (auto& tm : tm_list->getPurpose())
						{
							const auto state{ tm.getState() };
							if (TriangleMeshe::State::RENDERERLOADED == state)
							{
								_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TRIANGLEMESHE_RELEASE_BEGIN : " + tm.getName());
								for (const auto& call : m_callbacks)
								{
									call(D3D11SystemEvent::D3D11_TRIANGLEMESHE_RELEASE_BEGIN, tm.getName());
								}

								handleTrianglemesheRelease(tm);
							}
						}
					}
				}
				*/
			}
		}
	};

	p_entitygraph.registerSubscriber(eg_cb);
}

void D3D11System::manageInitialization()
{
	std::string rendering_target_entity_id;

	const auto forEachRenderingAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_rendering_components)
		{
			//////////////////////////////////////////////////////////////////////////////////////////////
			// manage D3D11 init

			const auto rendering_target_comp{ p_rendering_components.getComponent<core::renderingAspect::renderingTarget>("eg.std.renderingTarget") };
			const bool isWindowsRenderingTarget{ rendering_target_comp != nullptr &&
													core::renderingAspect::renderingTarget::SCREEN_RENDERINGTARGET == rendering_target_comp->getPurpose() };

			if (isWindowsRenderingTarget)
			{
				if (d3dimpl->init(p_entity))
				{
					m_initialized = true;
					rendering_target_entity_id = p_entity->getId();
				}
				else
				{
					_EXCEPTION("D3D11 initialization failed")
				}
			}
		}	
	};

	mage::helpers::extractAspectsTopDown<mage::core::renderingAspect>(m_entitygraph, forEachRenderingAspect);

	if (m_initialized)
	{
		for (const auto& call : m_callbacks)
		{
			call(D3D11SystemEvent::D3D11_WINDOW_READY, rendering_target_entity_id);
		}
	}
}

void D3D11System::handleRenderingQueuesState(Entity* p_entity, rendering::Queue& p_renderingQueue)
{
	switch (p_renderingQueue.getState())
	{
		case rendering::Queue::State::READY:

			// do queue rendering
			renderQueue(p_renderingQueue);
			break;

		default:
			// nothin' to do
			break;
	}
}

void D3D11System::manageRenderingQueue()
{
	const auto forEachRenderingAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_rendering_components)
		{
			const auto rendering_queues_list { p_rendering_components.getComponentsByType<rendering::Queue>() };
			if (rendering_queues_list.size() > 0)
			{
				auto& renderingQueue{ rendering_queues_list.at(0)->getPurpose() };

				this->handleRenderingQueuesState(p_entity, renderingQueue);

				renderingQueue.m_texts.clear();
			}
		}
	};
	mage::helpers::extractAspectsDownTop<mage::core::renderingAspect>(m_entitygraph, forEachRenderingAspect);
}

void D3D11System::manageResources()
{
	const auto forEachResourcesAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_resource_components)
		{
			auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

			{
				const auto shaders_list{ p_resource_components.getComponentsByType<std::pair<std::string,Shader>>() };

				for (auto& e : shaders_list)
				{
					auto& shader{ e->getPurpose().second };
					const auto state{ shader.getState() };
					if (Shader::State::BLOBLOADED == state)
					{

						_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_SHADER_CREATION_BEGIN : " + shader.m_source_id);
						for (const auto& call : m_callbacks)
						{
							call(D3D11SystemEvent::D3D11_SHADER_CREATION_BEGIN, shader.m_source_id);
						}

						handleShaderCreation(shader, shader.getType());
						shader.setState(Shader::State::RENDERERLOADING);
					}
				}
			}
			
			//search for line Meshes
			const auto lmeshes_list{ p_resource_components.getComponentsByType<LineMeshe>()};
			for (auto& e : lmeshes_list)
			{
				auto& lm{ e->getPurpose() };
				const auto state{ lm.getState()};
				if (LineMeshe::State::BLOBLOADED == state)
				{				
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_LINEMESHE_CREATION_BEGIN : " + lm.getSourceID());
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_LINEMESHE_CREATION_BEGIN, lm.getSourceID());
					}

					handleLinemesheCreation(lm);
					lm.setState(LineMeshe::State::RENDERERLOADING);
				}			
			}

			//search for plain triangle Meshes
			const auto tmeshes_list{ p_resource_components.getComponentsByType<TriangleMeshe>() };
			for (auto& e : tmeshes_list)
			{
				auto& tm{ e->getPurpose() };
				const auto state{ tm.getState() };

				if (TriangleMeshe::State::BLOBLOADED == state)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TRIANGLEMESHE_CREATION_BEGIN : " + tm.getSourceID());
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_TRIANGLEMESHE_CREATION_BEGIN, tm.getSourceID());
					}

					handleTrianglemesheCreation(tm);
					tm.setState(TriangleMeshe::State::RENDERERLOADING);
				}
			}

			//search for triangle Meshes from file			
			const auto filetmeshes_list{ p_resource_components.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>() };
			for (auto& e : filetmeshes_list)
			{
				auto& meshe_descr{ e->getPurpose() };

				TriangleMeshe& tm{ meshe_descr.second };
				const auto state{ tm.getState() };

				if (TriangleMeshe::State::BLOBLOADED == state)
				{
					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TRIANGLEMESHE_CREATION_BEGIN : " + tm.getSourceID());
					for (const auto& call : m_callbacks)
					{
						call(D3D11SystemEvent::D3D11_TRIANGLEMESHE_CREATION_BEGIN, tm.getSourceID());
					}

					handleTrianglemesheCreation(tm);
					tm.setState(TriangleMeshe::State::RENDERERLOADING);
				}
			}

			//search for render-target-textures
			{
				const auto textures_list{ p_resource_components.getComponentsByType<std::pair<size_t,Texture>>() };

				for (auto& e : textures_list)
				{
					auto& staged_texture{ e->getPurpose() };
					Texture& texture{ staged_texture.second };

					const auto state{ texture.getState() };
					if (Texture::State::BLOBLOADED == state)
					{
						_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TEXTURE_CREATION_BEGIN : " + texture.m_source_id);
						for (const auto& call : m_callbacks)
						{
							call(D3D11SystemEvent::D3D11_TEXTURE_CREATION_BEGIN, texture.m_source_id);
						}

						handleTextureCreation(texture);
						texture.setState(Texture::State::RENDERERLOADING);						
					}
				}
			}

			//search for textures-from-file
			{
				const auto textures_list{ p_resource_components.getComponentsByType<std::pair<size_t,std::pair<std::string, Texture>>>() };

				for (auto& e : textures_list)
				{
					auto& staged_texture{ e->getPurpose() };

					Texture& texture{ staged_texture.second.second };
					const auto path{ staged_texture.second.first };

					const auto state{ texture.getState() };
					if (Texture::State::BLOBLOADED == state)
					{
						_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> D3D11_TEXTURE_CREATION_BEGIN : " + texture.m_source_id);
						for (const auto& call : m_callbacks)
						{
							call(D3D11SystemEvent::D3D11_TEXTURE_CREATION_BEGIN, texture.m_source_id);
						}

						handleTextureCreation(texture);
						texture.setState(Texture::State::RENDERERLOADING);
					}
				}
			}
		}
	};
	mage::helpers::extractAspectsTopDown<mage::core::resourcesAspect>(m_entitygraph, forEachResourcesAspect);
}

void D3D11System::collectWorldTransformations() const
{
	const auto forEachRenderingAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_rendering_aspect)
		{
			auto& drawing_control_list { p_rendering_aspect.getComponentsByType<rendering::DrawingControl>() };
			if (drawing_control_list.size() > 0)
			{
				auto& drawing_control{ drawing_control_list.at(0)->getPurpose() };

				// search for a world aspect on the same entity
				if (!p_entity->hasAspect(core::worldAspect::id))
				{
					_EXCEPTION("missing entity world aspect : " + p_entity->getId());
				}

				const auto& world_aspect{ p_entity->aspectAccess(worldAspect::id) };
				const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

				if (0 == worldpositions_list.size())
				{					
					// try ptr version of the component
					const auto& worldpositions_ptr_list{ world_aspect.getComponentsByType<transform::WorldPosition*>() };
					
					if (0 == worldpositions_ptr_list.size())
					{
						_EXCEPTION("entity world aspect : missing world position " + p_entity->getId());
					}
					else
					{
						const transform::WorldPosition* entity_worldposition{ worldpositions_ptr_list.at(0)->getPurpose() };
						drawing_control.world = entity_worldposition->global_pos;
					}						
				}
				else
				{
					const transform::WorldPosition entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
					drawing_control.world = entity_worldposition.global_pos;
				}
			}
		}
	};

	mage::helpers::extractAspectsTopDown<mage::core::renderingAspect>(m_entitygraph, forEachRenderingAspect);
}

void D3D11System::renderQueue(const rendering::Queue& p_renderingQueue) const
{
	if (rendering::Queue::Purpose::UNDEFINED == p_renderingQueue.getPurpose())
	{
		return;
	}

	///////////////////////////////// queue main view ////////////////////////////////

	maths::Matrix current_mainview_cam;
	maths::Matrix current_mainview_proj;

	current_mainview_cam.identity();

	// set a dummy default perspective
	current_mainview_proj.perspective(1.0, 0.5, 1.0, 100000.0);

	//////////////////////////////// get view and proj matrix for this queue

	const std::string current_main_view_entity_id{ p_renderingQueue.getMainView()};
	if (current_main_view_entity_id != "")
	{
		auto& viewode{ m_entitygraph.node(current_main_view_entity_id) };
		const auto view_entity{ viewode.data() };

		// extract cam aspect
		const auto& cam_aspect{ view_entity->aspectAccess(cameraAspect::id) };
		const auto& cam_projs_list{ cam_aspect.getComponentsByType<maths::Matrix>() };

		if (0 == cam_projs_list.size())
		{
			_EXCEPTION("entity main view aspect : missing projection definition " + view_entity->getId());
		}
		else
		{
			current_mainview_proj = cam_projs_list.at(0)->getPurpose();
		}

		// extract world aspect

		const auto& world_aspect{ view_entity->aspectAccess(worldAspect::id) };
		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position " + view_entity->getId());
		}
		else
		{
			auto& entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
			current_mainview_cam = entity_worldposition.global_pos;
		}
	}

	maths::Matrix current_mainview_view = current_mainview_cam;
	current_mainview_view.inverse();

	///////////////////////////////// queue secondary view (useful for some effects that requires combination with a different point of view, like shadows map for example

	maths::Matrix current_secondaryview_cam;
	maths::Matrix current_secondaryview_proj;

	current_secondaryview_cam.identity();

	// set a dummy default perspective
	current_secondaryview_proj.perspective(1.0, 0.5, 1.0, 100000.0);


	const std::string current_secondary_view_entity_id{ p_renderingQueue.getSecondaryView()};
	if (current_secondary_view_entity_id != "")
	{
		auto& viewode{ m_entitygraph.node(current_secondary_view_entity_id) };
		const auto view_entity{ viewode.data() };

		// extract cam aspect
		const auto& cam_aspect{ view_entity->aspectAccess(cameraAspect::id) };
		const auto& cam_projs_list{ cam_aspect.getComponentsByType<maths::Matrix>() };

		if (0 == cam_projs_list.size())
		{
			_EXCEPTION("entity secondary view aspect : missing projection definition " + view_entity->getId());
		}
		else
		{
			current_secondaryview_proj = cam_projs_list.at(0)->getPurpose();
		}

		// extract world aspect

		const auto& world_aspect{ view_entity->aspectAccess(worldAspect::id) };
		const auto& worldpositions_list{ world_aspect.getComponentsByType<transform::WorldPosition>() };

		if (0 == worldpositions_list.size())
		{
			_EXCEPTION("entity world aspect : missing world position " + view_entity->getId());
		}
		else
		{
			auto& entity_worldposition{ worldpositions_list.at(0)->getPurpose() };
			current_secondaryview_cam = entity_worldposition.global_pos;
		}
	}

	maths::Matrix current_secondaryiew_view = current_secondaryview_cam;
	current_secondaryiew_view.inverse();

	////////////////////////////////////////////////////////////////////////

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

	if (rendering::Queue::Purpose::SCREEN_RENDERING == p_renderingQueue.getPurpose())
	{
		d3dimpl->beginScreen();
	}
	else //BUFFER_RENDERING
	{
		d3dimpl->beginTarget(p_renderingQueue.getTargetTextureUID());
	}

	if(p_renderingQueue.getTargetClearing())
	{ 
		d3dimpl->clearTarget(p_renderingQueue.getTargetClearColor());
	}

	if (p_renderingQueue.getTargetDepthClearing())
	{
		d3dimpl->clearTargetDepth();
	}
	
	{
		auto qnodes{ p_renderingQueue.getQueueNodes() };
		for (const auto& qnode : qnodes)
		{
			const rendering::Queue::RenderingOrderChannel rendering_channel{ qnode.second };

			for (const auto& vertexShaderInfo : rendering_channel.list)
			{
				const auto& vertexShaderId{ vertexShaderInfo.first };
				const auto& vertexShaderPayload{ vertexShaderInfo.second };

				//set vertex shader
				d3dimpl->setVertexShader(vertexShaderId);
				for (const auto& pixelShaderInfo : vertexShaderPayload.list)
				{
					const auto& pixelShaderId{ pixelShaderInfo.first };
					const auto& pixelShaderPayload{ pixelShaderInfo.second };

					//set pixel shader
					d3dimpl->setPixelShader(pixelShaderId);
					for (const auto& renderStatesInfo : pixelShaderPayload.list)
					{
						const auto renderStates{ renderStatesInfo.second.description };
						for (const auto& renderState : renderStates)
						{
							d3dimpl->setDepthStenciState(renderState);
							d3dimpl->setPSSamplers(renderState);
							d3dimpl->setVSSamplers(renderState);

							// prepare updates
							d3dimpl->prepareRenderState(renderState);
							d3dimpl->prepareBlendState(renderState);
						}

						// apply updates
						d3dimpl->setCacheRS();
						d3dimpl->setCacheBlendstate();

						///////////// TriangleMeshes BEGIN

						if (renderStatesInfo.second.trianglemeshes_list.size() > 0)
						{
							d3dimpl->setTriangleListTopology();
						}

						for (const auto& triangleMesheInfo : renderStatesInfo.second.trianglemeshes_list)
						{
							const auto& triangleMesheId{ triangleMesheInfo.first };
							d3dimpl->setTriangleMeshe(triangleMesheId);

							// nodes without textures
							{
								const auto& triangleQueueDrawingControls{ triangleMesheInfo.second.drawing_list };

								for (const auto& tdc : triangleQueueDrawingControls)
								{
									if (*tdc.second.draw)
									{
										//////
										const auto setup_func{ *tdc.second.setup };
										setup_func();

										////// Apply shaders params

										for (const auto& e : tdc.second.vshaders_map_cnx)
										{
											const auto& datacloud_data_id{ e.first };
											const auto& shader_param{ e.second };

											if ("Real4Vector" == shader_param.argument_type)
											{
												const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
												d3dimpl->setVertexshaderConstantsVec(shader_param.shader_register, rvector);
											}
										}

										for (const auto& e : tdc.second.pshaders_map_cnx)
										{
											const auto& datacloud_data_id{ e.first };
											const auto& shader_param{ e.second };

											if ("Real4Vector" == shader_param.argument_type)
											{
												const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
												d3dimpl->setPixelshaderConstantsVec(shader_param.shader_register, rvector);
											}
										}


										if (tdc.second.vshaders_vector_array)
										{
											for (int i = 0; i < tdc.second.vshaders_vector_array->size(); i++)
											{
												const mage::Shader::VectorArrayArgument& arg{ tdc.second.vshaders_vector_array->at(i) };
												int curr_register{ arg.start_shader_register };

												for (int j = 0; j < arg.array.size(); j++)
												{
													d3dimpl->setVertexshaderConstantsVec(curr_register, arg.array[j]);
													curr_register++;
												}
											}
										}

										if (tdc.second.pshaders_vector_array)
										{
											for (int i = 0; i < tdc.second.pshaders_vector_array->size(); i++)
											{
												const mage::Shader::VectorArrayArgument& arg{ tdc.second.pshaders_vector_array->at(i) };
												int curr_register{ arg.start_shader_register };

												for (int j = 0; j < arg.array.size(); j++)
												{
													d3dimpl->setPixelshaderConstantsVec(curr_register, arg.array[j]);
													curr_register++;
												}
											}
										}


										//////

										if (!(*tdc.second.projected_z_neg))
										{
											d3dimpl->drawTriangleMeshe(*tdc.second.world, current_mainview_view, current_mainview_proj, current_secondaryiew_view, current_secondaryview_proj);
										}

										//////

										const auto teardown_func{ *tdc.second.setup };
										teardown_func();
									}
								}
							}

							// nodes with textures
							const auto& textures_set_list{ triangleMesheInfo.second.textures_set_list };

							for (const auto& textures_set_entry : textures_set_list)
							{
								// Set textures stages

								const auto& textures_set{ textures_set_entry.second };
								for (int i = 0; i < mage::nbUVCoordsPerVertex; i++)
								{
									if (textures_set.textures.count(i))
									{
										// texture stage defined with an id
										const auto& texture_id{ textures_set.textures.at(i) };
										d3dimpl->bindTextureStage(texture_id, i);
									}
									else
									{
										d3dimpl->unbindTextureStage(i);
									}
								}

								/////

								const auto& triangleQueueDrawingControls{ textures_set_entry.second.drawing_list };

								for (const auto& tdc : triangleQueueDrawingControls)
								{
									if (*tdc.second.draw)
									{
										//////
										const auto setup_func{ *tdc.second.setup };
										setup_func();

										////// Apply shaders params

										for (const auto& e : tdc.second.vshaders_map_cnx)
										{
											const auto& datacloud_data_id{ e.first };
											const auto& shader_param{ e.second };

											if ("Real4Vector" == shader_param.argument_type)
											{
												const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
												d3dimpl->setVertexshaderConstantsVec(shader_param.shader_register, rvector);
											}
										}

										for (const auto& e : tdc.second.pshaders_map_cnx)
										{
											const auto& datacloud_data_id{ e.first };
											const auto& shader_param{ e.second };

											if ("Real4Vector" == shader_param.argument_type)
											{
												const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
												d3dimpl->setPixelshaderConstantsVec(shader_param.shader_register, rvector);
											}
										}


										if (tdc.second.vshaders_vector_array)
										{
											for (int i = 0; i < tdc.second.vshaders_vector_array->size(); i++)
											{
												const mage::Shader::VectorArrayArgument& arg{ tdc.second.vshaders_vector_array->at(i) };
												int curr_register{ arg.start_shader_register };

												for (int j = 0; j < arg.array.size(); j++)
												{
													d3dimpl->setVertexshaderConstantsVec(curr_register, arg.array[j]);
													curr_register++;
												}
											}
										}

										if (tdc.second.pshaders_vector_array)
										{
											for (int i = 0; i < tdc.second.pshaders_vector_array->size(); i++)
											{
												const mage::Shader::VectorArrayArgument& arg{ tdc.second.pshaders_vector_array->at(i) };
												int curr_register{ arg.start_shader_register };

												for (int j = 0; j < arg.array.size(); j++)
												{
													d3dimpl->setPixelshaderConstantsVec(curr_register, arg.array[j]);
													curr_register++;
												}
											}
										}


										//////

										if (!(*tdc.second.projected_z_neg))
										{
											d3dimpl->drawTriangleMeshe(*tdc.second.world, current_mainview_view, current_mainview_proj, current_secondaryiew_view, current_secondaryview_proj);
										}

										//////
										const auto teardown_func{ *tdc.second.setup };
										teardown_func();
									}
								}
							}
						}

						///////////// TriangleMeshes END

						///////////// LineMeshes BEGIN

						if (renderStatesInfo.second.linemeshes_list.size() > 0)
						{
							d3dimpl->setLineListTopology();
						}

						for (const auto& lineMesheInfo : renderStatesInfo.second.linemeshes_list)
						{
							const auto& lineMesheId{ lineMesheInfo.first };
							d3dimpl->setLineMeshe(lineMesheId);

							const auto& lineDrawingControls{ lineMesheInfo.second.drawing_list };
							for (const auto& ldc : lineDrawingControls)
							{
								if (*ldc.second.draw)
								{
									//////
									const auto setup_func{ *ldc.second.setup };
									setup_func();

									////// Apply shaders params

									for (const auto& e : ldc.second.vshaders_map_cnx)
									{
										const auto& datacloud_data_id{ e.first };
										const auto& shader_param{ e.second };

										if ("Real4Vector" == shader_param.argument_type)
										{
											const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
											d3dimpl->setVertexshaderConstantsVec(shader_param.shader_register, rvector);
										}
									}

									for (const auto& e : ldc.second.pshaders_map_cnx)
									{
										const auto& datacloud_data_id{ e.first };
										const auto& shader_param{ e.second };

										if ("Real4Vector" == shader_param.argument_type)
										{
											const maths::Real4Vector rvector{ { dataCloud->readDataValue<maths::Real4Vector>(datacloud_data_id) } };
											d3dimpl->setPixelshaderConstantsVec(shader_param.shader_register, rvector);
										}
									}

									//////
									d3dimpl->drawLineMeshe(*ldc.second.world, current_mainview_view, current_mainview_proj);

									//////
									const auto teardown_func{ *ldc.second.setup };
									teardown_func();
								}
							}
						}

						///////////// LineMeshes END
					}
				}

			}
		}
	}

	// render texts
	for (auto& text : p_renderingQueue.m_texts)
	{
		d3dimpl->drawText(text.font, text.color, text.position, text.rotation_rad, text.text);

		// after DrawString call, need to force blend state and renderstate restauration
		d3dimpl->setCacheRS(true);
		d3dimpl->setCacheBlendstate(true);

		d3dimpl->forceCurrentDepthStenciState();
		d3dimpl->forceCurrentPSSamplers();
		d3dimpl->forceCurrentVSSamplers();

		d3dimpl->forceTexturesBinding();

		d3dimpl->forceCurrentTopology();

		d3dimpl->forceCurrentPixelShader();
		d3dimpl->forceCurrentVertexShader();

		d3dimpl->forceCurrentMeshe();
	}
}

void D3D11System::run()
{
	if (!m_initialized)
	{
		manageInitialization();
	}

	manageResources();
	manageRenderingQueue();
	collectWorldTransformations();

	if (m_initialized)
	{
		d3dimpl->flipScreen();
	}

	m_runner.dispatchEvents();
}

void D3D11System::killRunner()
{
	mage::core::RunnerKiller runnerKiller;
	m_runner.m_mailbox_in.push(&runnerKiller);
	m_runner.join();
}

void D3D11System::handleShaderCreation(Shader& p_shaderInfos, int p_shaderType)
{
	const auto shaderType{ p_shaderType };

	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle shader creation ") + p_shaderInfos.m_source_id + std::string(" shader type ") + std::to_string(shaderType));

	const std::string shaderAction{ "load_shader_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(shaderAction, p_shaderInfos.m_source_id,
		[&,
			shaderType = shaderType,
			shaderAction = shaderAction
		]()
		{
			try
			{
				bool status { false };

				if (0 == shaderType)
				{
					status = d3dimpl->createVertexShader(p_shaderInfos.m_resource_uid, p_shaderInfos.getCode());
				}
				else if (1 == shaderType)
				{
					status = d3dimpl->createPixelShader(p_shaderInfos.m_resource_uid, p_shaderInfos.getCode());
				}

				if (!status)
				{
					_MAGE_ERROR(d3dimpl->logger(), "Failed to load shader " + p_shaderInfos.m_source_id + " in D3D11 ");

					// send error status to main thread and let terminate
					const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_shaderInfos.m_source_id, shaderAction };
					m_runner.m_mailbox_out.push(report);
				}
				else
				{
					_MAGE_DEBUG(d3dimpl->logger(), "Successful creation of shader " + p_shaderInfos.m_source_id + " in D3D11 ");
					p_shaderInfos.setState(Shader::State::RENDERERLOADED);
				}
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), "Failed to load shader " + p_shaderInfos.m_source_id + " in D3D11 : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_shaderInfos.m_source_id, shaderAction };
				m_runner.m_mailbox_out.push(report);
			}
		}
	)};

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleShaderRelease(Shader& p_shaderInfos, int p_shaderType)
{
	const auto shaderType{ p_shaderType };

	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle shader release ") + p_shaderInfos.m_source_id + std::string(" shader type ") + std::to_string(shaderType));

	const std::string shaderAction{ "release_shader_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(shaderAction, p_shaderInfos.m_source_id,
		[&,
			shaderType = shaderType,
			shaderAction = shaderAction
		]()
		{
			try
			{
				if (0 == shaderType)
				{
					d3dimpl->destroyVertexShader(p_shaderInfos.m_source_id);
				}
				else if (1 == shaderType)
				{
					d3dimpl->destroyPixelShader(p_shaderInfos.m_source_id);
				}

				_MAGE_DEBUG(d3dimpl->logger(), "Successful release of shader " + p_shaderInfos.m_source_id + " in D3D11 ");

			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), std::string("failed to release ") + p_shaderInfos.m_source_id + " : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_shaderInfos.m_source_id, shaderAction };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleLinemesheCreation(LineMeshe& p_lm)
{
	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle line meshe creation ") + p_lm.getSourceID());

	const std::string action{ "load_linemeshe_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(action, p_lm.getSourceID(),
		[&,
			action = action
		]()
		{
			try
			{
				bool status { false };
				status = d3dimpl->createLineMeshe(p_lm);

				if (!status)
				{
					_MAGE_ERROR(d3dimpl->logger(), "Failed to load linemeshe " + p_lm.getSourceID() + " in D3D11 ");

					// send error status to main thread and let terminate
					const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_lm.getSourceID(), action };
					m_runner.m_mailbox_out.push(report);
				}
				else
				{
					_MAGE_DEBUG(d3dimpl->logger(), "Successful creation of linemeshe " + p_lm.getSourceID() + " in D3D11 ");
					p_lm.setState(LineMeshe::State::RENDERERLOADED);
				}
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), "Failed to load linemeshe " + p_lm.getSourceID() + " in D3D11 : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_lm.getSourceID(), action };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleLinemesheRelease(LineMeshe& p_lm)
{
	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle line meshe release ") + p_lm.getSourceID());

	const std::string action{ "release_linemeshe_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(action, p_lm.getSourceID(),
		[&,
			action = action
		]()
		{
			try
			{
				d3dimpl->destroyLineMeshe(p_lm.getSourceID());
				_MAGE_DEBUG(d3dimpl->logger(), "Successful release of linemeshe " + p_lm.getSourceID() + " in D3D11 ");
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), std::string("failed to release ") + p_lm.getSourceID() + " : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_lm.getSourceID(), action };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleTrianglemesheCreation(TriangleMeshe& p_tm)
{
	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle triangle meshe creation ") + p_tm.getSourceID());

	const std::string action{ "load_trianglemeshe_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(action, p_tm.getSourceID(),
		[&,
			action = action
		]()
		{
			try
			{

				bool status { false };
				status = d3dimpl->createTriangleMeshe(p_tm);

				if (!status)
				{
					_MAGE_ERROR(d3dimpl->logger(), "Failed to load trianglemeshe " + p_tm.getSourceID() + " in D3D11 ");

					// send error status to main thread and let terminate
					const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_tm.getSourceID(), action };
					m_runner.m_mailbox_out.push(report);
				}
				else
				{
					_MAGE_DEBUG(d3dimpl->logger(), "Successful creation of trianglemeshe " + p_tm.getSourceID() + " in D3D11 ");
					p_tm.setState(TriangleMeshe::State::RENDERERLOADED);
				}
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), "Failed to load trianglemeshe " + p_tm.getSourceID() + " in D3D11 : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_tm.getSourceID(), action };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleTrianglemesheRelease(TriangleMeshe& p_tm)
{
	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle triangle meshe release ") + p_tm.getSourceID());

	const std::string action{ "release_trianglemeshe_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(action, p_tm.getSourceID(),
		[&,
			action = action
		]()
		{
			try
			{
				d3dimpl->destroyTriangleMeshe(p_tm.getSourceID());
				_MAGE_DEBUG(d3dimpl->logger(), "Successful release of trianglemeshe " + p_tm.getSourceID() + " in D3D11 ");
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), std::string("failed to release ") + p_tm.getSourceID() + " : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_tm.getSourceID(), action };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);
}

void D3D11System::handleTextureCreation(Texture& p_texture)
{
	_MAGE_DEBUG(d3dimpl->logger(), std::string("Handle texture creation ") + p_texture.m_source_id);

	const std::string action{ "load_texture_d3d11" };

	const auto task{ new mage::core::SimpleAsyncTask<>(action, p_texture.m_source_id,
		[&,
			action = action
		]()
		{
			try
			{
				bool status { false };
				status = d3dimpl->createTexture(p_texture);

				if (!status)
				{
					_MAGE_ERROR(d3dimpl->logger(), "Failed to load texture " + p_texture.m_source_id + " in D3D11 ");

					// send error status to main thread and let terminate
					const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_texture.m_source_id, action };
					m_runner.m_mailbox_out.push(report);
				}
				else
				{
					_MAGE_DEBUG(d3dimpl->logger(), "Successful creation of texture " + p_texture.m_source_id + " in D3D11 ");
					p_texture.setState(Texture::State::RENDERERLOADED);
				}
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(d3dimpl->logger(), "Failed to load texture " + p_texture.m_source_id + " in D3D11 : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, p_texture.m_source_id, action };
				m_runner.m_mailbox_out.push(report);
			}
		}
	) };

	m_runner.m_mailbox_in.push(task);

}