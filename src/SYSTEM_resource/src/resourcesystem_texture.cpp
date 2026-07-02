
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

#include <string>
#include "resourcesystem.h"

#include "logger_service.h"

#include "texture.h"
#include "filesystem.h"

#include "datacloud.h"
#include "resourcestatecontroler.h"

using namespace mage;
using namespace mage::core;


void ResourceSystem::handleTexture(const std::string& p_filename, Texture& p_textureInfos)
{
	const std::string textureAction{ "load_texture" };

	p_textureInfos.setSource(Texture::Source::CONTENT_FROM_FILE, p_filename);

	const auto resourceUID{ p_textureInfos.getResourceUID() };

	if (!m_texturesBlobCache.count(resourceUID))
	{
		_MAGE_DEBUG(m_localLogger, std::string("launching task because texture not found in resource cache : ") + p_textureInfos.getSourceID() + std::string(" ") + p_textureInfos.getResourceUID());

		m_texturesBlobCache_mutex.lock();
		m_texturesBlobCache[resourceUID]; // to create entry
		m_texturesBlobCache[resourceUID].state = TextureCacheEntry::State::BLOBLOADING;
		m_texturesBlobCache_mutex.unlock();

		const auto task{ new mage::core::SimpleAsyncTask<>(textureAction, p_filename,
			[&,
				textureAction = textureAction,
				currentIndex = m_runnerIndex,
				filename = p_filename,
				resourceUID = resourceUID
			]()
			{
				// build full path
				const auto texture_path{ m_texturesBasePath + "/" + filename };

				try
				{
					auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

					_MAGE_TRACE(m_localLoggerRunner, std::string("loading texture ") + filename + ", resource uid = " + resourceUID);

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_TEXTURE_LOAD_BEGIN : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_TEXTURE_LOAD_BEGIN, filename);
					}

					mage::core::FileContent<unsigned char> texture_content(texture_path);
					texture_content.load();

					m_texturesBlobCache_mutex.lock();
					m_texturesBlobCache.at(resourceUID).texture_content.fill(texture_content.getData(), texture_content.getDataSize());
					m_texturesBlobCache_mutex.unlock();

					p_textureInfos.setFileContent(m_texturesBlobCache.at(resourceUID).texture_content.getData(), m_texturesBlobCache.at(resourceUID).texture_content.getDataSize());

					_MAGE_DEBUG(m_localLoggerRunner, std::string("task has loaded texture ") + p_textureInfos.getSourceID() + ", resource uid = " + p_textureInfos.getResourceUID());

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_TEXTURE_LOAD_SUCCESS : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_TEXTURE_LOAD_SUCCESS, filename);
					}

					const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
					dataCloud->updateDataValue<std::string>("mage.resourcesystem.event", "Texture loaded :" + filename);

					
					ResourceStateControler::getInstance()->update(p_textureInfos, Texture::State::BLOBLOADED);


					m_texturesBlobCache_mutex.lock();
					m_texturesBlobCache.at(resourceUID).state = TextureCacheEntry::State::BLOBLOADED;
					m_texturesBlobCache_mutex.unlock();


				}
				catch (const std::exception& e)
				{
					_MAGE_ERROR(m_localLoggerRunner, std::string("failed to manage ") + texture_path + " : reason = " + e.what());

					// send error status to main thread and let terminate
					const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, filename, textureAction };
					m_runner[currentIndex].get()->m_mailbox_out.push(report);
				}
			}
		) };

		m_runner[m_runnerIndex].get()->m_mailbox_in.push(task);

		m_runnerIndex++;
		if (m_runnerIndex == nbRunners)
		{
			m_runnerIndex = 0;
		}
	}
	else
	{
		_MAGE_DEBUG(m_localLogger, std::string("texture found in resource cache : ") + p_textureInfos.getSourceID() + std::string(" ") + p_textureInfos.getResourceUID());

		m_texturesBlobCache_mutex.lock();
		const auto texture_state{ m_texturesBlobCache.at(resourceUID).state };
		m_texturesBlobCache_mutex.unlock();

		if (TextureCacheEntry::State::BLOBLOADED == texture_state)
		{
			p_textureInfos.setFileContent(m_texturesBlobCache.at(resourceUID).texture_content.getData(), m_texturesBlobCache.at(resourceUID).texture_content.getDataSize());
			ResourceStateControler::getInstance()->update(p_textureInfos, Texture::State::BLOBLOADED);
		}
	}
}