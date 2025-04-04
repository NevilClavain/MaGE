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

#include "datacloud.h"
#include "d3d11systemimpl.h"
#include "aspects.h"

#include "logsink.h"
#include "logconf.h"
#include "logging.h"


bool D3D11SystemImpl::init(mage::core::Entity* p_mainWindow)
{
	DECLARE_D3D11ASSERT_VARS

	_MAGE_DEBUG(m_localLogger, std::string("init D3D startup"))

	DXGI_SWAP_CHAIN_DESC swap_chain;
	ZeroMemory(&swap_chain, sizeof(swap_chain));

	const auto dataCloud{ mage::rendering::Datacloud::getInstance() };

	//get main windows infos
	auto& mainwindows_rendering_aspect{ p_mainWindow->aspectAccess(mage::core::renderingAspect::id) };

	int characteristics_width_resol{ 0 };
	int characteristics_height_resol{ 0 };
	float characteristics_v_width, characteristics_v_height;

	RECT rect{ 0 }; // /!\ valid on ly if !fullscreen

	const auto windowHWND{ mainwindows_rendering_aspect.getComponent<HWND>("eg.std.windowHWND")->getPurpose() };

	const auto fullscreen{ mainwindows_rendering_aspect.getComponent<bool>("eg.std.fullscreen")->getPurpose() };
	if (fullscreen)
	{
		int         fullscreen_width;
		int         fullscreen_height;
		DXGI_FORMAT fullscreen_format;
		int         fullscreen_refresh_rate_num;
		int         fullscreen_refresh_rate_den;

		fullscreenAutosetDesktopResolution(fullscreen_width, fullscreen_height, fullscreen_format, fullscreen_refresh_rate_num, fullscreen_refresh_rate_den);

		characteristics_width_resol = fullscreen_width;
		characteristics_height_resol = fullscreen_height;

		characteristics_v_width = 1.0;
		characteristics_v_height = characteristics_v_width * fullscreen_height / fullscreen_width;

		_MAGE_TRACE(m_localLogger, std::string("full screen resol : ") + std::to_string(fullscreen_width) + "x" + std::to_string(fullscreen_height))

		swap_chain.BufferDesc.Format = fullscreen_format;
		swap_chain.BufferDesc.RefreshRate.Numerator = fullscreen_refresh_rate_num;
		swap_chain.BufferDesc.RefreshRate.Denominator = fullscreen_refresh_rate_den;

		swap_chain.Windowed = FALSE;
	}
	else
	{
		RECT rect;
		GetClientRect(windowHWND, &rect);

		characteristics_width_resol = rect.right - rect.left;
		characteristics_height_resol = rect.bottom - rect.top;

		characteristics_v_width = 1.0;
		characteristics_v_height = characteristics_v_width * characteristics_height_resol / characteristics_width_resol;

		swap_chain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swap_chain.BufferDesc.RefreshRate.Numerator = 60;
		swap_chain.BufferDesc.RefreshRate.Denominator = 1;

		swap_chain.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swap_chain.Windowed = TRUE;
	}


	_MAGE_TRACE(m_localLogger, std::string("renderer characteristics : width_resol = ") + std::to_string(characteristics_width_resol) +
		std::string(" height_resol = ") + std::to_string(characteristics_height_resol) +
		std::string(" v_width = ") + std::to_string(characteristics_v_width) +
		std::string(" v_height = ") + std::to_string(characteristics_v_height))


	// complete main window entity with renderer characteristics
	mainwindows_rendering_aspect.addComponent<int>("eg.std.widthResol", characteristics_width_resol);
	mainwindows_rendering_aspect.addComponent<int>("eg.std.heightResol", characteristics_height_resol);
	mainwindows_rendering_aspect.addComponent<float>("eg.std.viewportWidth", characteristics_v_width);
	mainwindows_rendering_aspect.addComponent<float>("eg.std.viewportHeight", characteristics_v_height);

	dataCloud->registerData<mage::core::maths::IntCoords2D>("std.window_resol");
	dataCloud->updateDataValue<mage::core::maths::IntCoords2D>("std.window_resol", mage::core::maths::IntCoords2D(characteristics_width_resol, characteristics_height_resol));

	dataCloud->registerData<mage::core::maths::FloatCoords2D>("std.viewport");
	dataCloud->updateDataValue<mage::core::maths::FloatCoords2D>("std.viewport", mage::core::maths::FloatCoords2D(characteristics_v_width, characteristics_v_height));


	swap_chain.BufferDesc.Width = characteristics_width_resol;
	swap_chain.BufferDesc.Height = characteristics_height_resol;
	swap_chain.BufferCount = 1;
	swap_chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain.OutputWindow = windowHWND;
	swap_chain.SampleDesc.Count = 1; // de 1 a 4 en D3D11
	swap_chain.SampleDesc.Quality = 0; // en low quality


	static const std::vector<std::pair<D3D_DRIVER_TYPE, std::string>> driver_type =
	{
		{ D3D_DRIVER_TYPE_HARDWARE, "HARDWARE"},
		{ D3D_DRIVER_TYPE_WARP, "WARP"},
		{ D3D_DRIVER_TYPE_REFERENCE, "REF"},
		{ D3D_DRIVER_TYPE_SOFTWARE, "SOFTWARE"}
	};

	IDXGISwapChain* lpd3dswapchain{ nullptr };
	ID3D11Device* lpd3ddevice{ nullptr };
	ID3D11DeviceContext* lpd3ddevcontext{ nullptr };

	std::string				driver_descr;

	UINT createDeviceFlags{ 0 };

	// not accepted by D3D11CreateDeviceAndSwapChain (wtf ?)

#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	HRESULT r;
	for (auto& e : driver_type)
	{
		r = D3D11CreateDeviceAndSwapChain(nullptr, e.first,
			nullptr,
			createDeviceFlags,
			nullptr,
			0,
			D3D11_SDK_VERSION,
			&swap_chain,
			&lpd3dswapchain,
			&lpd3ddevice,
			nullptr,
			&lpd3ddevcontext);

		if (r == S_OK)
		{
			driver_descr = e.second;

			_MAGE_TRACE(m_localLogger, "D3D11CreateDeviceAndSwapChain is OK for " + driver_descr)
			break;
		}
		else
		{
			_MAGE_WARN(m_localLogger, "D3D11CreateDeviceAndSwapChain is KO for " + driver_descr + ", switching to next")
		}
	}

	hRes = r;
	D3D11_CHECK(D3D11CreateDeviceAndSwapChain);

	mainwindows_rendering_aspect.addComponent<std::string>("eg.std.d3d11DriverDescr", driver_descr);

	m_lpd3ddevcontext = lpd3ddevcontext;
	m_lpd3ddevice = lpd3ddevice;
	m_lpd3dswapchain = lpd3dswapchain;

	if (fullscreen)
	{
		lpd3dswapchain->SetFullscreenState(TRUE, nullptr);
	}

	///////////////////////////////////////////////////////////////////////
	// get device description

	IDXGIDevice* dxgiDevice{ nullptr };
	hRes = m_lpd3ddevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	D3D11_CHECK(QueryInterface);

	
	hRes = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&m_dxgiAdapter);
	D3D11_CHECK(dxgiDeviceGetParent);

	dxgiDevice->Release();

	// GPU name

	m_dxgiAdapter->GetDesc(&m_adapterDescription);

	dataCloud->registerData<std::string>("std.gpu");
	const std::wstring w_gpu_description(m_adapterDescription.Description);
	const std::string gpu_description(w_gpu_description.begin(), w_gpu_description.end());
	dataCloud->updateDataValue<std::string>("std.gpu", gpu_description);

	mainwindows_rendering_aspect.addComponent<std::string>("eg.std.gpuName", gpu_description);

	// GPU driver version

	LARGE_INTEGER i;	
	m_dxgiAdapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &i);

	const auto a{ i.QuadPart >> 48 };
	const auto b{ (i.QuadPart >> 32) & 0xFFFF };
	const auto c{ (i.QuadPart >> 16) & 0xFFFF };
	const auto d{ i.QuadPart & 0xFFFF };

	std::string driver_version{ std::to_string(a) + "." + std::to_string(b) + "." + std::to_string(c) + "." + std::to_string(d) };
	dataCloud->registerData<std::string>("std.gpu_driver");
	dataCloud->updateDataValue<std::string>("std.gpu_driver", driver_version);

	mainwindows_rendering_aspect.addComponent<std::string>("eg.std.gpuDriver", driver_version);

	///////////////////////////////////////////////////////////////////////

	ID3D11Texture2D* backBuffer;
	hRes = lpd3dswapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	D3D11_CHECK(GetBuffer);

	ID3D11RenderTargetView* screentarget;

	hRes = lpd3ddevice->CreateRenderTargetView(backBuffer, nullptr, &screentarget);
	D3D11_CHECK(CreateRenderTargetView);

	backBuffer->Release();

	m_screentarget = screentarget;

	///////////////////////////////////////////////////////////////////////

	D3D11_DEPTH_STENCIL_DESC dsDesc;
	ZeroMemory(&dsDesc, sizeof(dsDesc));

	// Depth test parameters
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;

	// Stencil test parameters
	dsDesc.StencilEnable = FALSE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* dsState_DepthTestDisabled{ nullptr };
	ID3D11DepthStencilState* dsState_DepthTestEnabled{ nullptr };


	hRes = lpd3ddevice->CreateDepthStencilState(&dsDesc, &dsState_DepthTestDisabled);
	D3D11_CHECK(CreateDepthStencilState);

	dsDesc.DepthEnable = TRUE;

	hRes = lpd3ddevice->CreateDepthStencilState(&dsDesc, &dsState_DepthTestEnabled);
	D3D11_CHECK(CreateDepthStencilState);

	// dans D3D9 plugin, zbuffer est activ� par d�faut (cf doc)
	// donc idem ici
	lpd3ddevcontext->OMSetDepthStencilState(dsState_DepthTestEnabled, 1);

	m_dsState_DepthTestEnabled = dsState_DepthTestEnabled;
	m_dsState_DepthTestDisabled = dsState_DepthTestDisabled;

	ID3D11Texture2D* pDepthStencil{ nullptr };
	ID3D11DepthStencilView* pDepthStencilView{ nullptr };

	if (!createDepthStencilBuffer(lpd3ddevice, characteristics_width_resol, characteristics_height_resol,
		DXGI_FORMAT_D24_UNORM_S8_UINT, &pDepthStencil, &pDepthStencilView))
	{
		return false;
	}

	m_pDepthStencil = pDepthStencil;
	m_pDepthStencilView = pDepthStencilView;

	///////////////////////////////////////////////////

	const auto fonts{ mainwindows_rendering_aspect.getComponent<std::vector<std::string>>("eg.std.fonts")->getPurpose() };
	for (const auto& fontname : fonts)
	{
		const std::wstring wfontname(fontname.begin(), fontname.end());

		const std::wstring wfontPath{ L"./fonts/" + wfontname };

		const auto spriteBatch{ std::make_shared<DirectX::SpriteBatch>(m_lpd3ddevcontext) };
		const auto spriteFont{ std::make_shared<DirectX::SpriteFont>(m_lpd3ddevice, wfontPath.c_str())};

		FontRenderingData fontData{ spriteBatch, spriteFont };

		m_fontWrappers[fontname] = fontData;
	}

	//////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));


	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;

	ID3D11SamplerState* linearFilterSamplerState{ nullptr };
	ID3D11SamplerState* pointFilterSamplerState{ nullptr };
	ID3D11SamplerState* anisotropicFilterSamplerState{ nullptr };

	ID3D11SamplerState* linearFilterSamplerState_uvwrap{ nullptr };
	ID3D11SamplerState* pointFilterSamplerState_uvwrap{ nullptr };
	ID3D11SamplerState* anisotropicFilterSamplerState_uvwrap{ nullptr };


	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &pointFilterSamplerState);
	D3D11_CHECK(CreateSamplerState);

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &pointFilterSamplerState_uvwrap);
	D3D11_CHECK(CreateSamplerState);

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &linearFilterSamplerState);
	D3D11_CHECK(CreateSamplerState);

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &linearFilterSamplerState_uvwrap);
	D3D11_CHECK(CreateSamplerState);

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &anisotropicFilterSamplerState);
	D3D11_CHECK(CreateSamplerState);

	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	hRes = lpd3ddevice->CreateSamplerState(&sampDesc, &anisotropicFilterSamplerState_uvwrap);
	D3D11_CHECK(CreateSamplerState);

	m_linearFilterSamplerState = linearFilterSamplerState;
	m_pointFilterSamplerState = pointFilterSamplerState;
	m_anisotropicFilterSamplerState = anisotropicFilterSamplerState;

	m_linearFilterSamplerState_uvwrap = linearFilterSamplerState_uvwrap;
	m_pointFilterSamplerState_uvwrap = pointFilterSamplerState_uvwrap;
	m_anisotropicFilterSamplerState_uvwrap = anisotropicFilterSamplerState_uvwrap;

	// set default sampling : pointFilter with no uvwrapping

	ID3D11SamplerState* ss_array[] = { pointFilterSamplerState };
	for (long i = 0; i < nbTextureStages; i++)
	{
		lpd3ddevcontext->VSSetSamplers(i, 1, ss_array);
		lpd3ddevcontext->PSSetSamplers(i, 1, ss_array);
	}
	m_PSExtendedSamplers = false;
	m_VSExtendedSamplers = false;
	m_currentPSSampler = "none";
	m_currentVSSampler = "none";

	////////////////////////////////////////////////////////////////////////////

	// default renderstate description
	D3D11_RASTERIZER_DESC rsDesc;

	rsDesc.FillMode = D3D11_FILL_SOLID;

	// dans d3d9, le cull mode par defaut est ccw (cf doc)
	// donc idem ici

	// cull ccw
	rsDesc.CullMode = D3D11_CULL_BACK;
	rsDesc.FrontCounterClockwise = FALSE;
	// cull ccw

	/*
	// cull cw
	rsDesc.CullMode = D3D11_CULL_FRONT;
	rsDesc.FrontCounterClockwise = FALSE;
	// cull cw
	*/

	rsDesc.DepthBias = 0;
	rsDesc.SlopeScaledDepthBias = 0.0f;
	rsDesc.DepthBiasClamp = 0.0f;
	rsDesc.DepthClipEnable = TRUE;
	rsDesc.ScissorEnable = FALSE;
	rsDesc.MultisampleEnable = FALSE;
	rsDesc.AntialiasedLineEnable = FALSE;

	m_currentRSDesc = rsDesc;

	// apply this default renderstate
	if (!setCacheRS())
	{
		return false;
	}

	////////////////////////////////////////////////////////////////////////////

	// default blend state description

	D3D11_BLEND_DESC blDesc;
	ZeroMemory(&blDesc, sizeof(blDesc));

	blDesc.AlphaToCoverageEnable = false;
	blDesc.IndependentBlendEnable = false;
	blDesc.RenderTarget[0].BlendEnable = false;
	blDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ZERO;
	blDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	m_currentBlendDesc = blDesc;

	// apply this default blend state
	if (!setCacheBlendstate())
	{
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ShaderArg);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;

	hRes = m_lpd3ddevice->CreateBuffer(&bd, nullptr, &m_vertexShaderArgsBuffer);
	D3D11_CHECK(CreateBuffer);

	hRes = m_lpd3ddevice->CreateBuffer(&bd, nullptr, &m_pixelShaderArgsBuffer);
	D3D11_CHECK(CreateBuffer);

	/////////////////////////////////////////////////////////////////////////////

	// set viewport....
	if (fullscreen)
	{
		m_mainScreenViewport.Width = characteristics_width_resol;
		m_mainScreenViewport.Height = characteristics_height_resol;
		m_mainScreenViewport.MinDepth = 0.0;
		m_mainScreenViewport.MaxDepth = 1.0;
		m_mainScreenViewport.TopLeftX = 0.0;
		m_mainScreenViewport.TopLeftY = 0.0;
	}
	else
	{
		m_mainScreenViewport.Width = characteristics_width_resol;
		m_mainScreenViewport.Height = characteristics_height_resol;
		m_mainScreenViewport.MinDepth = 0.0;
		m_mainScreenViewport.MaxDepth = 1.0;
		m_mainScreenViewport.TopLeftX = rect.left;
		m_mainScreenViewport.TopLeftY = rect.top;
	}

	m_lpd3ddevcontext->RSSetViewports(1, &m_mainScreenViewport);

	// WIC (Windows Imaging Component : must initialize library for textures loading of standard format (bmp, jpg, ...)
	hRes = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	D3D11_CHECK(CoInitializeEx);
	

	_MAGE_DEBUG(m_localLogger, std::string("init D3D SUCCESS"))

	m_initialized = true;
	return true;
}
