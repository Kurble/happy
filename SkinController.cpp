#include "stdafx.h"
#include "SkinController.h"

namespace happy
{
	void SkinController::setSkin(RenderSkin &skin)
	{
		m_RenderItem.m_Skin = skin;

		m_RenderItem.m_AnimationCount = 1;
		m_RenderItem.m_World.identity();
		m_RenderItem.m_World.scale(Vec3(0.6f, 0.6f, 0.6f));
		m_RenderItem.m_BlendAnimation = Vec4(1, 0, 0, 0);
		m_RenderItem.m_BlendFrame = Vec4(0, 0, 0, 0);
	}

	int SkinController::addAnimation(string name, Animation animation)
	{
		anim_state state;

		state.m_Name = name;
		state.m_Anim = animation;
		state.m_Blender = system_clock::now();
		state.m_Timer = system_clock::now();
		state.m_SpeedMultiplier = 1;
		state.m_BlendSpeed = 0;
		state.m_BlendTarget = 1;
		state.m_Looped = false;

		m_States.push_back(state);
		return (int)(m_States.size() - 1);
	}

	int SkinController::getAnimationIndex(string name)
	{
		return 0;
	}

	void SkinController::update()
	{
		m_RenderItem.m_Frames[0] = m_States.front().m_Anim.getFrame0(0);
		m_RenderItem.m_Frames[1] = m_States.front().m_Anim.getFrame1(0);
		m_RenderItem.m_BlendFrame[0] = m_States.front().m_Anim.getFrameBlend(0);
	}

	const SkinRenderItem& SkinController::getRenderItem() const
	{		
		return m_RenderItem;
	}
}