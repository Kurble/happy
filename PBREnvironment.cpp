#include "stdafx.h"
#include "PBREnvironment.h"
#include "RenderMesh.h"

//----------------------------------------------------------------------
// Convolution shaders
#include "CompiledShaders\ConvolutionVS.h"
#include "CompiledShaders\ConvolutionPS.h"
//----------------------------------------------------------------------

namespace happy
{
	// https://github.com/Brammie/Tusk/blob/master/Core/Rendering/SkyboxComponent.cpp
	// https://github.com/Brammie/Tusk/blob/master/Tusk-Editor/Resources/Shaders/irradiance.fsh

	PBREnvironment::PBREnvironment() { }

	PBREnvironment::PBREnvironment(ComPtr<ID3D11ShaderResourceView> &environment)
		: m_EnvironmentMap(environment)
	{
		m_CubemapArrayLength = 0;
	}

	UINT PBREnvironment::getCubemapArrayLength() const
	{
		return m_CubemapArrayLength;
	}

	ID3D11ShaderResourceView* PBREnvironment::getLightingSRV() const
	{
		return m_ConvolutedMaps.Get();
	}

	ID3D11ShaderResourceView* PBREnvironment::getEnvironmentSRV() const
	{
		return m_EnvironmentMap.Get();
	}

