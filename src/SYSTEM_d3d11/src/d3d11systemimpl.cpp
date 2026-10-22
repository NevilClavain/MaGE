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
#include "aspects.h"

#include <wincodec.h>

#include <ScreenGrab.h>
#include <algorithm>
#include <locale>
#include <codecvt>

D3D11SystemImpl::D3D11SystemImpl() :
m_localLogger("D3D11System", mage::core::logger::Configuration::getInstance())
{
}

mage::core::logger::Sink& D3D11SystemImpl::logger()
{
	return m_localLogger;
}

DirectX::XMFLOAT4X4 D3D11SystemImpl::convertMatrixToXMFloat44(const mage::core::maths::Matrix& p_mat)
{
	DirectX::XMFLOAT4X4 xm_mat;

    xm_mat._11 = p_mat(0, 0);
    xm_mat._12 = p_mat(0, 1);
    xm_mat._13 = p_mat(0, 2);
    xm_mat._14 = p_mat(0, 3);

    xm_mat._21 = p_mat(1, 0);
    xm_mat._22 = p_mat(1, 1);
    xm_mat._23 = p_mat(1, 2);
    xm_mat._24 = p_mat(1, 3);

    xm_mat._31 = p_mat(2, 0);
    xm_mat._32 = p_mat(2, 1);
    xm_mat._33 = p_mat(2, 2);
    xm_mat._34 = p_mat(2, 3);

    xm_mat._41 = p_mat(3, 0);
    xm_mat._42 = p_mat(3, 1);
    xm_mat._43 = p_mat(3, 2);
    xm_mat._44 = p_mat(3, 3);

	return xm_mat;
}

static void saveRenderingBufferToFile(const std::string& p_filename, ID3D11DeviceContext* p_context, ID3D11Texture2D* stagingTexture)
{
	// Determiner le format d'export basé sur l'extension du fichier
	std::string filename_lower = p_filename;
	std::transform(filename_lower.begin(), filename_lower.end(), filename_lower.begin(), ::tolower);

	// Convertir std::string en std::wstring pour DirectX
	std::wstring filename_wide(filename_lower.begin(), filename_lower.end());

	HRESULT hr = S_OK;

	try
	{
		if (filename_lower.find(".dds") != std::string::npos)
		{
			// Exporter en DDS
			hr = DirectX::SaveDDSTextureToFile(p_context, static_cast<ID3D11Resource*>(stagingTexture), filename_wide.c_str());
			if (!SUCCEEDED(hr))
			{
				_EXCEPTION("dumpRenderingBuffer : failed to save DDS file");
			}
		}
		else if (filename_lower.find(".png") != std::string::npos)
		{
			// Exporter en PNG
			hr = DirectX::SaveWICTextureToFile(p_context, static_cast<ID3D11Resource*>(stagingTexture), GUID_ContainerFormatPng, filename_wide.c_str());
			if (!SUCCEEDED(hr))
			{
				_EXCEPTION("dumpRenderingBuffer : failed to save PNG file");
			}
		}
		else if (filename_lower.find(".jpg") != std::string::npos || filename_lower.find(".jpeg") != std::string::npos)
		{
			// Exporter en JPEG
			hr = DirectX::SaveWICTextureToFile(p_context, static_cast<ID3D11Resource*>(stagingTexture), GUID_ContainerFormatJpeg, filename_wide.c_str());
			if (!SUCCEEDED(hr))
			{
				_EXCEPTION("dumpRenderingBuffer : failed to save JPEG file");
			}
		}
		else
		{
			// Format par défaut : DDS
			hr = DirectX::SaveDDSTextureToFile(p_context, static_cast<ID3D11Resource*>(stagingTexture), filename_wide.c_str());
			if (!SUCCEEDED(hr))
			{
				_EXCEPTION("dumpRenderingBuffer : failed to save file (default DDS format)");
			}
		}
	}
	catch (const std::exception& e)
	{
		_EXCEPTION(std::string("dumpRenderingBuffer : exception during file save : ") + e.what());
	}

}

