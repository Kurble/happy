#include "stdafx.h"
#include "Resources.h"
#include "PostProcessItem.h"

namespace happy
{
	void PostProcessItem::setInputSlot(Slot buffer, unsigned slot)
	{
		switch (buffer)
		{
		case Frame: m_SceneInputSlot = slot;  break;
		case PreviousFrame: m_PreviousFrameInputSlot = slot; break;
		case Depth: m_DepthInputSlot = slot;  break;
		case Normals: m_NormalsInputSlot = slot; break;
		case Velocity: m_VelocityInputSlot = slot;  break;
		}
	}

	void PostProcessItem::setSceneInputSlot(unsigned slot)
	{
		m_SceneInputSlot = slot;
	}

	void PostProcessItem::setDepthInputSlot(unsigned slot)
	{
		m_DepthInputSlot = slot;
	}

	void PostProcessItem::addInputSlot(const TextureHandle &texture, unsigned slot)
	{
		m_InputSlots.emplace_back(slot, texture.getTextureId());
	}
}