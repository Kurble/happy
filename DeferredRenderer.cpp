#include "stdafx.h"
#include "DeferredRenderer.h"
#include "DeferredRendererConstBuffers.h"
#include "Sphere.h"
#include "Cube.h"
#include "Resources.h"

#include <algorithm>

namespace happy
{
	DeferredRenderer::DeferredRenderer(const RenderingContext* pRenderContext, const RendererConfiguration config)
		: m_pRenderContext(pRenderContext)
		, m_Config(config)
	{
		createStates(pRenderContext);
		createGeometries(pRenderContext);
		createBuffers(pRenderContext);
		createShaders(pRenderContext);
	}

	const RenderingContext* DeferredRenderer::getContext() const
	{
		return m_pRenderContext;
	}

	const RendererConfiguration& DeferredRenderer::getConfig() const
	{
		return m_Config;
	}

	void DeferredRenderer::render(const RenderQueue *scene, RenderTarget *target) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.RSSetState(m_pRasterState.Get());
		context.RSSetViewports(1, &target->m_ViewPort);

		bb::mat4 jitteredProjection;
		jitteredProjection.identity();
		jitteredProjection.translate(bb::vec3(
			target->m_Jitter[target->m_JitterIndex].x / target->getWidth(), 
			target->m_Jitter[target->m_JitterIndex].y / target->getHeight(), 0.0f));
		jitteredProjection.multiply(target->m_Projection);

		CBufferScene sceneCB;
		sceneCB.jitteredView = target->m_View;
		sceneCB.jitteredProjection = jitteredProjection;
		sceneCB.currentView = target->m_View;
		sceneCB.currentProjection = target->m_Projection;
		sceneCB.previousView = target->m_ViewHistory;
		sceneCB.previousProjection = target->m_ProjectionHistory;
		sceneCB.inverseView = sceneCB.jitteredView;
		sceneCB.inverseView.inverse();
		sceneCB.inverseProjection = sceneCB.jitteredProjection;
		sceneCB.inverseProjection.inverse();
		sceneCB.width = (float)m_pRenderContext->getWidth();
		sceneCB.height = (float)m_pRenderContext->getHeight();
		sceneCB.convolutionStages = scene->m_Environment.getCubemapArrayLength();
		sceneCB.aoEnabled = m_Config.m_PostEffectQuality >= Quality::Normal ? 1 : 0;
		updateConstantBuffer<CBufferScene>(&context, m_pCBScene.Get(), sceneCB);

		renderGeometry(scene, target);

		renderDeferred(scene, target);

