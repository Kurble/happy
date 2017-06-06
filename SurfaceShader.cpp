#include "stdafx.h"
#include "SurfaceShader.h"

namespace happy
{
	SurfaceShader::SurfaceShader()
	{
	}

	SurfaceShader::~SurfaceShader()
	{
	}

	bool SurfaceShader::operator<(const SurfaceShader& o) const
	{
		if (m_HandleVS.Get() == o.m_HandleVS.Get())
			return m_HandlePS.Get() < o.m_HandlePS.Get();
		return m_HandleVS.Get() < o.m_HandleVS.Get();
	}

	void SurfaceShader::addInputSlot(const TextureHandle &texture, unsigned slot)
	{
		m_InputSlots.emplace_back(slot, texture.getTextureId());
	}

	void SurfaceShader::set(ID3D11DeviceContext* context) const
	{
		context->PSSetShader(m_HandlePS.Get(), nullptr, 0);

		for (auto &i : m_InputSlots)
			context->PSSetShaderResources(i.first, 1, (ID3D11ShaderResourceView**)&i.second);
	}

	void SurfaceShader::unset(ID3D11DeviceContext* context) const
	{
		ID3D11ShaderResourceView* srv = nullptr;

		for (auto &i : m_InputSlots)
			context->PSSetShaderResources(i.first, 1, &srv);
	}
}