#pragma once

namespace happy
{
	struct PostProcessItem
	{
		void setSceneInputSlot(unsigned slot);
		void setDepthInputSlot(unsigned slot);
		void addInputSlot(const TextureHandle &texture, unsigned slot);

		void* getShaderId() { return (void*)m_Handle.Get(); }

	protected:
		friend class Resources;
		friend class DeferredRenderer;
		
		ComPtr<ID3D11PixelShader> m_Handle;

		unsigned                      m_SceneInputSlot = 0;
		unsigned                      m_DepthInputSlot = (unsigned)-1;
		vector<pair<unsigned, void*>> m_InputSlots;
	};
}
