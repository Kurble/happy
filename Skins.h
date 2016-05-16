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