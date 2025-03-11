
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

#include "resourcesystem.h"

#include "logger_service.h"

#include "shaders_service.h"
#include "shader.h"

#include "datacloud.h"

using namespace mage;
using namespace mage::core;


void ResourceSystem::handleShader(const std::string& p_filename, Shader& p_shaderInfos)
{
	const auto shaderType{ p_shaderInfos.getType() };

	_MAGE_DEBUG(m_localLogger, std::string("Handle shader ") + p_filename + std::string(" shader type ") + std::to_string(shaderType));

	const std::string shaderAction{ "load_shader" };

	const auto task{ new mage::core::SimpleAsyncTask<>(shaderAction, p_filename,
		[&,
			shaderType = shaderType,
			shaderAction = shaderAction,
			currentIndex = m_runnerIndex,
			filename = p_filename
		]()
		{
			_MAGE_DEBUG(m_localLoggerRunner, std::string("loading ") + filename + " shader type = " + std::to_string(shaderType));

			// build full path
			const auto shader_path{ m_shadersBasePath + "/" + filename + ".hlsl"};
			const auto shader_metadata_path{ m_shadersBasePath + "/" + filename + ".json" };
			try
			{
				mage::core::FileContent<const char> shader_src_content(shader_path);
				shader_src_content.load();

				// no mutex needed here (only this thread access it)
				p_shaderInfos.setContentSize(shader_src_content.getDataSize());
				p_shaderInfos.setContent(shader_src_content.getData());
				p_shaderInfos.m_source_id = filename;
				p_shaderInfos.compute_resource_uid();

				const auto shaderCacheDirectory{ m_shadersCachePath + "/" + filename };

				bool generate_cache_entry{ false };

				///////// check driver version change...

				// get current driver version
				const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
				const auto current_driver{ dataCloud->readDataValue<std::string>("std.gpu_driver") };

				bool update_driver_text{ false };

				///////// check if shader exists in cache...

				if (!fileSystem::exists(shaderCacheDirectory))
				{
					_MAGE_TRACE(m_localLoggerRunner, std::string("cache directory missing : ") + shaderCacheDirectory);

					// create all
					fileSystem::createDirectory(shaderCacheDirectory);

					mage::core::FileContent<const char> driverversion_content(m_shadersCachePath + "/driverversion.text");
					driverversion_content.save(current_driver.c_str(), current_driver.length());

					generate_cache_entry = true;
				}
				else
				{
					///////////////////////////////////////////////////////////////////////////////

					if (fileSystem::exists(m_shadersCachePath + "/driverversion.text"))
					{
						// file exists
						mage::core::FileContent<const char> driverversion_content(m_shadersCachePath + "/driverversion.text");
						driverversion_content.load();

						const std::string last_driverversion(driverversion_content.getData(), driverversion_content.getDataSize());

						if (current_driver != last_driverversion)
						{
							update_driver_text = true;
						}
					}
					else
					{
						update_driver_text = true;
					}

					if (update_driver_text) // update driver text and so rebuild shaders
					{
						mage::core::FileContent<const char> driverversion_content(m_shadersCachePath + "/driverversion.text");
						driverversion_content.save(current_driver.c_str(), current_driver.length());
						m_forceAllShadersRegeneration = true;
					}

					///////////////////////////////////////////////////////////////////////////////

					if (m_forceAllShadersRegeneration)
					{
						generate_cache_entry = true;
					}

					_MAGE_TRACE(m_localLoggerRunner, std::string("cache directory exists : ") + shaderCacheDirectory);

					// check if cache md5 file exists AND compiled shader exists
					if (!fileSystem::exists(shaderCacheDirectory + "/bc.md5") || !fileSystem::exists(shaderCacheDirectory + "/bc.code"))
					{
						_MAGE_TRACE(m_localLoggerRunner, std::string("cache file missing !"));
						generate_cache_entry = true;
					}
					else
					{
						_MAGE_TRACE(m_localLoggerRunner, std::string("cache md5 file exists : ") + shaderCacheDirectory + "/bc.md5");

						// load cache md5 file
						mage::core::FileContent<char> cache_md5_content(shaderCacheDirectory + "/bc.md5");
						cache_md5_content.load();

						// check if md5 are equals

						if (std::string(cache_md5_content.getData(), cache_md5_content.getDataSize()) != p_shaderInfos.m_resource_uid)
						{
							_MAGE_TRACE(m_localLoggerRunner, std::string("MD5 not matching ! : ") + filename);
							generate_cache_entry = true;
						}
						else
						{
							// load bc.code file
							_MAGE_TRACE(m_localLoggerRunner, std::string("MD5 matches ! : ") + filename);
						}
					}
				}

				if (generate_cache_entry)
				{
					_MAGE_TRACE(m_localLoggerRunner, std::string("generating cache entry : ") + filename);

					std::unique_ptr<char[]> shaderBytes;
					size_t shaderBytesLength;

					auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_COMPILATION_BEGIN : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_BEGIN, filename);
					}

					bool compilationStatus;

					if (vertexShader == shaderType)
					{
						services::ShadersCompilationService::getInstance()->requestVertexCompilationShader(fileSystem::splitPath(shader_path).first, shader_src_content, shaderBytes, shaderBytesLength, compilationStatus);
					}
					else if (pixelShader == shaderType)
					{
						services::ShadersCompilationService::getInstance()->requestPixelCompilationShader(fileSystem::splitPath(shader_path).first, shader_src_content, shaderBytes, shaderBytesLength, compilationStatus);
					}

					if (compilationStatus)
					{
						_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_COMPILATION_SUCCESS : " + filename);
						for (const auto& call : m_callbacks)
						{
							call(ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_SUCCESS, filename);
						}

						mage::core::FileContent<char> cache_code_content(shaderCacheDirectory + "/bc.code");
						cache_code_content.save(shaderBytes.get(), shaderBytesLength);

						// create cache md5 file
						mage::core::FileContent<const char> shader_md5_content(shaderCacheDirectory + "/bc.md5");
						const std::string shaderMD5{ p_shaderInfos.m_resource_uid };
						shader_md5_content.save(shaderMD5.c_str(), shaderMD5.length());

						// and transfer file content to p_shaderInfos 'code' buffer
						core::Buffer<char> shaderCode;
						shaderCode.fill(shaderBytes.get(), shaderBytesLength);
						p_shaderInfos.setCode(shaderCode);
					}
					else
					{

						_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_COMPILATION_ERROR : " + filename);
						for (const auto& call : m_callbacks)
						{
							call(ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_ERROR, filename);
						}

						std::string compilErrorMessage(shaderBytes.get(), shaderBytesLength);
						_EXCEPTION("shader compilation error : " + compilErrorMessage)
					}
				}
				else
				{
					auto& eventsLogger{ services::LoggerSharing::getInstance()->getLogger("Events") };

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_LOAD_BEGIN : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_SHADER_LOAD_BEGIN, filename);
					}

					// load bc.code file
					mage::core::FileContent<char> cache_code_content(shaderCacheDirectory + "/bc.code");
					cache_code_content.load();

					// transfer file content to p_shaderInfos 'code' buffer
					core::Buffer<char> shaderCode;
					shaderCode.fill(cache_code_content.getData(), cache_code_content.getDataSize());
					p_shaderInfos.setCode(shaderCode);

					_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_LOAD_SUCCESS : " + filename);
					for (const auto& call : m_callbacks)
					{
						call(ResourceSystemEvent::RESOURCE_SHADER_LOAD_SUCCESS, filename);
					}
				}

				///// manage metadata json file

				mage::core::FileContent<const char> shadermetadata_src_content(shader_metadata_path);
				shadermetadata_src_content.load();

				const auto metadataSize{ shadermetadata_src_content.getDataSize() };
				const std::string metadata(shadermetadata_src_content.getData(), metadataSize);

				// json parser seem to be not thread-safe -> enter critical section
				m_jsonparser_mutex.lock();
				mage::core::Json<Shader> jsonParser;
				jsonParser.registerSubscriber(m_jsonparser_cb);
				const auto logParseStatus{ jsonParser.parse(metadata, &p_shaderInfos) };
				m_jsonparser_mutex.unlock();

				if (logParseStatus < 0)
				{
					_EXCEPTION("JSON parse error on " + shader_metadata_path);
				}

				////////////////////////////////

				p_shaderInfos.setState(Shader::State::BLOBLOADED);
			}
			catch (const std::exception& e)
			{
				_MAGE_ERROR(m_localLoggerRunner, std::string("failed to manage ") + shader_path + " : reason = " + e.what());

				// send error status to main thread and let terminate
				const Runner::TaskReport report{ RunnerEvent::TASK_ERROR, filename, shaderAction };
				m_runner[currentIndex].get()->m_mailbox_out.push(report);
			}
		}
	) };

	_MAGE_DEBUG(m_localLogger, "Pushing to runner number : " + std::to_string(m_runnerIndex));

	m_runner[m_runnerIndex].get()->m_mailbox_in.push(task);

	m_runnerIndex++;
	if (m_runnerIndex == nbRunners)
	{
		m_runnerIndex = 0;
	}
}
