
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

#pragma once

#include <unordered_set>
#include "system.h"
#include "shaders_service.h"
#include "textures_service.h"
#include "runner.h"
#include "eventsource.h"



namespace mage
{   
    // fwd decls
    namespace core { class Entity; }
    namespace core { class Entitygraph; }
    namespace rendering { struct Queue; }
    class Shader;
    class LineMeshe;
    class TriangleMeshe;
    class Texture;


    enum class D3D11SystemEvent
    {
        D3D11_WINDOW_READY,
        D3D11_SHADER_CREATION_BEGIN,
        D3D11_SHADER_CREATION_SUCCESS,
        D3D11_SHADER_RELEASE_BEGIN,
        D3D11_SHADER_RELEASE_SUCCESS,
        D3D11_LINEMESHE_CREATION_BEGIN,
        D3D11_LINEMESHE_CREATION_SUCCESS,
        D3D11_LINEMESHE_RELEASE_BEGIN,
        D3D11_LINEMESHE_RELEASE_SUCCESS,
        D3D11_TRIANGLEMESHE_CREATION_BEGIN,
        D3D11_TRIANGLEMESHE_CREATION_SUCCESS,
        D3D11_TRIANGLEMESHE_RELEASE_BEGIN,
        D3D11_TRIANGLEMESHE_RELEASE_SUCCESS,
        D3D11_TEXTURE_CREATION_BEGIN,
        D3D11_TEXTURE_CREATION_SUCCESS,
        D3D11_TEXTURE_RELEASE_BEGIN,
        D3D11_TEXTURE_RELEASE_SUCCESS
    };
               
    class D3D11System : public core::System, public mage::property::EventSource<D3D11SystemEvent, const std::string&>
    {
    public:

        D3D11System(core::Entitygraph& p_entitygraph);
        ~D3D11System() = default;

        void run();
        void killRunner();

        auto getShaderCompilationInvocationCallback() const
        {
            return m_shadercompilation_invocation_cb;
        };

        auto getTextureContentCopyInvocationCallback() const
        {
            return m_texturecontentcopy_invocation_cb;
        };

    private:
        bool	                                                m_initialized{ false };
        core::services::ShadersCompilationService::Callback     m_shadercompilation_invocation_cb;
        core::services::TextureContentCopyService::Callback     m_texturecontentcopy_invocation_cb;

        mage::core::Runner                                      m_runner;

        void    manageInitialization();       
        void    manageResources();
        void    manageRenderingQueue();
        void    collectWorldTransformations() const;

        void    handleShaderCreation(Shader& p_shaderInfos, int p_shaderType);
        void    handleShaderRelease(Shader& p_shaderInfos, int p_shaderType);

        void    handleLinemesheCreation(LineMeshe& p_lm);
        void    handleLinemesheRelease(LineMeshe& p_lm);

        void    handleTrianglemesheCreation(TriangleMeshe& p_tm);
        void    handleTrianglemesheRelease(TriangleMeshe& p_tm);

        void    handleTextureCreation(Texture& p_texture);

        void    renderQueue(const rendering::Queue& p_renderingQueue) const;

        void    handleRenderingQueuesState(core::Entity* p_entity, rendering::Queue& p_renderingQueue );
        
    };
}
