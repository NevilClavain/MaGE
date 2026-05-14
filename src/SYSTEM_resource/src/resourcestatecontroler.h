
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

#include "singleton.h"

#include "shader.h"
#include "texture.h"
#include "trianglemeshe.h"
#include "linemeshe.h"
#include <functional>
#include <vector>

namespace mage
{
    class ResourceStateControler : public property::Singleton<ResourceStateControler>
    {
    public:

        using ShaderCallback = std::function<void(Shader*, Shader::State)>;
        using TriangleMesheCallback = std::function<void(TriangleMeshe*, TriangleMeshe::State)>;
        using LineMesheCallback = std::function<void(LineMeshe*, LineMeshe::State)>;
        using TextureCallback = std::function<void(Texture*, Texture::State)>;

        void registerSubscriber(const ShaderCallback& p_callback);
        void registerSubscriber(const TriangleMesheCallback& p_callback);
        void registerSubscriber(const LineMesheCallback& p_callback);
        void registerSubscriber(const TextureCallback& p_callback);

        void update(Shader& p_resource, Shader::State p_state);
        void update(TriangleMeshe& p_resource, TriangleMeshe::State p_state);
        void update(LineMeshe& p_resource, LineMeshe::State p_state);
        void update(Texture& p_resource, Texture::State p_state);

    private:
        std::vector<ShaderCallback> m_shader_callbacks;
        std::vector<TriangleMesheCallback> m_trianglemeshe_callbacks;
        std::vector<LineMesheCallback> m_linemeshe_callbacks;
        std::vector<TextureCallback> m_texture_callbacks;
    };
};
