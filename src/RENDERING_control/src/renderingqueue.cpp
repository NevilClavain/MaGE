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

#include "renderingqueue.h"
#include "component.h"
#include "exceptions.h"

using namespace mage::rendering;
using namespace mage::core;
using namespace mage::core::maths;

Queue::Queue(const std::string& p_name) :
m_name(p_name)
{}

std::string Queue::getName() const
{
	return m_name;
}

Queue::Purpose Queue::getPurpose() const
{
	return m_purpose;
}

Queue::State Queue::getState() const
{
	return m_state;
}

void Queue::setState(State p_newstate)
{
	m_state = p_newstate;
}

void Queue::setScreenRenderingPurpose()
{
	m_purpose = Purpose::SCREEN_RENDERING;
}

void Queue::setBufferRenderingPurpose(mage::Texture& p_target_texture)
{
	m_purpose = Purpose::BUFFER_RENDERING;

	auto& render_target{ p_target_texture };

	render_target.m_source = Texture::Source::CONTENT_FROM_RENDERINGQUEUE;
	render_target.m_source_id = m_name;

	render_target.compute_resource_uid();

	m_targetTextureUID = render_target.getResourceUID();
	render_target.setState(Texture::State::BLOBLOADED);	
}

void Queue::enableTargetClearing(bool p_enable)
{
	m_clear_target = true;
}

void Queue::setTargetClearColor(const RGBAColor& p_color)
{
	m_target_clear_color = p_color;
}

bool Queue::getTargetClearing() const
{
	return m_clear_target;
}

RGBAColor Queue::getTargetClearColor() const
{
	return m_target_clear_color;
}

void Queue::pushText(const Queue::Text& p_text)
{
	m_texts.push_back(p_text);
}

Queue::QueueNodes Queue::getQueueNodes() const
{
	return m_queueNodes;
}

void Queue::setQueueNodes(const Queue::QueueNodes& p_nodes)
{
	m_queueNodes = p_nodes;
}

void Queue::setMainView(const std::string& p_entityId)
{
	m_mainView = p_entityId;
}

std::string	Queue::getMainView() const
{
	return m_mainView;
}

void Queue::setSecondaryView(const std::string& p_entityId)
{
	m_secondaryView = p_entityId;
}

std::string	Queue::getSecondaryView() const
{
	return m_secondaryView;
}

std::string	Queue::getTargetTextureUID() const
{
	return m_targetTextureUID;
}

void Queue::setTargetStage(size_t p_stage)
{
	m_targetStage = p_stage;
}

void Queue::enableTargetDepthClearing(bool p_enable)
{
	m_clear_target_depth = p_enable;
}

bool Queue::getTargetDepthClearing() const
{
	return m_clear_target_depth;
}

void Queue::resetStates()
{
	m_purpose = Purpose::UNDEFINED;
	m_state = State::WAIT_INIT;
}