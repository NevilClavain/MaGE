
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

#include "module_impl.h"
#include <string>

#include "logger_service.h"
#include "logging.h"
#include "renderingqueuesystem.h"
#include "sysengine.h"
#include "datacloud.h"
#include "aspects.h"
#include "syncvariable.h"
#include "entitygraph_helpers.h"

using namespace mage;
using namespace mage::core;

void ModuleImpl::onKeyPress(long p_key)
{
	const auto current_view_entity_id{ m_bufferRenderingQueue->getMainView() };

	if ('Q' == p_key)
	{
		if ("Camera01Entity" == current_view_entity_id)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("gblJointEntity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 10.0;
		}
		else if ("Camera02Entity" == current_view_entity_id)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("fullgbl_speed")->getPurpose() };

			speed = 2.0;
		}
		else if ("Camera03Entity" == current_view_entity_id)
		{
			auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
			const auto sliderJointEntity{ sliderJointEntityNode.data() };
			auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };

			auto& z_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("z_slide_pos")->getPurpose() };

			z_slide_pos.direction = SyncVariable::Direction::DEC;
			z_slide_pos.state = SyncVariable::State::ON;
		}
	}
	else if ('W' == p_key)
	{
		if ("Camera01Entity" == current_view_entity_id)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("gblJointEntity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = -10.0;
		}
		else if ("Camera02Entity" == current_view_entity_id)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("fullgbl_speed")->getPurpose() };

			speed = -2.0;
		}
		else if ("Camera03Entity" == current_view_entity_id)
		{
			auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
			const auto sliderJointEntity{ sliderJointEntityNode.data() };
			auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };

			auto& z_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("z_slide_pos")->getPurpose() };

			z_slide_pos.direction = SyncVariable::Direction::INC;
			z_slide_pos.state = SyncVariable::State::ON;
		}
	}

	if ("Camera02Entity" == current_view_entity_id)
	{
		if (VK_LEFT == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_y{ world_aspect.getComponent<double>("rspeed_y")->getPurpose() };
			rspeed_y = 9.0f;
		}
		else if (VK_RIGHT == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_y{ world_aspect.getComponent<double>("rspeed_y")->getPurpose() };
			rspeed_y = -9.0f;
		}
		else if (VK_UP == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_x{ world_aspect.getComponent<double>("rspeed_x")->getPurpose() };
			rspeed_x = 9.0f;

		}
		else if (VK_DOWN == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_x{ world_aspect.getComponent<double>("rspeed_x")->getPurpose() };
			rspeed_x = -9.0;
		}
		else if ('A' == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_z{ world_aspect.getComponent<double>("rspeed_z")->getPurpose() };
			rspeed_z = 9.0;
		}
		else if ('E' == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_z{ world_aspect.getComponent<double>("rspeed_z")->getPurpose() };
			rspeed_z = -9.0;
		}
	}
	else if ("Camera03Entity" == current_view_entity_id)
	{
		auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
		const auto sliderJointEntity{ sliderJointEntityNode.data() };
		auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };


		if (VK_LEFT == p_key)
		{
			auto& x_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("x_slide_pos")->getPurpose() };

			x_slide_pos.direction = SyncVariable::Direction::DEC;
			x_slide_pos.state = SyncVariable::State::ON;

		}
		else if (VK_RIGHT == p_key)
		{
			auto& x_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("x_slide_pos")->getPurpose() };

			x_slide_pos.direction = SyncVariable::Direction::INC;
			x_slide_pos.state = SyncVariable::State::ON;

		}
		else if (VK_UP == p_key)
		{
			auto& y_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("y_slide_pos")->getPurpose() };

			y_slide_pos.direction = SyncVariable::Direction::DEC;
			y_slide_pos.state = SyncVariable::State::ON;

		}
		else if (VK_DOWN == p_key)
		{
			auto& y_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("y_slide_pos")->getPurpose() };

			y_slide_pos.direction = SyncVariable::Direction::INC;
			y_slide_pos.state = SyncVariable::State::ON;
		}
	}
}

