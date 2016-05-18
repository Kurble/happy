#pragma once

#include "RenderingContext.h"
#include "VertexTypes.h"
#include "RenderMesh.h"

namespace happy
{
	class RenderSkin : public RenderMesh
	{
	public:
		void setBindPose(const RenderingContext *pRenderContext, const vector<Mat4> &pose);

		ID3D11Buffer* getBindPoseBuffer() const;

	private:
		ComPtr<ID3D11Buffer> m_pBindPose;
	};

	class Animation
	{
	public:
		void setAnimation(const RenderingContext *pRenderContext, const vector<Mat4> &animation, const unsigned bones, const unsigned frames, const float framerate);
		void setLooping(bool looping);

		ID3D11Buffer* getFrame0(float time);
		ID3D11Buffer* getFrame1(float time);
		float         getFrameBlend(float time);

	private:
		float m_FrameRate;
		bool  m_Looping;

		shared_ptr<vector<ComPtr<ID3D11Buffer>>> m_pFrames;
	};

	struct SkinRenderItem
	{
		RenderSkin m_Skin;
		Mat4  m_World;
		float m_BlendFrame;
		float m_BlendAnimation;
		// anim 0
		// anim 1;
	};
}