	void PBREnvironment::convolute(RenderingContext* pRenderContext, float width, unsigned int steps)
	{
		ID3D11Device& device = *pRenderContext->getDevice();
		ID3D11DeviceContext& context = *pRenderContext->getContext();

		m_CubemapArrayLength = steps;

		struct
		{
			bb::mat4 side;
			float exponent;

			float size;
			float start;
			float end;
			float incr;
		} cb;

		cb.size = width;
		cb.start = ((0.5f / cb.size) - 0.5f)*2.0f;
		cb.end = -cb.start;
		cb.incr = 2.0f / cb.size;

		ComPtr<ID3D11Buffer> pConstBuffer;
		{
			D3D11_BUFFER_DESC desc;
			desc.Usage = D3D11_USAGE_DYNAMIC;
			desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
			desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			desc.MiscFlags = 0;
			desc.ByteWidth = (UINT)((sizeof(cb) + 15) / 16) * 16;
			THROW_ON_FAIL(device.CreateBuffer(&desc, NULL, pConstBuffer.GetAddressOf()));
		}

		ComPtr<ID3D11Buffer> pScreenQuad;
		UINT stride, offset;
		{
			VertexPositionTexcoord quad[] =
			{
				quad[0] = { bb::vec4(-1, -1,  1,  1), bb::vec2(-1, -1) },
				quad[1] = { bb::vec4( 1,  1,  1,  1), bb::vec2( 1,  1) },
				quad[2] = { bb::vec4( 1, -1,  1,  1), bb::vec2( 1, -1) },
				quad[3] = { bb::vec4(-1, -1,  1,  1), bb::vec2(-1, -1) },
				quad[4] = { bb::vec4(-1,  1,  1,  1), bb::vec2(-1,  1) },
				quad[5] = { bb::vec4( 1,  1,  1,  1), bb::vec2( 1,  1) },
			};

			stride = sizeof(VertexPositionTexcoord);
			offset = 0;

			D3D11_BUFFER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ByteWidth = sizeof(VertexPositionTexcoord) * 6;
			desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			desc.Usage = D3D11_USAGE_IMMUTABLE;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = (void*)quad;

			THROW_ON_FAIL(device.CreateBuffer(&desc, &data, &pScreenQuad));
		}

		ComPtr<ID3D11DepthStencilState> pDepthStencilState;
		{
			D3D11_DEPTH_STENCIL_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.DepthEnable = false;
			desc.StencilEnable = false;
			THROW_ON_FAIL(device.CreateDepthStencilState(&desc, &pDepthStencilState));
		}

		ComPtr<ID3D11BlendState> pBlendState;
		{
			D3D11_BLEND_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.RenderTarget[0].BlendEnable = true;
			desc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_COLOR;
			desc.RenderTarget[0].DestBlend = D3D11_BLEND_DEST_COLOR;
			desc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
			desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
			desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			desc.IndependentBlendEnable = false;
			THROW_ON_FAIL(device.CreateBlendState(&desc, &pBlendState));
		}

		ComPtr<ID3D11SamplerState> pSamplerState;
		{
			D3D11_SAMPLER_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
			desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
			desc.MinLOD = -FLT_MAX;
			desc.MaxLOD = FLT_MAX;
			desc.MipLODBias = 0;
			desc.MaxAnisotropy = 1;
			desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
			THROW_ON_FAIL(device.CreateSamplerState(&desc, &pSamplerState));
		}

		ComPtr<ID3D11InputLayout> pQuadLayout;
		{
			THROW_ON_FAIL(device.CreateInputLayout(VertexPositionTexcoord::Elements, VertexPositionTexcoord::ElementCount, g_shConvolutionVS, sizeof(g_shConvolutionVS), &pQuadLayout));
		}

		ComPtr<ID3D11VertexShader> pVertexShader;
		{
			THROW_ON_FAIL(device.CreateVertexShader(g_shConvolutionVS, sizeof(g_shConvolutionVS), nullptr, &pVertexShader));
		}

		ComPtr<ID3D11PixelShader> pPixelShader;
		{
			THROW_ON_FAIL(device.CreatePixelShader(g_shConvolutionPS, sizeof(g_shConvolutionPS), nullptr, &pPixelShader));
		}

		ComPtr<ID3D11Texture2D> pTexture;
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Width = (UINT)width;
			desc.Height = (UINT)width;
			desc.MipLevels = 1;
			desc.ArraySize = 6 * (steps);
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
			THROW_ON_FAIL(device.CreateTexture2D(&desc, nullptr, &pTexture));
		}

		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBEARRAY;
			desc.TextureCubeArray.MipLevels = 1;
			desc.TextureCubeArray.MostDetailedMip = 0;
			desc.TextureCubeArray.First2DArrayFace = 0;
			desc.TextureCubeArray.NumCubes = steps;
			THROW_ON_FAIL(device.CreateShaderResourceView(pTexture.Get(), &desc, &m_ConvolutedMaps));
		}

		D3D11_VIEWPORT vp;
		vp.Width = (float)width;
		vp.Height = (float)width;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		context.RSSetViewports(1, &vp);

		context.OMSetBlendState(pBlendState.Get(), nullptr, 0xffffffff);
		context.OMSetDepthStencilState(pDepthStencilState.Get(), 0);
		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(pQuadLayout.Get());
		context.IASetVertexBuffers(0, 1, pScreenQuad.GetAddressOf(), &stride, &offset);
		context.VSSetShader(pVertexShader.Get(), nullptr, 0);
		context.PSSetShader(pPixelShader.Get(), nullptr, 0);
		context.PSSetShaderResources(0, 1, m_EnvironmentMap.GetAddressOf());

		for (unsigned step = 0; step < steps; ++step)
		{
			ComPtr<ID3D11RenderTargetView> pRenderTarget[6];
			for (unsigned side = 0; side < 6; ++side)
			{
				D3D11_RENDER_TARGET_VIEW_DESC desc;
				desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				desc.Texture2DArray.ArraySize = 1;
				desc.Texture2DArray.MipSlice = 0;
				desc.Texture2DArray.FirstArraySlice = step*6 + side;
				THROW_ON_FAIL(device.CreateRenderTargetView(pTexture.Get(), &desc, &pRenderTarget[side]));
			}

			for (unsigned side = 0; side < 6; ++side)
			{
				cb.side.setRow(3, bb::vec4(0, 0, 0, 0));
				switch (side)
				{
					case D3D11_TEXTURECUBE_FACE_POSITIVE_Z: 
						cb.side.setRow(0, bb::vec4( 1,  0,  0,  0));
						cb.side.setRow(1, bb::vec4( 0,  1,  0,  0));
						cb.side.setRow(2, bb::vec4( 0,  0,  1,  0));
						break;
					case D3D11_TEXTURECUBE_FACE_NEGATIVE_Z:
						cb.side.setRow(0, bb::vec4(-1,  0,  0,  0));
						cb.side.setRow(1, bb::vec4( 0,  1,  0,  0));
						cb.side.setRow(2, bb::vec4( 0,  0, -1,  0));
						break;
					case D3D11_TEXTURECUBE_FACE_POSITIVE_X: 
						cb.side.setRow(0, bb::vec4( 0,  0, -1,  0));
						cb.side.setRow(1, bb::vec4( 0,  1,  0,  0));
						cb.side.setRow(2, bb::vec4( 1,  0,  0,  0));
						break;
					case D3D11_TEXTURECUBE_FACE_NEGATIVE_X:
						cb.side.setRow(0, bb::vec4( 0,  0,  1,  0));
						cb.side.setRow(1, bb::vec4( 0,  1,  0,  0));
						cb.side.setRow(2, bb::vec4(-1,  0,  0,  0));
						break;
					case D3D11_TEXTURECUBE_FACE_POSITIVE_Y: 
						cb.side.setRow(0, bb::vec4( 1,  0,  0,  0));
						cb.side.setRow(1, bb::vec4( 0,  0, -1,  0));
						cb.side.setRow(2, bb::vec4( 0,  1,  0,  0));
						break;
					case D3D11_TEXTURECUBE_FACE_NEGATIVE_Y: 
						cb.side.setRow(0, bb::vec4( 1,  0,  0,  0));
						cb.side.setRow(1, bb::vec4( 0,  0,  1,  0));
						cb.side.setRow(2, bb::vec4( 0, -1,  0,  0));
						break;
					
				}
				
				float gloss = (float)step / (float)(steps-1);
				cb.exponent = pow(2.0f, roundf(gloss * 12.0f));

				D3D11_MAPPED_SUBRESOURCE msr;
				THROW_ON_FAIL(context.Map(pConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
				memcpy(msr.pData, (void*)&cb, ((sizeof(cb) + 15) / 16) * 16);
				context.Unmap(pConstBuffer.Get(), 0);

				context.OMSetRenderTargets(1, pRenderTarget[side].GetAddressOf(), nullptr);
				
				context.VSSetConstantBuffers(0, 1, pConstBuffer.GetAddressOf());
				context.PSSetSamplers(0, 1, pSamplerState.GetAddressOf());
				context.PSSetConstantBuffers(0, 1, pConstBuffer.GetAddressOf());

				context.Draw(6, 0);
			}
		}

		ID3D11Buffer* nbuf = nullptr;
		context.PSSetConstantBuffers(0, 1, &nbuf);
		context.VSSetConstantBuffers(0, 1, &nbuf);

		ID3D11SamplerState* nsam = nullptr;
		context.PSSetSamplers(0, 1, &nsam);

		ID3D11ShaderResourceView* nrsv = nullptr;
		context.PSSetShaderResources(0, 1, &nrsv);
	}
}