		target->m_LastUsedHistoryBuffer += 1;
		target->m_LastUsedHistoryBuffer %= 2;
		target->m_JitterIndex += 1;
		target->m_JitterIndex %= RenderTarget::MultiSamples;
	}

	void DeferredRenderer::renderStaticMeshList(const vector<RenderQueue::MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		
		context.IASetInputLayout(layout);
		context.VSSetShader(shader, nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : renderList)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_Transform;
			objectCB.previousWorld = elem.m_Transform;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			UINT stride = elem.m_Mesh.getVertexStride();
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Mesh.getVtxBuffer();

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Group);
			context.PSSetShaderResources(0, 3, elem.m_Mesh.getTextures());
			context.IASetIndexBuffer(elem.m_Mesh.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Mesh.getIndexCount(), 0, 0);
		}
	}

	void DeferredRenderer::renderGeometry(const RenderQueue *scene, RenderTarget *target) const
	{
		ID3D11DeviceContext& context = *m_pRenderContext->getContext();

		ID3D11RenderTargetView* rtvs[] = { target->m_GraphicsBuffer[0].rtv.Get(), target->m_GraphicsBuffer[1].rtv.Get(), target->m_GraphicsBuffer[2].rtv.Get() };
		context.OMSetRenderTargets(3, rtvs, target->m_pDepthBufferView.Get());
		context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context.ClearDepthStencilView(target->m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBObject.Get(),
			m_pCBSkin.Get(),
		};

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.PSSetShader(m_pPSGeometry.Get(), nullptr, 0);
		context.PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context.PSSetConstantBuffers(0, 3, constBuffers);

		//=========================================================
		// Render Static Meshes, opaque
		//=========================================================
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList(scene->m_Geometry##X, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//=========================================================
		// Render Skins, opaque
		//=========================================================
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_CurrentWorld;
			objectCB.previousWorld = elem.m_PreviousWorld;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			CBufferSkin skinCB;
			skinCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 2; ++i) skinCB.previousBlendAnim[i] = (&elem.m_PreviousBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.previousBlendFrame[i] = (&elem.m_PreviousBlendFrame.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendAnim[i] = (&elem.m_CurrentBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendFrame[i] = (&elem.m_CurrentBlendFrame.x)[i];
			updateConstantBuffer(&context, m_pCBSkin.Get(), skinCB);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoordIndicesWeights);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Skin.getVtxBuffer();
			
			vector<ID3D11Buffer*> buffers = 
			{
				elem.m_Skin.getBindPoseBuffer()
			};

			for (unsigned i = 0; i < 4; ++i)
				if (i < elem.m_AnimationCount * 2)
					buffers.push_back(elem.m_PreviousFrames[i]);
				else
					buffers.push_back(nullptr);
			for (unsigned i = 0; i < 4; ++i)
				if (i < elem.m_AnimationCount * 2)
					buffers.push_back(elem.m_CurrentFrames[i]);
				else
					buffers.push_back(nullptr);

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Groups);
			context.VSSetConstantBuffers(3, (UINT)buffers.size(), &buffers[0]);
			context.PSSetShaderResources(0, 3, elem.m_Skin.getTextures());
			context.IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}

		context.PSSetShader(m_pPSGeometryAlphaStippled.Get(), nullptr, 0);
		context.PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context.PSSetConstantBuffers(0, 3, constBuffers);

		//=========================================================
		// Render Static Meshes, transparent
		//=========================================================
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList(scene->m_Geometry##X##Transparent, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST

		//=========================================================
		// Render Skins, transparent
		//=========================================================
		context.IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context.VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		for (const auto &elem : scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_CurrentWorld;
			objectCB.previousWorld = elem.m_PreviousWorld;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			CBufferSkin skinCB;
			skinCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 2; ++i) skinCB.previousBlendAnim[i] = (&elem.m_PreviousBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.previousBlendFrame[i] = (&elem.m_PreviousBlendFrame.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendAnim[i] = (&elem.m_CurrentBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendFrame[i] = (&elem.m_CurrentBlendFrame.x)[i];
			updateConstantBuffer(&context, m_pCBSkin.Get(), skinCB);

			UINT stride = sizeof(VertexPositionNormalTangentBinormalTexcoordIndicesWeights);
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Skin.getVtxBuffer();

			vector<ID3D11Buffer*> buffers =
			{
				elem.m_Skin.getBindPoseBuffer()
			};

			for (unsigned i = 0; i < 4; ++i)
				if (i < elem.m_AnimationCount * 2)
					buffers.push_back(elem.m_PreviousFrames[i]);
				else
					buffers.push_back(nullptr);
			for (unsigned i = 0; i < 4; ++i)
				if (i < elem.m_AnimationCount * 2)
					buffers.push_back(elem.m_CurrentFrames[i]);
				else
					buffers.push_back(nullptr);

			context.OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), elem.m_Groups);
			context.VSSetConstantBuffers(3, (UINT)buffers.size(), &buffers[0]);
			context.PSSetShaderResources(0, 3, elem.m_Skin.getTextures());
			context.IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context.DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}

		//=========================================================
		// Render Decals
		//=========================================================
		context.OMSetRenderTargets(3, rtvs, target->m_pDepthBufferViewReadOnly.Get());
		context.OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);
		context.IASetInputLayout(m_pILPositionTexcoord.Get());
		context.VSSetShader(m_pVSPositionTexcoord.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 3, constBuffers);
		context.PSSetShader(m_pPSDecals.Get(), nullptr, 0);
		for (const auto &elem : scene->m_Decals)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_Transform;
			objectCB.inverseWorld = elem.m_Transform;
			objectCB.inverseWorld.inverse();
			objectCB.alpha = 1.0f;
			updateConstantBuffer(&context, m_pCBObject.Get(), objectCB);

			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			ID3D11Buffer* buffer = m_pCubeVBuffer.Get();

			ID3D11ShaderResourceView* textures[] =
			{
				elem.m_Texture.m_Handle.Get(),
				elem.m_NormalMap.m_Handle.Get(),
				nullptr,
				nullptr,
				nullptr,
				target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get(),
			};

			context.OMSetDepthStencilState(m_pDecalsDepthStencilState.Get(), elem.m_Filter);
			context.PSSetShaderResources(0, 6, textures);
			context.IASetIndexBuffer(m_pCubeIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context.IASetVertexBuffers(0, 1, m_pCubeVBuffer.GetAddressOf(), &stride, &offset);
			context.DrawIndexed(36, 0, 0);
		}
	}

	void DeferredRenderer::renderDeferred(const RenderQueue *scene, RenderTarget *target) const
	{
		float clearColor[] = { 0, 0, 0, 0 };
		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
		ID3D11ShaderResourceView* nullSRV = nullptr;

		ID3D11DeviceContext& context = *m_pRenderContext->getContext();
		context.OMSetDepthStencilState(m_pLightingDepthStencilState.Get(), 0);
		context.OMSetBlendState(nullptr, nullptr, 0xffffffff);

		ID3D11SamplerState* samplers[] = { m_pScreenSampler.Get(), m_pGSampler.Get() };

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBPointLighting.Get()
		};

		context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context.IASetInputLayout(m_pILScreenQuad.Get());
		context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
		context.VSSetConstantBuffers(0, 2, constBuffers);
		context.PSSetSamplers(0, 2, samplers);
		context.PSSetConstantBuffers(0, 2, constBuffers);
		UINT stride = sizeof(VertexPositionTexcoord);
		UINT offset = 0;
		context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);
		
		//=========================================================
		// Generate SSAO buffer
		//=========================================================
		if (m_Config.m_PostEffectQuality >= Quality::High)
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
				m_pCBSSAO.Get(),
			};

			context.RSSetViewports(1, &target->m_BlurViewPort);

			// Shader 1: SSAO
			context.OMSetRenderTargets(1, target->m_GraphicsBuffer[RenderTarget::GBuf_OcclusionIdx].rtv.GetAddressOf(), nullptr);
			context.PSSetShader(m_pPSSSAO.Get(), nullptr, 0);
			context.PSSetConstantBuffers(0, 3, constBuffers);
			context.PSSetSamplers(0, 2, samplers);

			srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
			srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			srvs[6] = m_pNoiseTexture[target->m_JitterIndex].Get();
			context.PSSetShaderResources(0, 8, srvs);

			context.Draw(6, 0);

			context.RSSetViewports(1, &target->m_ViewPort);
		}

		//=========================================================
		// SRVs State
		//=========================================================
		srvs[0] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics0Idx].srv.Get();
		srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
		srvs[2] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics2Idx].srv.Get();
		srvs[3] = nullptr;
		srvs[4] = target->m_GraphicsBuffer[RenderTarget::GBuf_OcclusionIdx].srv.Get();
		srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
		srvs[6] = scene->m_Environment.getLightingSRV();
		srvs[7] = scene->m_Environment.getEnvironmentSRV();

		//=========================================================
		// Render lighting
		//=========================================================
		{
			ID3D11RenderTargetView* rtvs[] = 
			{ 
				target->m_PostBuffer[0].rtv.Get()
			};
			context.OMSetRenderTargets(1, rtvs, nullptr);
			context.PSSetShaderResources(0, 8, srvs);

			// Render environmental lighting
			context.PSSetShader(m_pPSGlobalLighting.Get(), nullptr, 0);
			context.Draw(6, 0);

			// Render point lights
			// no overdraw pls, put the lights in the pbr shader

			/*context.VSSetShader(m_pVSPointLighting.Get(), nullptr, 0);
			context.IASetInputLayout(m_pILPointLighting.Get());
			context.IASetVertexBuffers(0, 1, m_pSphereVBuffer.GetAddressOf(), &stride, &offset);
			context.IASetIndexBuffer(m_pSphereIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context.PSSetShader(m_pPSPointLighting.Get(), nullptr, 0);

			for (auto &light : m_PointLights)
			{
				CBufferPointLight cbuf;
				cbuf.position = bb::vec4(light.m_Position.x, light.m_Position.y, light.m_Position.z, 0.0f);
				cbuf.color    = bb::vec4(light.m_Color.x, light.m_Color.y, light.m_Color.z, 0.0f);
				cbuf.scale    = light.m_Radius;
				cbuf.falloff  = light.m_FaloffExponent;

				D3D11_MAPPED_SUBRESOURCE msr;
				THROW_ON_FAIL(context.Map(m_pCBPointLighting.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
				memcpy(msr.pData, (void*)&cbuf, ((sizeof(CBufferPointLight) + 15) / 16) * 16);
				context.Unmap(m_pCBPointLighting.Get(), 0);

				context.DrawIndexed(sizeof(g_SphereIndices) / sizeof(uint16_t), 0, 0);
			}*/
		}

		//=========================================================
		// Anti Aliasing
		//=========================================================
		if (m_Config.m_AAEnabled)
		{
			ID3D11RenderTargetView* rtvs[] = 
			{ 
				target->historyRTV()
			};

			srvs[0] = target->m_PostBuffer[0].srv.Get();
			srvs[1] = target->historySRV();
			srvs[2] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			srvs[3] = nullptr;
			srvs[4] = nullptr;
			srvs[5] = nullptr;
			srvs[6] = nullptr;
			srvs[7] = nullptr;

			CBufferTAA cbuf;
			cbuf.viewInverse = target->m_View;
			cbuf.viewInverse.inverse();
			cbuf.projectionInverse = target->m_Projection;
			cbuf.projectionInverse.inverse();
			cbuf.viewHistory = target->m_ViewHistory;
			cbuf.projectionHistory = target->m_ProjectionHistory;
			cbuf.blendFactor = 1.6f / (RenderTarget::MultiSamples + 1);
			cbuf.texelWidth = 1.0f / target->getWidth();
			cbuf.texelHeight = 1.0f / target->getHeight();
			target->m_ViewHistory = target->m_View;
			target->m_ProjectionHistory = target->m_Projection;

			updateConstantBuffer<CBufferTAA>(&context, m_pCBTAA.Get(), cbuf);

			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
				m_pCBTAA.Get(),
			};

			context.OMSetRenderTargets(1, rtvs, nullptr);
			context.PSSetShader(m_pPSTAA.Get(), nullptr, 0);
			context.PSSetShaderResources(0, 8, srvs);
			context.PSSetConstantBuffers(0, 3, constBuffers);

			// Render antialias
			context.Draw(6, 0);
		}

		//=========================================================
		// Post processing
		//=========================================================
		{
			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
			};

			context.IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context.IASetInputLayout(m_pILScreenQuad.Get());
			context.VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
			context.VSSetConstantBuffers(0, 2, constBuffers);
			UINT stride = sizeof(VertexPositionTexcoord);
			UINT offset = 0;
			context.IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);

			int pptarget = 1;
			int ppview = 0;
			size_t pass = 0;

			for (auto process = scene->m_PostProcessItems.begin(); process != scene->m_PostProcessItems.end(); ++process)
			{
				ID3D11RenderTargetView* pp_rtvs[] = { target->m_PostBuffer[pptarget].rtv.Get() };
				if (pass == (scene->m_PostProcessItems.size() - 1))
				{
					if (target->m_pOutputTarget)
					{
						pp_rtvs[0] = target->m_pOutputTarget;
						target->m_Handle = nullptr;
					}
					else
					{
						target->m_Handle = target->m_PostBuffer[pptarget].srv.Get();
					}
				}

				ID3D11ShaderResourceView* pp_srvs[10] = { 0 };
				if (process->m_SceneInputSlot < 10) 
					pp_srvs[process->m_SceneInputSlot] = target->m_PostBuffer[ppview].srv.Get();
				if (process->m_DepthInputSlot < 10) 
					pp_srvs[process->m_DepthInputSlot] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
				if (process->m_NormalsInputSlot < 10)
					pp_srvs[process->m_DepthInputSlot] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
				if (process->m_VelocityInputSlot < 10)
					pp_srvs[process->m_VelocityInputSlot] = nullptr;

				for (auto &slot : process->m_InputSlots)
					pp_srvs[slot.first] = (ID3D11ShaderResourceView*)slot.second;

				// take AA result as first pass input if AA is enabled
				if (pass == 0 && m_Config.m_AAEnabled)
				{
					pp_srvs[process->m_SceneInputSlot] = target->currentSRV();
				}

				if (process->m_ConstBuffer)
				{
					D3D11_MAPPED_SUBRESOURCE msr;
					THROW_ON_FAIL(context.Map(process->m_ConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
					memcpy(msr.pData, (void*)process->m_ConstBufferData->data(), process->m_ConstBufferData->size());
					context.Unmap(process->m_ConstBuffer.Get(), 0);

					constBuffers[1] = process->m_ConstBuffer.Get();
				}
				else
				{
					constBuffers[1] = nullptr;
				}

				context.OMSetRenderTargets(1, pp_rtvs, nullptr);
				context.PSSetShader(process->m_Handle.Get(), nullptr, 0);
				context.PSSetShaderResources(0, 10, pp_srvs);
				context.PSSetSamplers(0, 2, samplers);
				context.PSSetConstantBuffers(0, 2, constBuffers);
				
				context.Draw(6, 0);

				std::swap(pptarget, ppview);
				pass++;

				context.PSSetShaderResources(process->m_SceneInputSlot, 1, &nullSRV);
			}
		}

		// Reset SRVs since we need them as output next frame
		for (int i = 0; i < 8; ++i) srvs[i] = nullptr;
		context.PSSetShaderResources(0, 8, srvs);
	}
}