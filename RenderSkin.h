#pragma once

#include "RenderingContext.h"
#include "VertexTypes.h"
#include "RenderMesh.h"

namespace happy
{
	class RenderSkin : public RenderMesh
	{
	public:
		virtual ~RenderSkin() {}

		virtual shared_ptr<RenderMesh> clone() const override { return make_shared<RenderSkin>(*this); }

		void setBindPose(const RenderingContext *pRenderContext, const vector<bb::mat4> &pose);

		ID3D11Buffer* getBindPoseBuffer() const;

	private:
		ComPtr<ID3D11Buffer> m_pBindPose;
	};
}