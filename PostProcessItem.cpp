#include "stdafx.h"
#include "Resources.h"
#include "PostProcessItem.h"

namespace happy
{
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