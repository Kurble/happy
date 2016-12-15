#pragma once

namespace happy
{
	struct PostProcessItem
	{
		enum Slot
		{
			Frame,
			PreviousFrame,
			Depth,
			Normals,
			Velocity,
		};

		virtual ~PostProcessItem() {}

		void setInputSlot(Slot buffer, unsigned slot);

		void setSceneInputSlot(unsigned slot);
		void setDepthInputSlot(unsigned slot);
		
		void addInputSlot(const TextureHandle &texture, unsigned slot);

		void* getShaderId() { return (void*)m_Handle.Get(); }

	protected:
		friend class Resources;
		friend class DeferredRenderer;
		friend class Canvas;
		
		ComPtr<ID3D11PixelShader> m_Handle;
		ComPtr<ID3D11Buffer>              m_ConstBuffer;
		shared_ptr<vector<unsigned char>> m_ConstBufferData;

		unsigned                      m_SceneInputSlot = 0;
		unsigned                      m_PreviousFrameInputSlot = (unsigned)-1;
		unsigned                      m_DepthInputSlot = (unsigned)-1;
		unsigned                      m_NormalsInputSlot = (unsigned)-1;
		unsigned                      m_VelocityInputSlot = (unsigned)-1;
		vector<pair<unsigned, void*>> m_InputSlots;
	};

	template <typename T> struct ConstBufPostProcessItem: public PostProcessItem
	{
		void setConstBuffer(const T& value)
		{
			memcpy(m_ConstBufferData->data(), reinterpret_cast<const unsigned char*>(&value), sizeof(value));
		}
	};
}
