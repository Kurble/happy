#pragma once

#include "RenderingContext.h"

namespace happy
{
	class PBREnvironment
	{
	public:
		PBREnvironment();
		PBREnvironment(ComPtr<ID3D11ShaderResourceView> &environment);

		void convolute(RenderingContext *pRenderContext, float resolution = 64.0f, unsigned int steps=4);

		UINT getCubemapArrayLength() const;
		ID3D11ShaderResourceView* getEnvironmentSRV() const;
		ID3D11ShaderResourceView* getLightingSRV() const;

	private:
		ComPtr<ID3D11ShaderResourceView> m_EnvironmentMap;
		ComPtr<ID3D11ShaderResourceView> m_ConvolutedMaps;
		UINT m_CubemapArrayLength;
	};
}