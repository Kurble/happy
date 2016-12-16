#pragma once

#include "TextureHandle.h"
#include "PostProcessItem.h"
#include "RenderingContext.h"

namespace happy
{
	class TemporalAAPostEffect : private PostProcessItem
	{
	public:
		TemporalAAPostEffect(RenderingContext* context);

	private:
		//
	};
}