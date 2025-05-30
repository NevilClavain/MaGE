
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

#include "samplesbase.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <list>

#include <chrono>

#include "aspects.h"

#include "d3d11system.h"
#include "timesystem.h"
#include "resourcesystem.h"
#include "worldsystem.h"
#include "dataprintsystem.h"
#include "renderingqueuesystem.h"
#include "animationssystem.h"

#include "sysengine.h"
#include "filesystem.h"
#include "logconf.h"

#include "logger_service.h"

#include "worldposition.h"
#include "animatorfunc.h"

#include "trianglemeshe.h"
#include "renderstate.h"

#include "animationbone.h"
#include "scenenode.h"


#include "syncvariable.h"
#include "animators_helpers.h"

#include "datacloud.h"

#include "shaders_service.h"
#include "textures_service.h"

#include "entitygraph_helpers.h"


using namespace mage;
using namespace mage::core;
using namespace mage::rendering;

void SamplesBase::init(const std::string p_appWindowsEntityName)
{
	/////////// systems

	auto sysEngine{ SystemEngine::getInstance() };

	sysEngine->makeSystem<mage::TimeSystem>(timeSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::D3D11System>(d3d11SystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::ResourceSystem>(resourceSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::WorldSystem>(worldSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::RenderingQueueSystem>(renderingQueueSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::DataPrintSystem>(dataPrintSystemSlot, m_entitygraph);
	sysEngine->makeSystem<mage::AnimationsSystem>(animationsSystemSlot, m_entitygraph);

	// D3D11 system provides compilation shader service : give access to this to resources sytem
	const auto d3d11System{ sysEngine->getSystem<mage::D3D11System>(d3d11SystemSlot) };
	services::ShadersCompilationService::getInstance()->registerSubscriber(d3d11System->getShaderCompilationInvocationCallback());
	services::TextureContentCopyService::getInstance()->registerSubscriber(d3d11System->getTextureContentCopyInvocationCallback());


	///////////////////////////

	d3d11_system_events_base();

	m_appWindowsEntityName = p_appWindowsEntityName;
}


void SamplesBase::d3d11_system_events_base()
{
	const auto sysEngine{ SystemEngine::getInstance() };
	const auto d3d11System{ sysEngine->getSystem<mage::D3D11System>(d3d11SystemSlot) };

	const D3D11System::Callback d3d11_cb
	{
		[&, this](D3D11SystemEvent p_event, const std::string& p_id)
		{
			switch (p_event)
			{
				case D3D11SystemEvent::D3D11_WINDOW_READY:
				{
					auto& appwindowNode{ m_entitygraph.node(p_id) };
					const auto appwindow{ appwindowNode.data() };

					const auto& mainwindows_rendering_aspect{ appwindow->aspectAccess(mage::core::renderingAspect::id) };

					const float characteristics_v_width{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportWidth")->getPurpose()};
					const float characteristics_v_height{ mainwindows_rendering_aspect.getComponent<float>("eg.std.viewportHeight")->getPurpose()};


					const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

					dataCloud->registerData<maths::Real4Vector>("screen_channel_number");
					dataCloud->updateDataValue<maths::Real4Vector>("screen_channel_number", maths::Real4Vector(0.0, 0.0, 0.0, 0.0));
					//dataCloud->updateDataValue<maths::Real4Vector>("screen_channel_number", maths::Real4Vector(1.0, 0.0, 0.0, 0.0));





					const auto window_dims{ dataCloud->readDataValue<mage::core::maths::IntCoords2D>("std.window_resol") };

					const int w_width{ window_dims.x() };
					const int w_height{ window_dims.y() };

					const auto rendering_quad_textures_channnelA{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };
					const auto rendering_quad_textures_channnelB{ Texture(Texture::Format::TEXTURE_RGB, w_width, w_height) };

					auto& screen_rendering_queue{ mage::helpers::plugRenderingQuad(m_entitygraph,
						"screen_queue",
						characteristics_v_width, characteristics_v_height,
						p_id,
						"screenRendering_Filter_DirectForward_Queue_Entity",
						"screenRendering_Filter_DirectForward_Quad_Entity",
						"screenRendering_Filter_DirectForward_View_Entity",
						"filter_directforward_switch2chan_vs",
						"filter_directforward_switch2chan_ps",

						{
							std::make_pair(Texture::STAGE_0, rendering_quad_textures_channnelA),
							std::make_pair(Texture::STAGE_1, rendering_quad_textures_channnelB)
						},
						Texture::STAGE_0

					)};

					Entity* screenRendering_Filter_DirectForward_Quad_Entity{ m_entitygraph.node("screenRendering_Filter_DirectForward_Quad_Entity").data() };

					auto& screenRendering_Filter_DirectForward_Quad_Entity_rendering_aspect{ screenRendering_Filter_DirectForward_Quad_Entity->aspectAccess(core::renderingAspect::id) };

					rendering::DrawingControl& fogDrawingControl{ screenRendering_Filter_DirectForward_Quad_Entity_rendering_aspect.getComponent<mage::rendering::DrawingControl>("drawingControl")->getPurpose() };
					fogDrawingControl.pshaders_map.push_back(std::make_pair("screen_channel_number", "input_channel"));





					// channel B : for debug purposes

					rendering::Queue debugChannelsRenderingQueue("debug_channel_queue");
					debugChannelsRenderingQueue.setTargetClearColor({ 0, 63, 128, 255 });
					debugChannelsRenderingQueue.enableTargetClearing(true);
					debugChannelsRenderingQueue.enableTargetDepthClearing(true);
					debugChannelsRenderingQueue.setTargetStage(Texture::STAGE_1);

					mage::helpers::plugRenderingQueue(m_entitygraph, debugChannelsRenderingQueue, "screenRendering_Filter_DirectForward_Quad_Entity", "bufferRendering_Scene_Debug_Queue_Entity");










					m_windowRenderingQueue = &screen_rendering_queue;

					auto sysEngine{ SystemEngine::getInstance() };
					const auto dataPrintSystem{ sysEngine->getSystem<mage::DataPrintSystem>(dataPrintSystemSlot) };

					dataPrintSystem->setRenderingQueue(m_windowRenderingQueue);



					//
					{
						rendering::RenderState rs_noculling(rendering::RenderState::Operation::SETCULLING, "cw");
						rendering::RenderState rs_zbuffer(rendering::RenderState::Operation::ENABLEZBUFFER, "false");
						rendering::RenderState rs_fill(rendering::RenderState::Operation::SETFILLMODE, "solid");
						rendering::RenderState rs_texturepointsampling(rendering::RenderState::Operation::SETTEXTUREFILTERTYPE, "point");

						const std::vector<rendering::RenderState> rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling };

						constexpr double gear_size{ 0.04 };

						m_loading_gear = helpers::plug2DSpriteWithSyncVariables(m_entitygraph, "screenRendering_Filter_DirectForward_Queue_Entity", "loading_gear", gear_size, gear_size,
							"sprite_vs", "sprite_ps", "gear.bmp", rs_list, 1000,
							mage::transform::WorldPosition::TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT);

						const auto& time_aspect{ m_loading_gear->aspectAccess(timeAspect::id) };

						core::SyncVariable& x_pos{ time_aspect.getComponent<SyncVariable>("x_pos")->getPurpose() };
						core::SyncVariable& y_pos{ time_aspect.getComponent<SyncVariable>("y_pos")->getPurpose() };

						const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
						const auto viewport{ dataCloud->readDataValue<maths::FloatCoords2D>("std.viewport") };

						x_pos.value = (-viewport.x() * 0.5) + gear_size * 0.5;
						y_pos.value = (-viewport.y() * 0.5) + gear_size * 0.5;
					}


					{
						rendering::RenderState rs_noculling(rendering::RenderState::Operation::SETCULLING, "cw");
						rendering::RenderState rs_zbuffer(rendering::RenderState::Operation::ENABLEZBUFFER, "false");
						rendering::RenderState rs_fill(rendering::RenderState::Operation::SETFILLMODE, "solid");
						rendering::RenderState rs_texturepointsampling(rendering::RenderState::Operation::SETTEXTUREFILTERTYPE, "point");

						const std::vector<rendering::RenderState> rs_list = { rs_noculling, rs_zbuffer, rs_fill, rs_texturepointsampling };

						constexpr double logo_size{ 0.06 };

						m_logo = helpers::plug2DSpriteWithPosition(m_entitygraph, "screenRendering_Filter_DirectForward_Queue_Entity", "logo", logo_size, logo_size,
							"sprite_vs", "sprite_ps", "mage.png", rs_list, 1000,
							mage::transform::WorldPosition::TransformationComposition::TRANSFORMATION_RELATIVE_FROM_PARENT);

						const auto& world_aspect{ m_logo->aspectAccess(worldAspect::id) };

						double& x_pos{ world_aspect.getComponent<double>("x_pos")->getPurpose() };
						double& y_pos{ world_aspect.getComponent<double>("y_pos")->getPurpose() };

						const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
						const auto viewport{ dataCloud->readDataValue<maths::FloatCoords2D>("std.viewport") };

						x_pos = (viewport.x() * 0.5) - logo_size * 0.5;
						y_pos = (-viewport.y() * 0.5) + logo_size * 0.5;
					}

				}
				break;
			}
		}
	};
	d3d11System->registerSubscriber(d3d11_cb);
}
