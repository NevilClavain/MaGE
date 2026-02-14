
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

#pragma once

#include <mutex>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

#include <json_struct/json_struct.h>


#include "logsink.h"
#include "logconf.h"
#include "logging.h"


#include "system.h"
#include "runner.h"
#include "eventsource.h"
#include "buffer.h"
#include "matrix.h"
#include "component.h"

#include "buffer.h"

namespace mage
{
    // fwd decls
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
    class Shader;
    class Texture;
    class TriangleMeshe;
    struct SceneNode;

    enum class ResourceSystemEvent
    {
        RESOURCE_SHADER_CACHE_CREATED,
        RESOURCE_SHADER_COMPILATION_BEGIN,
        RESOURCE_SHADER_COMPILATION_SUCCESS,
        RESOURCE_SHADER_COMPILATION_ERROR,
        RESOURCE_SHADER_LOAD_BEGIN,
        RESOURCE_SHADER_LOAD_SUCCESS,

        RESOURCE_TEXTURE_LOAD_BEGIN,
        RESOURCE_TEXTURE_LOAD_SUCCESS,

        RESOURCE_MESHE_LOAD_BEGIN,
        RESOURCE_MESHE_LOAD_SUCCESS

    };

    namespace json
    {
        struct ShadersReal4VectorInput
        {
            std::string argument_id;
            int         register_index;
            std::string description;

            JS_OBJ(argument_id, register_index, description);
        };

        struct ShadersReal4VectorsArrayInput
        {
            int         length;
            int         register_index;
            std::string description;

            JS_OBJ(length, register_index, description);
        };

        struct ShaderMetadata
        { 
            std::vector<ShadersReal4VectorInput>        real4vector_inputs;
            std::vector< ShadersReal4VectorsArrayInput> real4vectorsarray_inputs;

            JS_OBJ(real4vector_inputs, real4vectorsarray_inputs);
        };
    }

    class ResourceSystem : public core::System, public mage::property::EventSource<ResourceSystemEvent, const std::string&>
    {
    public:

        ResourceSystem() = delete;
        ResourceSystem(core::Entitygraph& p_entitygraph);
        ~ResourceSystem();

        void run();
        void killRunner();

        size_t getNbBusyRunners() const;

    private:
        mage::core::logger::Sink                                        m_localLogger;
        mage::core::logger::Sink                                        m_localLoggerRunner;
        const std::string                                               m_shadersBasePath{ "./shaders/resources" };
        const std::string                                               m_texturesBasePath{ "./textures" };
        const std::string                                               m_meshesBasePath{ "./meshes" };
        const std::string                                               m_shadersCachePath{ "./bc_cache" };

        //mage::core::Json<Shader>::Callback	                        m_jsonparser_cb;
        std::mutex                                                      m_jsonparser_mutex;

        static constexpr unsigned int                                   nbRunners{ 2 };

        std::vector<std::unique_ptr<mage::core::Runner>>                m_runner;
        int                                                             m_runnerIndex{ 0 };

        bool                                                            m_forceAllShadersRegeneration{ false };

        std::mutex	                                                    m_texturesBlobCache_mutex;
        std::unordered_map<std::string, core::Buffer<unsigned char>>    m_texturesBlobCache;
       
        void handleShader(const std::string& p_filename, Shader& p_shaderInfos);
        void handleTexture(const std::string& p_filename, Texture& p_textureInfos );
        
        void handleSceneFile(const std::string& p_filename,
                            const std::string& p_mesheid, TriangleMeshe& p_mesheInfos,
                            const core::ComponentList<std::map<std::string, SceneNode>>& p_nodes_hierarchy_list);
    };
}
