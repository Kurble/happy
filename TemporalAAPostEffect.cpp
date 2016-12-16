#include "stdafx.h"
#include "TemporalAAPostEffect.h"
#include "CompiledShaders\TAA.h"

namespace happy
{
	TemporalAAPostEffect::TemporalAAPostEffect(RenderingContext* context)
	{
		THROW_ON_FAIL(context->getDevice()->CreatePixelShader(g_shTAA, sizeof(g_shTAA), nullptr, &m_Handle));

		m_KeepsHistory = true;
	}
}