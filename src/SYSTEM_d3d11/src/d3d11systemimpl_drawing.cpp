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

#include "d3d11systemimpl.h"
#include "matrixchain.h"


using namespace mage::core::maths;
using namespace mage::transform;


void D3D11SystemImpl::bindShadersConstantBuffers(const mage::core::maths::Matrix& p_view,
                                                    const mage::core::maths::Matrix& p_proj,
                                                    const mage::core::maths::Matrix& p_secondary_view,
                                                    const mage::core::maths::Matrix& p_secondary_proj)
{
    auto view{ p_view };
    view.transpose();

    setVertexshaderConstantsMat(12, view);
    setPixelshaderConstantsMat(12, view);

    //////////////////////////////////////////////////////////////////////

    auto cam{ p_view };
    cam.inverse();
    cam.transpose();

    setVertexshaderConstantsMat(16, cam);
    setPixelshaderConstantsMat(16, cam);

    auto proj{ p_proj };

    proj.transpose();
    setVertexshaderConstantsMat(20, proj);
    setPixelshaderConstantsMat(20, proj);

    //////////////////////////////////////////////////////////////////////
    // for secondary view
    //////////////////////////////////////////////////////////////////////


    auto secondary_view{ p_secondary_view };
    secondary_view.transpose();

    setPixelshaderConstantsMat(32, secondary_view);
    setVertexshaderConstantsMat(32, secondary_view);


    auto secondary_cam{ p_secondary_view };
    secondary_cam.inverse();

    secondary_cam.transpose();

    setPixelshaderConstantsMat(36, secondary_cam);
    setVertexshaderConstantsMat(36, secondary_cam);

    auto secondary_proj{ p_secondary_proj };

    secondary_proj.transpose();

    setPixelshaderConstantsMat(40, secondary_proj);
    setVertexshaderConstantsMat(40, secondary_proj);

    ///////////////////////////////////////////////////////////////////////
    // update des shaders constants buffers...

    D3D11_MAPPED_SUBRESOURCE mapped = {};

    auto hRes = m_lpd3ddevcontext->Map(m_vertexShaderArgsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &m_vertexshader_args, sizeof(m_vertexshader_args));
    m_lpd3ddevcontext->Unmap(m_vertexShaderArgsBuffer, 0);

    hRes = m_lpd3ddevcontext->Map(m_pixelShaderArgsBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    memcpy(mapped.pData, &m_pixelshader_args, sizeof(m_pixelshader_args));
    m_lpd3ddevcontext->Unmap(m_pixelShaderArgsBuffer, 0);

    ///////////////

    m_lpd3ddevcontext->VSSetConstantBuffers(0, 1, &m_vertexShaderArgsBuffer);
    m_lpd3ddevcontext->PSSetConstantBuffers(0, 1, &m_pixelShaderArgsBuffer);
}

void D3D11SystemImpl::drawIndexedInstancedLines(int p_instances_count)
{
    m_lpd3ddevcontext->DrawIndexedInstanced(m_next_nblines * 2, p_instances_count, 0, 0, 0);
}

void D3D11SystemImpl::drawIndexedInstancedTriangles(int p_instances_count)
{
    m_lpd3ddevcontext->DrawIndexedInstanced(m_next_nbtriangles * 3, p_instances_count, 0, 0, 0);
}

void D3D11SystemImpl::beginScreen()
{
    m_currentTarget = m_screentarget;
    m_currentView = m_pDepthStencilView;

    m_lpd3ddevcontext->OMSetRenderTargets(1, &m_currentTarget, m_currentView);
    m_lpd3ddevcontext->RSSetViewports(1, &m_mainScreenViewport);
}

void D3D11SystemImpl::beginTarget(const std::string& p_targetName)
{
    if (m_textures.count(p_targetName))
    {
        const auto ti{ m_textures.at(p_targetName) };

        m_currentTarget = ti.rendertextureTargetView;
        m_currentView = ti.stencilDepthView;

        m_lpd3ddevcontext->OMSetRenderTargets(1, &m_currentTarget, m_currentView);
        m_lpd3ddevcontext->RSSetViewports(1, &ti.viewport);
    }
    else
    {
        _EXCEPTION_("Target texture not found : " + p_targetName);
    }
}

void D3D11SystemImpl::clearTarget(const mage::core::maths::RGBAColor& p_clear_color)
{
    FLOAT clearcolor[4];

    clearcolor[0] = p_clear_color.r() / 255.0f;
    clearcolor[1] = p_clear_color.g() / 255.0f;
    clearcolor[2] = p_clear_color.b() / 255.0f;
    clearcolor[3] = p_clear_color.a() / 255.0f;

    if (m_currentTarget) 
    {
        m_lpd3ddevcontext->ClearRenderTargetView(m_currentTarget, clearcolor);
    }    
}

void D3D11SystemImpl::clearTargetDepth()
{
    if (m_currentView)
    {
        m_lpd3ddevcontext->ClearDepthStencilView(m_currentView, D3D11_CLEAR_DEPTH, 1.0, 0);
    }    
}

void D3D11SystemImpl::flipScreen(void)
{
    m_lpd3dswapchain->Present(0, 0);
}

void D3D11SystemImpl::drawText(const std::string& p_font, const mage::core::maths::RGBAColor& p_clear_color, const mage::core::maths::IntCoords2D& p_pos, float p_rotation, const std::string& p_text)
{
    const auto fontData{ m_fontWrappers.at(p_font) };

    const auto spriteBatch{ fontData.spriteBatch.get() };
    const auto spriteFont{ fontData.spriteFont.get() };

    const DirectX::XMFLOAT2 pos{ (float)p_pos.x(), (float)p_pos.y() };

    const DirectX::FXMVECTOR color{ p_clear_color.r() / 255.0f, p_clear_color.g() / 255.0f, p_clear_color.b() / 255.0f, p_clear_color.a() / 255.0f };
    
    spriteBatch->Begin();
    spriteFont->DrawString(spriteBatch, p_text.c_str(), pos, color, p_rotation);
    spriteBatch->End();
}