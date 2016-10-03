#pragma once

namespace happy
{
	struct TextureHandle
	{
		// useful if you use something like ImGui
		void* getTextureId() const { return (void*)m_Handle.Get(); }

		ComPtr<ID3D11ShaderResourceView> m_Handle;
	};
}
