
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
#include "timecontrol.h"

using namespace mage;
using namespace mage::core;

void ModuleImpl::onKeyPress(long p_key)
{
	if (!m_appReady) return;

	auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(renderingQueueSystemSlot)) };
	auto& [mainView, secondaryView] { renderingQueueSystemInstance->getViewGroupCurrentViews("player_camera") };


	if ('Q' == p_key)
	{
		if ("camera_Entity" == mainView)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("cameraGblJoint_Entity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 20.0;
		}
	}
	else if ('W' == p_key)
	{
		if ("camera_Entity" == mainView)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("cameraGblJoint_Entity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = -20.0;
		}
	}
	else if (VK_CONTROL == p_key)
	{
		m_left_ctrl = true;
	}
}

void ModuleImpl::onEndKeyPress(long p_key)
{
	if (!m_appReady) return;

	auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

	auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(SystemEngine::getInstance()->getSystem(renderingQueueSystemSlot)) };
	auto& [mainView, secondaryView] { renderingQueueSystemInstance->getViewGroupCurrentViews("player_camera") };


	if (VK_ESCAPE == p_key)
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

	else if (VK_F7 == p_key)
	{
		auto tc{ TimeControl::getInstance() };

		auto mode{ tc->getTimeFactor() };
		if (TimeControl::TimeScale::FREEZE == mode)
		{
			tc->setTimeFactor(TimeControl::TimeScale::NORMAL_TIME);
		}
		else if (TimeControl::TimeScale::NORMAL_TIME == mode)
		{
			tc->setTimeFactor(TimeControl::TimeScale::FREEZE);
		}
	}

	else if (VK_F8 == p_key)
	{
		auto renderingQueueSystem{ SystemEngine::getInstance()->getSystem(renderingQueueSystemSlot) };
		auto renderingQueueSystemInstance{ dynamic_cast<mage::RenderingQueueSystem*>(renderingQueueSystem) };

		renderingQueueSystemInstance->requestRenderingqueueLogging("screenRendering_Filter_DirectForward_Queue_Entity");
	}

	else if (VK_F9 == p_key)
	{
		helpers::logEntitygraph(m_entitygraph, true);
	}

	else if ('Q' == p_key)
	{
		if ("camera_Entity" == mainView)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("cameraGblJoint_Entity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 0.0;
		}
	}

	else if ('W' == p_key)
	{
		if ("camera_Entity" == mainView)
		{
			auto& gblJointEntityNode{ m_entitygraph.node("cameraGblJoint_Entity") };
			const auto gblJointEntity{ gblJointEntityNode.data() };

			auto& world_aspect{ gblJointEntity->aspectAccess(core::worldAspect::id) };

			double& speed{ world_aspect.getComponent<double>("gbl_speed")->getPurpose() };

			speed = 0.0;
		}
	}

	else if (VK_CONTROL == p_key)
	{
		m_left_ctrl = false;
	}
}

void ModuleImpl::onKeyPulse(long p_key)
{
}

void ModuleImpl::onChar(long p_char, long p_scan)
{
}