#pragma once

#include "TextureHandle.h"
#include "RenderMesh.h"

namespace happy
{
	using RenderList = vector<pair<RenderMesh, Mat4>>;

	class Canvas : public TextureHandle
	{
	public:
		void clearTexture();
		void clearGeometry();

		void pushRenderMesh(const RenderMesh &mesh, const Mat4 &transform);
		void setCamera(const Mat4 &view, const Mat4 &projection);

		void renderToTexture() const;

	private:
		friend class Resources;

		void load(ID3D11Device* device);

		template <typename T, size_t Length> void CreateVertexShader(ID3D11Device* device, ComPtr<ID3D11VertexShader> &vs, ComPtr<ID3D11InputLayout> &il, const BYTE(&shaderByteCode)[Length])
		{
			size_t length = Length;
			THROW_ON_FAIL(device->CreateVertexShader(shaderByteCode, length, nullptr, &vs));
			THROW_ON_FAIL(device->CreateInputLayout(T::Elements, T::ElementCount, shaderByteCode, length, &il));
		}

		template <size_t Length> void CreatePixelShader(ID3D11Device* device, ComPtr<ID3D11PixelShader> &ps, const BYTE(&shaderByteCode)[Length])
		{
			size_t length = Length;
			THROW_ON_FAIL(device->CreatePixelShader(shaderByteCode, length, nullptr, &ps));
		}

		ComPtr<ID3D11RenderTargetView> m_RenderTarget;
		RenderList m_GeometryPositionTexcoord;
		RenderList m_GeometryPositionNormalTexcoord;
		RenderList m_GeometryPositionNormalTangentBinormalTexcoord;
		Mat4 m_View;
		Mat4 m_Projection;

		ComPtr<ID3D11VertexShader> m_pVSPositionTexcoord;
		ComPtr<ID3D11VertexShader> m_pVSPositionNormalTexcoord;
		ComPtr<ID3D11VertexShader> m_pVSPositionNormalTangentBinormalTexcoord;
		ComPtr<ID3D11PixelShader>  m_pPSGeometry;
	};
}
