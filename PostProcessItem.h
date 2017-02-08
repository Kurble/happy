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
		void addInputSlot(const TextureHandle &texture, unsigned slot);

		void* getShaderId() { return nullptr; }

	protected:
		//
	};

	template <typename T> struct ConstBufPostProcessItem: public PostProcessItem
	{
		void setConstBuffer(const T& value)
		{
			//memcpy(m_ConstBufferData->data(), reinterpret_cast<const unsigned char*>(&value), sizeof(value));
		}
	};
}
