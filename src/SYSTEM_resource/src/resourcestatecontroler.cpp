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

#include "resourcestatecontroler.h"

using namespace mage;

void ResourceStateControler::registerSubscriber(const ShaderCallback& p_callback)
{
    m_shader_callbacks.push_back(p_callback);
}

void ResourceStateControler::registerSubscriber(const TriangleMesheCallback& p_callback)
{
    m_trianglemeshe_callbacks.push_back(p_callback);
}

void ResourceStateControler::registerSubscriber(const LineMesheCallback& p_callback)
{
    m_linemeshe_callbacks.push_back(p_callback);
}

void ResourceStateControler::registerSubscriber(const TextureCallback& p_callback)
{
    m_texture_callbacks.push_back(p_callback);
}

void ResourceStateControler::update(Shader& p_resource, Shader::State p_state)
{
    p_resource.setState(p_state);

    for (const auto& call : m_shader_callbacks)
    {
        call(&p_resource, p_state);
    }
}

void ResourceStateControler::update(TriangleMeshe& p_resource, TriangleMeshe::State p_state)
{
    p_resource.setState(p_state);

    for (const auto& call : m_trianglemeshe_callbacks)
    {
        call(&p_resource, p_state);
    }
}

void ResourceStateControler::update(LineMeshe& p_resource, LineMeshe::State p_state)
{
    p_resource.setState(p_state);

    for (const auto& call : m_linemeshe_callbacks)
    {
        call(&p_resource, p_state);
    }
}

void ResourceStateControler::update(Texture& p_resource, Texture::State p_state)
{
    p_resource.setState(p_state);

    for (const auto& call : m_texture_callbacks)
    {
        call(&p_resource, p_state);
    }
}
