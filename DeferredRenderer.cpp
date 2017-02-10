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
		, m_BufLineWidgets(pRenderContext->getDevice(), 1024)
		, m_BufTriWidgets(pRenderContext->getDevice(), 1536)
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
		auto context = m_pRenderContext->getContext("DeferredRenderer::render");
		context->RSSetState(m_pRasterState.Get());
		context->RSSetViewports(1, &target->m_ViewPort);

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
		sceneCB.width = (float)target->getWidth();
		sceneCB.height = (float)target->getHeight();
		sceneCB.convolutionStages = scene->m_Environment.getCubemapArrayLength();
		sceneCB.aoEnabled = m_Config.m_PostEffectQuality >= Quality::Normal ? 1 : 0;
		updateConstantBuffer<CBufferScene>(context, m_pCBScene.Get(), sceneCB);

		// Render the scene to the graphics buffer
		renderGeometry(scene, target);

		// Prepare pipeline for screen space rendering
		setScreenSpaceRendering();

		// Perform screen space rendering
		renderLighting(scene, target);
		renderWidgets(scene, target);
		renderAA(scene, target);
		renderPostProcessing(scene, target);

		// Reset SRVs since we need them as output next frame
		vector<ID3D11ShaderResourceView*> srvs(8, nullptr);
		context->PSSetShaderResources(0, 8, srvs.data());

		// Update temporal counters in render target
		target->m_LastUsedHistoryBuffer += 1;
		target->m_LastUsedHistoryBuffer %= 2;
		target->m_JitterIndex += 1;
		target->m_JitterIndex %= RenderTarget::MultiSamples;
	}

	void DeferredRenderer::setScreenSpaceRendering() const
	{
		auto context = m_pRenderContext->getContext();

		UINT stride = sizeof(VertexPositionTexcoord);
		UINT offset = 0;
		context->OMSetDepthStencilState(m_pLightingDepthStencilState.Get(), 0);
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetInputLayout(m_pILScreenQuad.Get());
		context->VSSetShader(m_pVSScreenQuad.Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		context->IASetVertexBuffers(0, 1, m_pScreenQuadBuffer.GetAddressOf(), &stride, &offset);
	}

	void DeferredRenderer::renderGeometry(const RenderQueue *scene, RenderTarget *target) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderGeometry");

		ID3D11RenderTargetView* rtvs[] = 
		{ 
			target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics0Idx].rtv.Get(), 
			target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].rtv.Get(),
			target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics2Idx].rtv.Get(),
			target->m_GraphicsBuffer[RenderTarget::GBuf_VelocityIdx].rtv.Get()
		};
		{
			float col[] = { 1, 1, 1, 1 };
			context->ClearDepthStencilView(target->m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);
			context->ClearRenderTargetView(target->m_GraphicsBuffer[RenderTarget::GBuf_VelocityIdx].rtv.Get(), col);
		}
		context->OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
		context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

		auto setStaticRendering = [&] {context->OMSetRenderTargets(3, rtvs, target->m_pDepthBufferView.Get());};
		auto setDynamicRendering = [&] {context->OMSetRenderTargets(4, rtvs, target->m_pDepthBufferView.Get());};

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBObject.Get(),
			m_pCBSkin.Get(),
		};

		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//=========================================================
		// Render opaque meshes
		//=========================================================
		context->PSSetShader(m_pPSGeometry.Get(), nullptr, 0);
		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		setStaticRendering();
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList(scene->m_Geometry##X, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST
		setDynamicRendering();
		renderSkinList(scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights);

		//=========================================================
		// Render transparent meshes
		//=========================================================
		context->PSSetShader(m_pPSGeometryAlphaStippled.Get(), nullptr, 0);
		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		setStaticRendering();
		#define RENDER_STATIC_MESH_LIST(X) renderStaticMeshList(scene->m_Geometry##X##Transparent, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(PositionTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(PositionNormalTangentBinormalTexcoord);
		#undef RENDER_STATIC_MESH_LIST
		setDynamicRendering();
		renderSkinList(scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent);

		//=========================================================
		// Render Decals
		//=========================================================
		context->OMSetRenderTargets(3, rtvs, target->m_pDepthBufferViewReadOnly.Get());
		context->OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);
		context->IASetInputLayout(m_pILPositionTexcoord.Get());
		context->VSSetShader(m_pVSPositionTexcoord.Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShader(m_pPSDecals.Get(), nullptr, 0);
		for (const auto &elem : scene->m_Decals)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_Transform;
			objectCB.inverseWorld = elem.m_Transform;
			objectCB.inverseWorld.inverse();
			objectCB.alpha = 1.0f;
			updateConstantBuffer(context, m_pCBObject.Get(), objectCB);

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

			context->OMSetDepthStencilState(m_pDecalsDepthStencilState.Get(), elem.m_Filter);
			context->PSSetShaderResources(0, 6, textures);
			context->IASetIndexBuffer(m_pCubeIBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
			context->IASetVertexBuffers(0, 1, m_pCubeVBuffer.GetAddressOf(), &stride, &offset);
			context->DrawIndexed(36, 0, 0);
		}
	}

	void DeferredRenderer::renderStaticMeshList(const vector<RenderQueue::MeshItem> &renderList, ID3D11InputLayout *layout, ID3D11VertexShader *shader, ID3D11Buffer **constBuffers) const
	{
		if (renderList.size() == 0) return;

		auto context = m_pRenderContext->getContext("DeferredRenderer::renderStaticMeshList");

		context->IASetInputLayout(layout);
		context->VSSetShader(shader, nullptr, 0);
		context->VSSetConstantBuffers(0, 3, constBuffers);

		StencilMask current = (StencilMask)-1;

		for (const auto &elem : renderList)
		{
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_Transform;
			objectCB.previousWorld = elem.m_Transform;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(context, m_pCBObject.Get(), objectCB);

			UINT stride = elem.m_Mesh.getVertexStride();
			UINT offset = 0;
			ID3D11Buffer* buffer = elem.m_Mesh.getVtxBuffer();

			if (current != elem.m_Group)
				context->OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), current = elem.m_Group);
			context->PSSetShaderResources(0, 3, elem.m_Mesh.getTextures());
			context->IASetIndexBuffer(elem.m_Mesh.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context->DrawIndexed((UINT)elem.m_Mesh.getIndexCount(), 0, 0);
		}
	}

	void DeferredRenderer::renderSkinList(const vector<SkinRenderItem> &renderList) const
	{
		if (renderList.size() == 0) return;

		auto context = m_pRenderContext->getContext("DeferredRenderer::renderSkinList");

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			m_pCBObject.Get(),
			m_pCBSkin.Get(),
		};
		context->IASetInputLayout(m_pILPositionNormalTangentBinormalTexcoordIndicesWeights.Get());
		context->VSSetShader(m_pVSPositionNormalTangentBinormalTexcoordIndicesWeights.Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 3, constBuffers);

		StencilMask current = (StencilMask)-1;

		for (const auto &elem : renderList)
		{
			auto skinContext = m_pRenderContext->getContext("skin element");
			CBufferObject objectCB;
			objectCB.currentWorld = elem.m_CurrentWorld;
			objectCB.previousWorld = elem.m_PreviousWorld;
			objectCB.alpha = elem.m_Alpha;
			updateConstantBuffer(context, m_pCBObject.Get(), objectCB);

			CBufferSkin skinCB;
			skinCB.animationCount = elem.m_AnimationCount;
			for (int i = 0; i < 2; ++i) skinCB.previousBlendAnim[i] = (&elem.m_PreviousBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.previousBlendFrame[i] = (&elem.m_PreviousBlendFrame.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendAnim[i] = (&elem.m_CurrentBlendAnimation.x)[i];
			for (int i = 0; i < 2; ++i) skinCB.currentBlendFrame[i] = (&elem.m_CurrentBlendFrame.x)[i];
			updateConstantBuffer(context, m_pCBSkin.Get(), skinCB);

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

			if (current != elem.m_Groups)
				context->OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), current = elem.m_Groups);
			context->VSSetConstantBuffers(3, (UINT)buffers.size(), &buffers[0]);
			context->PSSetShaderResources(0, 3, elem.m_Skin.getTextures());
			context->IASetIndexBuffer(elem.m_Skin.getIdxBuffer(), DXGI_FORMAT_R16_UINT, 0);
			context->IASetVertexBuffers(0, 1, &buffer, &stride, &offset);
			context->DrawIndexed((UINT)elem.m_Skin.getIndexCount(), 0, 0);
		}
	}

	void DeferredRenderer::renderScreenSpacePass(ID3D11PixelShader *ps, ID3D11RenderTargetView* rtv, ID3D11Buffer** constBuffers, ID3D11ShaderResourceView** srvs, ID3D11SamplerState** samplers) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderScreenSpacePass");

		context->OMSetRenderTargets(1, &rtv, nullptr);
		context->PSSetShader(ps, nullptr, 0);
		context->PSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShaderResources(0, 8, srvs);
		context->PSSetSamplers(0, 2, samplers);

		context->Draw(6, 0);
	}

	void DeferredRenderer::renderLighting(const RenderQueue *scene, RenderTarget *target) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderLighting");

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			nullptr,
			nullptr,
		};

		ID3D11ShaderResourceView* srvs[8] = { 0 };

		ID3D11SamplerState* samplers[] =
		{
			m_pScreenSampler.Get(),
			m_pGSampler.Get()
		};

		//=========================================================
		// SSAO
		//=========================================================
		if (m_Config.m_PostEffectQuality >= Quality::High)
		{
			srvs[0] = nullptr;
			srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
			srvs[2] = nullptr;
			srvs[3] = nullptr;
			srvs[4] = nullptr;
			srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			srvs[6] = m_pNoiseTexture[target->m_JitterIndex].Get();
			srvs[7] = nullptr;	
			constBuffers[2] = m_pCBSSAO.Get();

			context->RSSetViewports(1, &target->m_BlurViewPort);
			renderScreenSpacePass(m_pPSSSAO.Get(), target->m_GraphicsBuffer[RenderTarget::GBuf_OcclusionIdx].rtv.Get(), constBuffers, srvs, samplers);
			context->RSSetViewports(1, &target->m_ViewPort);
		}

		//=========================================================
		// PBR
		//=========================================================
		srvs[0] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics0Idx].srv.Get();
		srvs[1] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
		srvs[2] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics2Idx].srv.Get();
		srvs[3] = nullptr;
		srvs[4] = target->m_GraphicsBuffer[RenderTarget::GBuf_OcclusionIdx].srv.Get();
		srvs[5] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
		srvs[6] = scene->m_Environment.getLightingSRV();
		srvs[7] = scene->m_Environment.getEnvironmentSRV();
		constBuffers[2] = nullptr;
		renderScreenSpacePass(m_pPSGlobalLighting.Get(), target->m_PostBuffer[0].rtv.Get(), constBuffers, srvs, samplers);	
	}

	void DeferredRenderer::renderWidgets(const RenderQueue *scene, RenderTarget *target) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderWidgets");

		if (scene->m_Lines.size() > 0)
		{
			ID3D11ShaderResourceView* srvs[8] = { 0 };

			context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
			context->IASetInputLayout(m_pILPositionColor.Get()); 
			context->VSSetShader(m_pVSWidgetsPositionColor.Get(), nullptr, 0);
			context->VSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
			context->PSSetShader(m_pPSWidgets.Get(), nullptr, 0);
			context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
			context->PSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
			context->PSSetShaderResources(0, 8, srvs);
			context->OMSetRenderTargets(1, target->m_PostBuffer[0].rtv.GetAddressOf(), target->m_pDepthBufferView.Get());
			context->OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
			context->OMSetBlendState(nullptr, nullptr, 0xffffffff);

			m_BufLineWidgets.begin(context);
			m_BufLineWidgets.draw(context, scene->m_Lines.data(), scene->m_Lines.size(), 2,
				[](VertexPositionColor* vertices, const RenderQueue::LineWidgetItem* objects, size_t count)
				{
					for (size_t i=0; i<count; ++i)
					{
						vertices[i * 2 + 0].pos   = objects[i].m_From;
						vertices[i * 2 + 0].color = objects[i].m_Color;
						vertices[i * 2 + 1].pos   = objects[i].m_To;
						vertices[i * 2 + 1].color = objects[i].m_Color;
					}
				}
			);
			m_BufLineWidgets.end(context);

			// restore screen space rendering
			setScreenSpaceRendering();
		}
	}

	void DeferredRenderer::renderAA(const RenderQueue *scene, RenderTarget *target) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderAA");

		ID3D11Buffer* constBuffers[] =
		{
			m_pCBScene.Get(),
			nullptr,
			nullptr,
		};

		ID3D11ShaderResourceView* srvs[8] = { 0 };

		ID3D11SamplerState* samplers[] =
		{
			m_pScreenSampler.Get(),
			m_pGSampler.Get()
		};

		//=========================================================
		// TAA
		//=========================================================
		if (m_Config.m_AAEnabled)
		{
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
			updateConstantBuffer<CBufferTAA>(context, m_pCBTAA.Get(), cbuf);

			srvs[0] = target->m_PostBuffer[0].srv.Get();
			srvs[1] = target->historySRV();
			srvs[2] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			srvs[3] = target->m_GraphicsBuffer[RenderTarget::GBuf_VelocityIdx].srv.Get();
			srvs[4] = nullptr;
			srvs[5] = nullptr;
			srvs[6] = nullptr;
			srvs[7] = nullptr;
			constBuffers[2] = m_pCBTAA.Get();
			renderScreenSpacePass(m_pPSTAA.Get(), target->historyRTV(), constBuffers, srvs, samplers);
		}
	}

	void DeferredRenderer::renderPostProcessing(const RenderQueue *scene, RenderTarget *target) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderPostProcessing");

		int pptarget = 1;
		int ppview = 0;
		size_t pass = 0;

		vector<const PostProcessItem*> processItems = { &m_ColorGrading };
		for (auto &pp : scene->m_PostProcessItems) processItems.push_back(&pp);

		for (auto process : processItems)
		{
			ID3D11RenderTargetView* rtvs[] = { target->m_PostBuffer[pptarget].rtv.Get() };
			if (pass == (processItems.size() - 1))
			{
				if (target->m_pOutputTarget)
				{
					rtvs[0] = target->m_pOutputTarget;
					target->m_Handle = nullptr;
				}
				else
				{
					target->m_Handle = target->m_PostBuffer[pptarget].srv.Get();
				}
			}

			ID3D11ShaderResourceView* srvs[8] = { 0 };
			if (process->m_SceneInputSlot < 8)
				srvs[process->m_SceneInputSlot] = target->m_PostBuffer[ppview].srv.Get();
			if (process->m_DepthInputSlot < 8)
				srvs[process->m_DepthInputSlot] = target->m_GraphicsBuffer[RenderTarget::GBuf_DepthStencilIdx].srv.Get();
			if (process->m_NormalsInputSlot < 8)
				srvs[process->m_DepthInputSlot] = target->m_GraphicsBuffer[RenderTarget::GBuf_Graphics1Idx].srv.Get();
			if (process->m_VelocityInputSlot < 8)
				srvs[process->m_VelocityInputSlot] = nullptr;

			for (auto &slot : process->m_InputSlots)
				srvs[slot.first] = (ID3D11ShaderResourceView*)slot.second;

			// take AA result as first pass input if AA is enabled
			if (pass == 0 && m_Config.m_AAEnabled)
			{
				srvs[process->m_SceneInputSlot] = target->currentSRV();
			}

			ID3D11Buffer* constBuffers[] =
			{
				m_pCBScene.Get(),
				nullptr,
			};

			ID3D11SamplerState* samplers[] =
			{
				m_pScreenSampler.Get(),
				m_pGSampler.Get()
			};

			if (process->m_ConstBuffer)
			{
				D3D11_MAPPED_SUBRESOURCE msr;
				THROW_ON_FAIL(context->Map(process->m_ConstBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &msr));
				memcpy(msr.pData, (void*)process->m_ConstBufferData->data(), process->m_ConstBufferData->size());
				context->Unmap(process->m_ConstBuffer.Get(), 0);

				constBuffers[1] = process->m_ConstBuffer.Get();
			}
			else
			{
				constBuffers[1] = nullptr;
			}

			context->OMSetRenderTargets(1, rtvs, nullptr);
			context->PSSetShader(process->m_Handle.Get(), nullptr, 0);
			context->PSSetShaderResources(0, 8, srvs);
			context->PSSetSamplers(0, 2, samplers);
			context->PSSetConstantBuffers(0, 2, constBuffers);

			context->Draw(6, 0);

			std::swap(pptarget, ppview);
			pass++;

			ID3D11ShaderResourceView *nullSRV = nullptr;
			context->PSSetShaderResources(process->m_SceneInputSlot, 1, &nullSRV);
		}
	}
}