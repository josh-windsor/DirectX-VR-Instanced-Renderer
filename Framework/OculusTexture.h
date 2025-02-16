
//------------------------------------------------------------
// ovrSwapTextureSet wrapper class that also maintains the render target views
// needed for D3D11 rendering.
// Made by Oculus, edited by Josh Windsor
#include "OVR_CAPI_D3D.h"
#include <vector>
#include "CommonHeader.h"
#ifndef VALIDATE
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(nullptr, (msg), "JW_STGA", MB_ICONERROR | MB_OK); exit(-1); }
#endif

struct OculusTexture
{
	ovrSession               Session;
	ovrTextureSwapChain      TextureChain;
	ovrTextureSwapChain      DepthTextureChain;
	std::vector<ID3D11RenderTargetView*> TexRtv;
	std::vector<ID3D11DepthStencilView*> TexDsv;

	OculusTexture() :
		Session(nullptr),
		TextureChain(nullptr),
		DepthTextureChain(nullptr)
	{
	}

	bool Init(ovrSession session, int sizeW, int sizeH, int sampleCount, ID3D11Device* pD3DDevice)
	{
		Session = session;

		// create color texture swap chain first
		{
			//texture swap chain params
			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = sizeW;
			desc.Height = sizeH;
			desc.MipLevels = 1;
			desc.SampleCount = sampleCount;
			desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
			desc.MiscFlags = ovrTextureMisc_DX_Typeless | ovrTextureMisc_AutoGenerateMips;
			desc.BindFlags = ovrTextureBind_DX_RenderTarget;
			desc.StaticImage = ovrFalse;

			//get texture chain
			ovrResult result = ovr_CreateTextureSwapChainDX(session, pD3DDevice, &desc, &TextureChain);
			if (!OVR_SUCCESS(result))
				panicF("%i", result);


			int textureCount = 0;
			//get the current textures
			ovr_GetTextureSwapChainLength(Session, TextureChain, &textureCount);
			for (int i = 0; i < textureCount; ++i)
			{
				//convert oculus texture to dx
				ID3D11Texture2D* tex = nullptr;
				ovr_GetTextureSwapChainBufferDX(Session, TextureChain, i, IID_PPV_ARGS(&tex));

				//creates render target view with oculus texture
				D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
				rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				rtvd.ViewDimension = (sampleCount > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS
					: D3D11_RTV_DIMENSION_TEXTURE2D;
				ID3D11RenderTargetView* rtv;
				HRESULT hr = pD3DDevice->CreateRenderTargetView(tex, &rtvd, &rtv);
				VALIDATE((hr == ERROR_SUCCESS), "Error creating render target view");

				TexRtv.push_back(rtv);
				tex->Release();
			}
		}

		//create depth swap chain
		{
			//sets the depth params
			ovrTextureSwapChainDesc desc = {};
			desc.Type = ovrTexture_2D;
			desc.ArraySize = 1;
			desc.Width = sizeW;
			desc.Height = sizeH;
			desc.MipLevels = 1;
			desc.SampleCount = sampleCount;
			desc.Format = OVR_FORMAT_D32_FLOAT;
			desc.MiscFlags = ovrTextureMisc_None;
			desc.BindFlags = ovrTextureBind_DX_DepthStencil;
			desc.StaticImage = ovrFalse;

			ovrResult result = ovr_CreateTextureSwapChainDX(session, pD3DDevice, &desc, &DepthTextureChain);
			if (!OVR_SUCCESS(result))
				return false;

			int textureCount = 0;
			//get the current textures
			ovr_GetTextureSwapChainLength(Session, DepthTextureChain, &textureCount);
			for (int i = 0; i < textureCount; ++i)
			{
				//convert oculus texture to dx
				ID3D11Texture2D* tex = nullptr;
				ovr_GetTextureSwapChainBufferDX(Session, DepthTextureChain, i, IID_PPV_ARGS(&tex));

				//creates depth stencil view with oculus texture
				D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
				dsvDesc.ViewDimension = (sampleCount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS
					: D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvDesc.Texture2D.MipSlice = 0;

				ID3D11DepthStencilView* dsv;
				HRESULT hr = pD3DDevice->CreateDepthStencilView(tex, &dsvDesc, &dsv);
				VALIDATE((hr == ERROR_SUCCESS), "Error creating depth stencil view");
				
				TexDsv.push_back(dsv);
				tex->Release();

			}
		}
		return true;
	}

	~OculusTexture()
	{
		for (int i = 0; i < (int)TexRtv.size(); ++i)
		{
			TexRtv[i]->Release();
		}
		for (int i = 0; i < (int)TexDsv.size(); ++i)
		{
			TexDsv[i]->Release();
		}
		if (TextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, TextureChain);
		}
		if (DepthTextureChain)
		{
			ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
		}
	}

	ID3D11RenderTargetView* GetRTV()
	{
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &index);
		return TexRtv[index];
	}
	ID3D11DepthStencilView* GetDSV()
	{
		int index = 0;
		ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &index);
		return TexDsv[index];
	}

	// Commit changes
	void Commit()
	{
		ovr_CommitTextureSwapChain(Session, TextureChain);
		ovr_CommitTextureSwapChain(Session, DepthTextureChain);
	}
};

// ========================================================