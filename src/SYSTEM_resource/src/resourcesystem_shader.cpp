
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
#include <json_struct/json_struct.h>

#include "resourcesystem.h"
#include "logger_service.h"
#include "shaders_service.h"
#include "datacloud.h"
#include "resourcestatecontroler.h"

using namespace mage;
using namespace mage::core;


void ResourceSystem::handleShader(const std::string& p_filename, Shader& p_shaderInfos)
{
	const auto shaderType{ p_shaderInfos.getType() };

	const std::string shaderAction{ "load_shader" };

	p_shaderInfos.setSourceID(p_filename);

	const auto resourceUID{ p_shaderInfos.getResourceUID() };

	if (!m_shadersCache.count(resourceUID))
	{
		m_shadersCache_mutex.lock();
		m_shadersCache[resourceUID]; // to create entry
		m_shadersCache[resourceUID].state = ShaderCacheEntry::State::BLOBLOADING;
		m_shadersCache_mutex.unlock();

		const auto task{ new mage::core::SimpleAsyncTask<>(shaderAction, p_filename,
			[&,
				shaderType = shaderType,
				shaderAction = shaderAction,
				currentIndex = m_runnerIndex,
				filename = p_filename,
				resourceUID = resourceUID
			]()
			{
				// build full path
				const auto shader_path{ m_shadersBasePath + "/" + filename + ".hlsl"};
				const auto shader_metadata_path{ m_shadersBasePath + "/" + filename + ".json" };
				try
				{
					mage::core::FileContent<const char> shader_src_content(shader_path);
					shader_src_content.load();

					m_shadersCache_mutex.lock();
					m_shadersCache.at(resourceUID).shader_source = std::string(shader_src_content.getData(), shader_src_content.getDataSize());
					m_shadersCache_mutex.unlock();

					p_shaderInfos.setFileContent(m_shadersCache.at(resourceUID).shader_source.c_str(), m_shadersCache.at(resourceUID).shader_source.size());

					_MAGE_DEBUG(m_localLoggerRunner, std::string("loading shader ") + filename + " type = " + std::to_string(shaderType) + ", resource uid = " + resourceUID);

					const auto shaderCacheDirectory{ m_shadersCachePath + "/" + filename };

					bool generate_cache_entry{ false };

					///////// check driver version change...

					// get current driver version
					const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
					const auto current_driver{ dataCloud->readDataValue<std::string>("mage.infos.gpu_driver") };

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

							if (std::string(cache_md5_content.getData(), cache_md5_content.getDataSize()) != p_shaderInfos.getContentHash())
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

						const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
						dataCloud->updateDataValue<std::string>("mage.resourcesystem.event", "Shader compilation: " + filename + " BEGIN");

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

							const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
							dataCloud->updateDataValue<std::string>("mage.resourcesystem.event", "Shader compilation " + filename + " SUCCESS");

							mage::core::FileContent<char> cache_code_content(shaderCacheDirectory + "/bc.code");
							cache_code_content.save(shaderBytes.get(), shaderBytesLength);

							// create cache md5 file
							mage::core::FileContent<const char> shader_md5_content(shaderCacheDirectory + "/bc.md5");

							const std::string shaderMD5{ p_shaderInfos.getContentHash()};
							shader_md5_content.save(shaderMD5.c_str(), shaderMD5.length());

							m_shadersCache_mutex.lock();
							m_shadersCache.at(resourceUID).shader_code.fill(shaderBytes.get(), shaderBytesLength);
							m_shadersCache_mutex.unlock();

							p_shaderInfos.setCode(m_shadersCache.at(resourceUID).shader_code.getData(), m_shadersCache.at(resourceUID).shader_code.getDataSize());
						}
						else
						{

							_MAGE_DEBUG(eventsLogger, "EMIT EVENT -> RESOURCE_SHADER_COMPILATION_ERROR : " + filename);
							for (const auto& call : m_callbacks)
							{
								call(ResourceSystemEvent::RESOURCE_SHADER_COMPILATION_ERROR, filename);
							}

							const auto dataCloud{ mage::rendering::Datacloud::getInstance() };
							dataCloud->updateDataValue<std::string>("mage.resourcesystem.event", "Shader compilation " + filename + " ERROR");

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

						m_shadersCache_mutex.lock();
						m_shadersCache.at(resourceUID).shader_code.fill(cache_code_content.getData(), cache_code_content.getDataSize());
						m_shadersCache_mutex.unlock();

						p_shaderInfos.setCode(m_shadersCache.at(resourceUID).shader_code.getData(), m_shadersCache.at(resourceUID).shader_code.getDataSize());

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

					json::ShaderMetadata shader_metadata;

					// json parser seem to be not thread-safe -> enter critical section
					m_jsonparser_mutex.lock();

					JS::ParseContext parseContext(metadata);

					const auto metadataParseStatus{ parseContext.parseTo(shader_metadata) };

					thread_local Shader::GenericArgument generic_argument;
					for (const auto& e : shader_metadata.real4vector_inputs)
					{
						generic_argument.argument_type = "Real4Vector";
						generic_argument.argument_id = e.argument_id;
						generic_argument.shader_register = e.register_index;

						p_shaderInfos.addGenericArgument(generic_argument);

						m_shadersCache_mutex.lock();
						m_shadersCache.at(resourceUID).generic_arguments.push_back(generic_argument);
						m_shadersCache_mutex.unlock();
					}

					thread_local Shader::VectorArrayArgument vector_array_argument;
					for (const auto& e : shader_metadata.real4vectorsarray_inputs)
					{
						vector_array_argument.start_shader_register = e.register_index;
						vector_array_argument.array.resize(e.length);

						p_shaderInfos.addVectorArrayArgument(vector_array_argument);

						m_shadersCache_mutex.lock();
						m_shadersCache.at(resourceUID).vectorarray_arguments.push_back(vector_array_argument);
						m_shadersCache_mutex.unlock();
					}
					m_jsonparser_mutex.unlock();

					if (metadataParseStatus != JS::Error::NoError)
					{
						const auto errorStr{ parseContext.makeErrorString() };
						_EXCEPTION("JSON parse error on " + shader_metadata_path + " : " + errorStr);
					}

					////////////////////////////////

					_MAGE_DEBUG(m_localLoggerRunner, std::string("task has loaded shader ") + p_shaderInfos.getSourceID() + ", resource uid = " + p_shaderInfos.getResourceUID());

					ResourceStateControler::getInstance()->update(p_shaderInfos, Shader::State::BLOBLOADED);

					m_shadersCache_mutex.lock();
					m_shadersCache.at(resourceUID).state = ShaderCacheEntry::State::BLOBLOADED;
					m_shadersCache_mutex.unlock();

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

		m_runner[m_runnerIndex].get()->m_mailbox_in.push(task);

		m_runnerIndex++;
		if (m_runnerIndex == nbRunners)
		{
			m_runnerIndex = 0;
		}
	}
	else
	{
		m_shadersCache_mutex.lock();
		const auto shader_state{ m_shadersCache.at(resourceUID).state };
		m_shadersCache_mutex.unlock();

		if (ShaderCacheEntry::State::BLOBLOADED == shader_state)
		{
			p_shaderInfos.setFileContent(m_shadersCache.at(resourceUID).shader_source.c_str(), m_shadersCache.at(resourceUID).shader_source.size());

			p_shaderInfos.setCode(m_shadersCache.at(resourceUID).shader_code.getData(), m_shadersCache.at(resourceUID).shader_code.getDataSize());

			for (const auto& e : m_shadersCache.at(resourceUID).generic_arguments)
			{
				p_shaderInfos.addGenericArgument(e);
			}

			for (const auto& e : m_shadersCache.at(resourceUID).vectorarray_arguments)
			{
				p_shaderInfos.addVectorArrayArgument(e);
			}

			ResourceStateControler::getInstance()->update(p_shaderInfos, Shader::State::BLOBLOADED);
		}
	}
}
