
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

#include "logger_service.h"

#include "texture.h"
#include "filesystem.h"

using namespace mage;
using namespace mage::core;


void ResourceSystem::handleTexture(const std::string& p_filename, Texture& p_textureInfos)
{
	_MAGE_DEBUG(m_localLogger, std::string("Handle Texture ") + p_filename);

	const std::string textureAction{ "load_texture" };

	p_textureInfos.m_source = Texture::Source::CONTENT_FROM_FILE;
	p_textureInfos.m_source_id = p_filename;
	p_textureInfos.compute_resource_uid();

	const auto resourceUID{ p_textureInfos.getResourceUID() };

	if (!m_texturesBlobCache.count(resourceUID))
	{
		const auto task{ new mage::core::SimpleAsyncTask<>(textureAction, p_filename,
			[&,
				textureAction = textureAction,
				currentIndex = m_runnerIndex,
				filename = p_filename
			]()
			{
				// build full path
				const auto texture_path{ m_texturesBasePath + "/" + filename };

				try
				{
					auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

					_MAGE_DEBUG(m_localLoggerRunner, std::string("loading texture ") + filename + ", resource uid = " + p_textureInfos.getResourceUID());

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_TEXTURE_LOAD_BEGIN : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_TEXTURE_LOAD_BEGIN, filename);
					}

					mage::core::FileContent<unsigned char> texture_content(texture_path);
					texture_content.load();

					m_texturesBlobCache_mutex.lock();
					m_texturesBlobCache[p_textureInfos.getResourceUID()].fill(texture_content.getData(), texture_content.getDataSize());
					m_texturesBlobCache_mutex.unlock();

					p_textureInfos.m_file_content = m_texturesBlobCache.at(p_textureInfos.getResourceUID()).getData();
					p_textureInfos.m_file_content_size = m_texturesBlobCache.at(p_textureInfos.getResourceUID()).getDataSize();


					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_TEXTURE_LOAD_SUCCESS : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_TEXTURE_LOAD_SUCCESS, filename);
					}
					p_textureInfos.setState(Texture::State::BLOBLOADED);
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
		p_textureInfos.m_file_content = m_texturesBlobCache.at(resourceUID).getData();
		p_textureInfos.m_file_content_size = m_texturesBlobCache.at(resourceUID).getDataSize();
		p_textureInfos.setState(Texture::State::BLOBLOADED);
	}
}