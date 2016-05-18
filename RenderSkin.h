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
}