void ModuleImpl::onEndKeyPress(long p_key)
{
	auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

	const auto current_view_entity_id{ m_bufferRenderingQueue->getMainView() };


	if (VK_SPACE == p_key)
	{
		_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> CLOSE_APP");
		for (const auto& call : m_callbacks)
		{
			call(mage::interfaces::ModuleEvents::CLOSE_APP, 0);
		}
	}
	else if (VK_F1 == p_key)
	{
		if (m_show_mouse_cursor)
		{
			m_show_mouse_cursor = false;
		}
		else
		{
			m_show_mouse_cursor = true;
		}

		_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> MOUSE_DISPLAY_CHANGED");
		for (const auto& call : m_callbacks)
		{
			call(mage::interfaces::ModuleEvents::MOUSE_DISPLAY_CHANGED, (int)m_show_mouse_cursor);
		}
	}
	else if (VK_F2 == p_key)
	{
		if (m_mouse_relative_mode)
		{
			m_mouse_relative_mode = false;
		}
		else
		{
			m_mouse_relative_mode = true;
		}

		_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> MOUSE_MODE_CHANGED");
		for (const auto& call : m_callbacks)
		{
			call(mage::interfaces::ModuleEvents::MOUSE_MODE_CHANGED, (int)m_mouse_relative_mode);
		}
	}
	else if (VK_F3 == p_key)
	{
		if (true == m_quadEntity0_state_request)
		{
			m_quadEntity0_state_request = false;
		}
		else
		{
			m_quadEntity0_state_request = true;
		}
	}
	else if (VK_F4 == p_key)
	{
		if (true == m_quadEntity1_state_request)
		{
			m_quadEntity1_state_request = false;
		}
		else
		{
			m_quadEntity1_state_request = true;
		}
	}
	else if (VK_F5 == p_key)
	{
		if (true == m_quadEntity2_state_request)
		{
			m_quadEntity2_state_request = false;
		}
		else
		{
			m_quadEntity2_state_request = true;
		}
	}

	else if (VK_F7 == p_key)
	{
		helpers::logEntitygraph(m_entitygraph);
	}


	else if (VK_F8 == p_key)
	{
		auto renderingQueueSystem{ SystemEngine::getInstance()->getSystem(renderingQueueSystemSlot) };
		auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(renderingQueueSystem) };

		renderingQueueSystemInstance->requestRenderingqueueLogging("bufferRenderingEntity");		
	}

	else if (VK_F9 == p_key)
	{
		helpers::logEntitygraph(m_entitygraph, true);
	}

	else if ('Q' == p_key)
	{
		if ("Camera01Entity" == current_view_entity_id)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("gblJointEntity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 0.0;
		}
		else if ("Camera02Entity" == current_view_entity_id)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("fullgbl_speed")->getPurpose() };

			speed = 0.0;
		}
		else if ("Camera03Entity" == current_view_entity_id)
		{
			auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
			const auto sliderJointEntity{ sliderJointEntityNode.data() };
			auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };

			auto& z_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("z_slide_pos")->getPurpose() };
			z_slide_pos.state = SyncVariable::State::OFF;
		}
	}

	else if ('W' == p_key)
	{
		if ("Camera01Entity" == current_view_entity_id)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("gblJointEntity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 0.0;
		}
		else if ("Camera02Entity" == current_view_entity_id)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("fullgbl_speed")->getPurpose() };

			speed = 0.0;
		}
		else if ("Camera03Entity" == current_view_entity_id)
		{
			auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
			const auto sliderJointEntity{ sliderJointEntityNode.data() };
			auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };

			auto& z_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("z_slide_pos")->getPurpose() };
			z_slide_pos.state = SyncVariable::State::OFF;
		}
	}

	if ("Camera02Entity" == current_view_entity_id)
	{

		if (VK_LEFT == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_y{ world_aspect.getComponent<double>("rspeed_y")->getPurpose() };
			rspeed_y = 0.0;
		}
		else if (VK_RIGHT == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_y{ world_aspect.getComponent<double>("rspeed_y")->getPurpose() };
			rspeed_y = 0.0;
		}
		else if (VK_UP == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_x{ world_aspect.getComponent<double>("rspeed_x")->getPurpose() };
			rspeed_x = 0.0;

		}
		else if (VK_DOWN == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_x{ world_aspect.getComponent<double>("rspeed_x")->getPurpose() };
			rspeed_x = 0.0;
		}
		else if ('A' == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_z{ world_aspect.getComponent<double>("rspeed_z")->getPurpose() };
			rspeed_z = 0.0;
		}
		else if ('E' == p_key)
		{
			auto& fullGblJointEntityNode{ m_entitygraph.node("fullGblJointEntity") };
			const auto fullGblJointEntity{ fullGblJointEntityNode.data() };

			auto& world_aspect{ fullGblJointEntity->aspectAccess(core::worldAspect::id) };

			double& rspeed_z{ world_aspect.getComponent<double>("rspeed_z")->getPurpose() };
			rspeed_z = 0.0;
		}
	}
	else if ("Camera03Entity" == current_view_entity_id)
	{
		auto& sliderJointEntityNode{ m_entitygraph.node("sliderJointEntity") };
		const auto sliderJointEntity{ sliderJointEntityNode.data() };
		auto& slider_time_aspect{ sliderJointEntity->aspectAccess(core::timeAspect::id) };


		if (VK_LEFT == p_key)
		{
			auto& x_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("x_slide_pos")->getPurpose() };
			x_slide_pos.state = SyncVariable::State::OFF;

		}
		else if (VK_RIGHT == p_key)
		{
			auto& x_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("x_slide_pos")->getPurpose() };
			x_slide_pos.state = SyncVariable::State::OFF;
		}
		else if (VK_UP == p_key)
		{
			auto& y_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("y_slide_pos")->getPurpose() };
			y_slide_pos.state = SyncVariable::State::OFF;

		}
		else if (VK_DOWN == p_key)
		{
			auto& y_slide_pos{ slider_time_aspect.getComponent< SyncVariable>("y_slide_pos")->getPurpose() };
			y_slide_pos.state = SyncVariable::State::OFF;

		}
	}
}

void ModuleImpl::onKeyPulse(long p_key)
{
}

void ModuleImpl::onChar(long p_char, long p_scan)
{
}