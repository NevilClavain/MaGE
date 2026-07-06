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

#include "renderingqueue.h"
#include "component.h"
#include "exceptions.h"
#include "resourcestatecontroler.h"

using namespace mage::rendering;
using namespace mage::core;
using namespace mage::core::maths;

Queue::Queue(const std::string& p_name) :
m_name(p_name)
{}

Queue::Queue(const Queue& p_other)
{
	m_name = p_other.m_name;
	m_purpose = p_other.m_purpose;
	m_state = p_other.m_state;
	m_clear_target = p_other.m_clear_target;
	m_target_clear_color = p_other.m_target_clear_color;
	m_clear_target_depth = p_other.m_clear_target_depth;
	m_texts = p_other.m_texts;
	m_queueNodes = p_other.m_queueNodes;


	m_queueNodesA = p_other.m_queueNodesA;
	m_queueNodesB = p_other.m_queueNodesB;

	m_queueMutex.lock();
	m_queueNodesFront = p_other.m_queueNodesFront;
	m_queueNodesBack = p_other.m_queueNodesBack;
	m_queueMutex.unlock();

	m_mainView = p_other.m_mainView;
	m_secondaryView = p_other.m_secondaryView;
	m_targetStage = p_other.m_targetStage;

	m_targetTextureUID = p_other.m_targetTextureUID;
}

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

	render_target.setSource(Texture::Source::CONTENT_FROM_RENDERINGQUEUE, m_name);

	m_targetTextureUID = render_target.getResourceUID();
		
	ResourceStateControler::getInstance()->update(render_target, Texture::State::BLOBLOADED);
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

std::vector<Queue::Text> Queue::getTexts() const
{
	return m_texts;
}

void Queue::setTexts(const std::vector<Queue::Text>& p_texts)
{
	m_texts = p_texts;
}

void Queue::clearTexts()
{
	m_texts.clear();
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

size_t Queue::getTargetStage() const
{
	return m_targetStage;
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

Queue::QueueNodes* Queue::queueNodesFrontAccess()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	return m_queueNodesFront;
}

Queue::QueueNodes* Queue::queueNodesBackAccess()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);
	return m_queueNodesBack;
}

void Queue::switchQueues()
{
	std::lock_guard<std::mutex> lock(m_queueMutex);

	if (m_queueNodesFront == &m_queueNodesA)
	{
		m_queueNodesFront = &m_queueNodesB;
		m_queueNodesBack = &m_queueNodesA;
	}
	else // (m_queueNodesFront == &m_queueNodesB)
	{
		m_queueNodesFront = &m_queueNodesA;
		m_queueNodesBack = &m_queueNodesB;
	}
}

