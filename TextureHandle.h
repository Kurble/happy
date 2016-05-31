#pragma once

namespace happy
{
	struct TextureHandle
	{
		ComPtr<ID3D11ShaderResourceView> m_Handle;

		ID3D11ShaderResourceView* getTextureId() { return m_Handle.Get(); }
	};
}