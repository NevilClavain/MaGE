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

#include "resourcesystem.h"

#include "entity.h"
#include "entitygraph.h"
#include "aspects.h"
#include "ecshelpers.h"

#include "logger_service.h"

#include "shader.h"
#include "texture.h"
#include "trianglemeshe.h"

#include "filesystem.h"

using namespace mage;
using namespace mage::core;

ResourceSystem::ResourceSystem(Entitygraph& p_entitygraph) : System(p_entitygraph),
m_localLogger("ResourceSystem", mage::core::logger::Configuration::getInstance()),
m_localLoggerRunner("ResourceSystemRunner", mage::core::logger::Configuration::getInstance())
{
	m_runner.reserve(nbRunners);

	for (int i = 0; i < nbRunners; i++)
	{
		m_runner.push_back(std::make_unique< mage::core::Runner>());
	}
	
	///////// check & create shader cache if needed
	
	if (!fileSystem::exists(m_shadersCachePath))
	{
		_MAGE_DEBUG(m_localLogger, std::string("Shader cache missing, creating it..."));
		fileSystem::createDirectory(m_shadersCachePath);

		auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };
		_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_CACHE_CREATED");

		for (const auto& call : m_callbacks)
		{
			call(ResourceSystemEvent::RESOURCE_SHADER_CACHE_CREATED, m_shadersCachePath);
		}
	}
	
	/////////////////////////////////////////////

	const Runner::Callback cb
	{
		[&, this](mage::core::RunnerEvent p_event, const std::string& p_target_descr, const std::string& p_action_descr)
		{
			if (mage::core::RunnerEvent::TASK_ERROR == p_event)
			{
				_EXCEPTION(std::string("failed action ") + p_action_descr + " on target " + p_target_descr);
			}
			else if (mage::core::RunnerEvent::TASK_DONE == p_event)
			{
				_MAGE_DEBUG(m_localLoggerRunner, std::string("TASK_DONE ") + p_target_descr + " " + p_action_descr);
			}

		}
	};

	for (int i = 0; i < nbRunners; i++)
	{
		m_runner[i].get()->registerSubscriber(cb);
		m_runner[i].get()->startup();
	}
}

ResourceSystem::~ResourceSystem()
{
	_MAGE_DEBUG(m_localLogger, std::string("Exiting..."));
}

void ResourceSystem::run()
{
	const auto forEachResourceAspect
	{
		[&](Entity* p_entity, const ComponentContainer& p_resource_components)
		{

			////// Handle shaders ///////////
			//const auto shaders_list{ p_resource_aspect.getComponentsByType<Shader>() };

			const auto shaders_list{ p_resource_components.getComponentsByType<std::pair<std::string, Shader>>() };
			for (auto& e : shaders_list)
			{
				auto& shader{ e->getPurpose().second };
				const auto filename{ e->getPurpose().first};

				const auto state{ shader.getState() };
				if (Shader::State::INIT == state)
				{
					handleShader(filename, shader);
					shader.setState(Shader::State::BLOBLOADING);
				}
			}
			////// Handle textures ///////////
			const auto textures_list{ p_resource_components.getComponentsByType<std::pair<size_t, std::pair<std::string, Texture>>>() };
			for (auto& e : textures_list)
			{
				auto& staged_texture{ e->getPurpose() };
				Texture& texture{ staged_texture.second.second };
				const auto filename{ staged_texture.second.first };
					
				const auto state{ texture.getState() };
				if (Texture::State::INIT == state)
				{
					handleTexture(filename, texture);
					texture.setState(Texture::State::BLOBLOADING);
				}
			}

			////// Handle meshes //////////////
			const auto meshes_list{ p_resource_components.getComponentsByType<std::pair<std::pair<std::string, std::string>, TriangleMeshe>>() };
			const auto& nodes_list{ p_resource_components.getComponentsByType<std::map<std::string, SceneNode>>() };
			
			if (meshes_list.size() > 0)
			{
				auto& meshe_descr{ meshes_list.at(0)->getPurpose() };
				TriangleMeshe& meshe{ meshe_descr.second };

				const auto& ids{ meshe_descr.first };

				const std::string& file_path{ ids.second };
				const std::string& meshe_id{ ids.first };

				const auto state{ meshe.getState() };
				if (TriangleMeshe::State::INIT == state)
				{
					handleSceneFile(file_path, meshe_id, meshe, nodes_list);
					meshe.setState(TriangleMeshe::State::BLOBLOADING);
				}
			}
		}
	};

	mage::helpers::extractAspectsTopDown<mage::core::resourcesAspect>(m_entitygraph, forEachResourceAspect);

	for (int i = 0; i < nbRunners; i++)
	{
		m_runner[i].get()->dispatchEvents();
	}	
}

void ResourceSystem::killRunner()
{
	mage::core::RunnerKiller runnerKiller;

	for (int i = 0; i < nbRunners; i++)
	{
		m_runner[i].get()->m_mailbox_in.push(&runnerKiller);
		m_runner[i].get()->join();
	}
}

size_t ResourceSystem::getNbBusyRunners() const
{
	size_t count{ 0 };

	for (int i = 0; i < nbRunners; i++)
	{
		if (m_runner[i].get()->isBusy())
		{
			count++;
		}
	}

	return count;
}