void D3D11SystemImpl::dumpRenderingBuffer(const std::string& p_texture_id, const std::string& p_filename) const
{
	if(m_textures.find(p_texture_id) == m_textures.end())
	{
		_EXCEPTION("D3D11SystemImpl::dumpRenderingBuffer : texture id not found : " + p_texture_id);
		return;
	}

	const TextureData& textureData{ m_textures.at(p_texture_id) };

	HRESULT hr = S_OK;

	// Créer une texture temporaire en staging pour la copie CPU
	D3D11_TEXTURE2D_DESC stagingDesc = textureData.desc;
	stagingDesc.Usage = D3D11_USAGE_STAGING;
	stagingDesc.BindFlags = 0;
	stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	stagingDesc.MiscFlags = 0;

	ID3D11Texture2D* stagingTexture = nullptr;
	hr = m_lpd3ddevice->CreateTexture2D(&stagingDesc, nullptr, &stagingTexture);
	if (FAILED(hr))
	{
		_EXCEPTION("dumpRenderingBuffer : failed to create staging texture");
		return;
	}

	// Copier la texture GPU vers la staging texture
	ID3D11Resource* srcResource = nullptr;

	if (textureData.source == mage::Texture::Source::CONTENT_FROM_FILE)
	{
		// Pour les textures chargées depuis des fichiers
		srcResource = textureData.textureResource;
	}
	else if (textureData.source == mage::Texture::Source::CONTENT_FROM_RENDERINGQUEUE)
	{
		// Pour les render targets
		srcResource = textureData.targetTexture;
	}
	else
	{
		stagingTexture->Release();
		_EXCEPTION("dumpRenderingBuffer : unsupported texture source type");
		return;
	}

	if (!srcResource)
	{
		stagingTexture->Release();
		_EXCEPTION("dumpRenderingBuffer : source resource is null");
		return;
	}

	// Copier la texture du GPU au CPU via la staging texture
	m_lpd3ddevcontext->CopyResource(stagingTexture, srcResource);

	// Mapper la staging texture pour accéder aux données
	D3D11_MAPPED_SUBRESOURCE mappedResource = {};
	hr = m_lpd3ddevcontext->Map(stagingTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
	if (FAILED(hr))
	{
		stagingTexture->Release();
		_EXCEPTION("dumpRenderingBuffer : failed to map staging texture");
		return;
	}

	
	D3D11_TEXTURE2D_DESC desc2;
	stagingTexture->GetDesc(&desc2);
	DXGI_FORMAT format = desc2.Format;

	if(desc2.Format == DXGI_FORMAT_B8G8R8A8_UNORM)
	{
		saveRenderingBufferToFile(p_filename, m_lpd3ddevcontext, stagingTexture);
	}
	else if (desc2.Format == DXGI_FORMAT_R32_FLOAT)
	{

		// Calcul min/max
		float minValue = FLT_MAX;
		float maxValue = -FLT_MAX;

		for (UINT y = 0; y < desc2.Height; ++y)
		{
			float* row = reinterpret_cast<float*>(
				static_cast<uint8_t*>(mappedResource.pData) + y * mappedResource.RowPitch);

			for (UINT x = 0; x < desc2.Width; ++x)
			{
				float v = row[x];

				minValue = std::min(minValue, v);
				maxValue = std::max(maxValue, v);
			}
		}

		// Evite division par zéro
		float range = maxValue - minValue;
		if (range < 1e-20f)
		{
			range = 1.0f;
		}

		// Buffer RGBA8
		std::vector<uint8_t> rgba(desc2.Width * desc2.Height * 4);

		for (UINT y = 0; y < desc2.Height; ++y)
		{
			float* srcRow = reinterpret_cast<float*>(
				static_cast<uint8_t*>(mappedResource.pData) + y * mappedResource.RowPitch);

			for (UINT x = 0; x < desc2.Width; ++x)
			{
				float v = srcRow[x];

				float n = (v - minValue) / range;

				n = std::max(0.0f, std::min(1.0f, n));

				uint8_t gray = static_cast<uint8_t>(n * 255.0f);

				size_t dst = (y * desc2.Width + x) * 4;

				rgba[dst + 0] = gray; // R
				rgba[dst + 1] = gray; // G
				rgba[dst + 2] = gray; // B
				rgba[dst + 3] = 255;  // A
			}
		}

		// create intermediate RGB texure of grayscale values
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = desc2.Width;
		texDesc.Height = desc2.Height;
		texDesc.MipLevels = 1;
		texDesc.ArraySize = 1;
		texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		texDesc.SampleDesc.Count = 1;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = 0;

		D3D11_SUBRESOURCE_DATA initData = {};
		initData.pSysMem = rgba.data();
		initData.SysMemPitch = desc2.Width * 4;

		ID3D11Texture2D* grayTexture = nullptr;

		hr = m_lpd3ddevice->CreateTexture2D(
			&texDesc,
			&initData,
			&grayTexture);

		if (FAILED(hr))
		{
			_EXCEPTION("CreateTexture2D failed");
		}

		saveRenderingBufferToFile(p_filename, m_lpd3ddevcontext, grayTexture);
	}
	else
	{
		m_lpd3ddevcontext->Unmap(stagingTexture, 0);
		stagingTexture->Release();
		_EXCEPTION("dumpRenderingBuffer : unsupported texture format for dumping : " + std::to_string(desc2.Format));
		return;
	}
	
	// Nettoyer les ressources
	m_lpd3ddevcontext->Unmap(stagingTexture, 0);
	stagingTexture->Release();
}