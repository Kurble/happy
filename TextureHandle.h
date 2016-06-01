#pragma once

namespace happy
{
	struct TextureHandle
	{
		ComPtr<ID3D11ShaderResourceView> m_Handle;

		// useful if you use something like ImGui
		void* getTextureId() { return (void*)m_Handle.Get(); }
	};
}
