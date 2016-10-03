#pragma once

namespace happy
{
	struct PostProcessItem
	{
		void setSceneInputSlot(unsigned slot);
		void setDepthInputSlot(unsigned slot);
		void addInputSlot(const TextureHandle &texture, unsigned slot);
		template <typename T> void setConstBuffer(const T& value)
		{
			m_ConstBufferData = vector<unsigned char>(sizeof(value));
			memcpy(m_ConstBufferData->data(), reinterpet_cast<unsigned char*>(&value), sizeof(value));
		}

		void* getShaderId() { return (void*)m_Handle.Get(); }

	protected:
		friend class Resources;
		friend class DeferredRenderer;
		
		ComPtr<ID3D11PixelShader> m_Handle;
		ComPtr<ID3D11Buffer>              m_ConstBuffer;
		shared_ptr<vector<unsigned char>> m_ConstBufferData;

		unsigned                      m_SceneInputSlot = 0;
		unsigned                      m_DepthInputSlot = (unsigned)-1;
		vector<pair<unsigned, void*>> m_InputSlots;
	};
}
