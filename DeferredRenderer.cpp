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
		, m_BufParticles(pRenderContext->getDevice(), 16384)
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
		sceneCB.timestep = 1 / 60.0f;
		updateConstantBuffer<CBufferScene>(context, m_pCBScene.Get(), sceneCB);

		// Render the scene to the graphics buffer
		renderGeometry(scene, target);

		// Prepare pipeline for screen space rendering
		setScreenSpaceRendering();

		// Perform screen space rendering
		ID3D11RenderTargetView *finalOutput = nullptr;
		renderLighting(scene, target);
		renderAA(scene, target);
		renderPostProcessing(scene, target, &finalOutput);
		renderWidgets(scene, target, finalOutput);
		renderParticles(scene, target, finalOutput);

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
		
		float col[] = { 1, 1, 1, 1 };
		context->ClearDepthStencilView(target->m_pDepthBufferView.Get(), D3D11_CLEAR_DEPTH, 1, 0);
		context->ClearRenderTargetView(target->m_GraphicsBuffer[RenderTarget::GBuf_VelocityIdx].rtv.Get(), col);
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
		// Render dynamic meshes
		//=========================================================

		setDynamicRendering();

		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShader(m_pPSGeometry.Get(), nullptr, 0);

		renderSkinList(scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights);
		for (auto &s : scene->m_SubQueues)
		{
			if (s.first.m_HandleVS.Get()) continue;

			const RenderQueue_Root* sub = &s.second;
			if (sub->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights.empty() && 
				sub->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent.empty()) continue;
			s.first.set(context);

			renderSkinList(sub->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeights);
			renderSkinList(sub->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent);

			s.first.unset(context);
		}

		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShader(m_pPSGeometryAlphaStippled.Get(), nullptr, 0);

		renderSkinList(scene->m_GeometryPositionNormalTangentBinormalTexcoordIndicesWeightsTransparent);

		//=========================================================
		// Render static meshes
		//=========================================================

		setStaticRendering();

		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShader(m_pPSGeometry.Get(), nullptr, 0);

		#define RENDER_STATIC_MESH_LIST(s, X) renderStaticMeshList(s->m_Geometry##X, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		#define RENDER_STATIC_MESH_LIST_TRANS(s, X) renderStaticMeshList(s->m_Geometry##X##Transparent, m_pIL##X.Get(), m_pVS##X.Get(), constBuffers)
		RENDER_STATIC_MESH_LIST(scene, PositionTexcoord);
		RENDER_STATIC_MESH_LIST(scene, PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST(scene, PositionNormalTangentBinormalTexcoord);
		for (auto &s : scene->m_SubQueues)
		{
			const RenderQueue_Root* sub = &s.second;
			if (sub->m_GeometryPositionTexcoord.empty() &&
				sub->m_GeometryPositionNormalTexcoord.empty() &&
				sub->m_GeometryPositionNormalTangentBinormalTexcoord.empty() &&
				sub->m_GeometryPositionTexcoordTransparent.empty() &&
				sub->m_GeometryPositionNormalTexcoordTransparent.empty() &&
				sub->m_GeometryPositionNormalTangentBinormalTexcoordTransparent.empty()) continue;
			s.first.set(context);

			if (s.first.m_HandleVS.Get())
			{
				renderStaticMeshList(sub->m_GeometryPositionNormalTangentBinormalTexcoord, s.first.m_HandleIL.Get(), s.first.m_HandleVS.Get(), constBuffers);
				renderStaticMeshList(sub->m_GeometryPositionNormalTangentBinormalTexcoordTransparent, s.first.m_HandleIL.Get(), s.first.m_HandleVS.Get(), constBuffers);
			}
			else
			{
				RENDER_STATIC_MESH_LIST(sub, PositionTexcoord);
				RENDER_STATIC_MESH_LIST(sub, PositionNormalTexcoord);
				RENDER_STATIC_MESH_LIST(sub, PositionNormalTangentBinormalTexcoord);
				RENDER_STATIC_MESH_LIST_TRANS(sub, PositionTexcoord);
				RENDER_STATIC_MESH_LIST_TRANS(sub, PositionNormalTexcoord);
				RENDER_STATIC_MESH_LIST_TRANS(sub, PositionNormalTangentBinormalTexcoord);
			}

			s.first.unset(context);
		}
		
		context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
		context->PSSetConstantBuffers(0, 3, constBuffers);
		context->PSSetShader(m_pPSGeometryAlphaStippled.Get(), nullptr, 0);

		RENDER_STATIC_MESH_LIST_TRANS(scene, PositionTexcoord);
		RENDER_STATIC_MESH_LIST_TRANS(scene, PositionNormalTexcoord);
		RENDER_STATIC_MESH_LIST_TRANS(scene, PositionNormalTangentBinormalTexcoord);
		
		#undef RENDER_STATIC_MESH_LIST
		#undef RENDER_STATIC_MESH_LIST_TRANS

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
			objectCB.alpha = elem.m_Alpha;
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

			UINT stride = (UINT) elem.m_Mesh.getVertexStride();
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

	void DeferredRenderer::renderWidgets(const RenderQueue *scene, RenderTarget *target, ID3D11RenderTargetView *rtv) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderWidgets");

		bool lines = scene->m_Lines.size() > 0;
		bool tris  = scene->m_Quads.size() > 0 
			      || scene->m_Cones.size() > 0
				  || scene->m_Cubes.size() > 0
			      || scene->m_Spheres.size() > 0;

		if (lines || tris)
		{
			for (int pass = 0; pass < 2; ++pass)
			{
				ID3D11ShaderResourceView* srvs[8] = { 0 };

				context->IASetInputLayout(m_pILPositionColor.Get()); 
				context->VSSetShader(m_pVSWidgetsPositionColor.Get(), nullptr, 0);
				context->VSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
				context->PSSetShader(pass == 1 ? m_pPSOccludedWidgets.Get() : m_pPSWidgets.Get(), nullptr, 0);
				context->PSSetSamplers(0, 1, m_pGSampler.GetAddressOf());
				context->PSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
				context->PSSetShaderResources(0, 8, srvs);
				context->OMSetRenderTargets(1, &rtv, target->m_pDepthBufferView.Get());
				context->OMSetDepthStencilState(pass == 1 ? m_pOccludedWidgetsState.Get() : m_pGBufferDepthStencilState.Get(), 0);
				context->OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);

				if (lines)
				{
					context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
					m_BufLineWidgets.begin(context);
					m_BufLineWidgets.draw(context, scene->m_Lines.data(), scene->m_Lines.size(), 2,
						[](VertexPositionColor* vertices, const RenderQueue::LineWidgetItem* objects, size_t count)
						{
							for (size_t i = 0; i < count; ++i)
							{
								vertices[i * 2 + 0].pos   = objects[i].m_From;
								vertices[i * 2 + 0].color = objects[i].m_Color;
								vertices[i * 2 + 1].pos   = objects[i].m_To;
								vertices[i * 2 + 1].color = objects[i].m_Color;
							}
						}
					);
					m_BufLineWidgets.end(context);
				}
				if (tris)
				{
					context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
					m_BufTriWidgets.begin(context);
					m_BufTriWidgets.draw(context, scene->m_Quads.data(), scene->m_Quads.size(), 12,
						[](VertexPositionColor* vertices, const RenderQueue::QuadWidgetItem* objects, size_t count)
						{
							for (size_t i = 0; i < count; ++i)
							{
								vertices[i * 12 +  0].pos = objects[i].m_V[0];
								vertices[i * 12 +  0].color = objects[i].m_Color;
								vertices[i * 12 +  1].pos = objects[i].m_V[1];
								vertices[i * 12 +  1].color = objects[i].m_Color;
								vertices[i * 12 +  2].pos = objects[i].m_V[2];
								vertices[i * 12 +  2].color = objects[i].m_Color;
								vertices[i * 12 +  3].pos = objects[i].m_V[0];
								vertices[i * 12 +  3].color = objects[i].m_Color;
								vertices[i * 12 +  4].pos = objects[i].m_V[2];
								vertices[i * 12 +  4].color = objects[i].m_Color;
								vertices[i * 12 +  5].pos = objects[i].m_V[3];
								vertices[i * 12 +  5].color = objects[i].m_Color;
								vertices[i * 12 +  6].pos = objects[i].m_V[1];
								vertices[i * 12 +  6].color = objects[i].m_Color;
								vertices[i * 12 +  7].pos = objects[i].m_V[0];
								vertices[i * 12 +  7].color = objects[i].m_Color;
								vertices[i * 12 +  8].pos = objects[i].m_V[2];
								vertices[i * 12 +  8].color = objects[i].m_Color;
								vertices[i * 12 +  9].pos = objects[i].m_V[2];
								vertices[i * 12 +  9].color = objects[i].m_Color;
								vertices[i * 12 + 10].pos = objects[i].m_V[0];
								vertices[i * 12 + 10].color = objects[i].m_Color;
								vertices[i * 12 + 11].pos = objects[i].m_V[3];
								vertices[i * 12 + 11].color = objects[i].m_Color;
							}
						}
					);
				
					m_BufTriWidgets.draw(context, scene->m_Cones.data(), scene->m_Cones.size(), 48,
						[](VertexPositionColor* vertices, const RenderQueue::ConeWidgetItem* objects, size_t count)
						{
							for (size_t i = 0; i < count; ++i)
							{
								bb::vec4 up = (objects[i].m_To - objects[i].m_From).normalized();
								bb::vec4 right = bb::vec3(up.x, up.y, up.z).perpendicular().normalized();
								bb::vec4 forward = bb::vec3(up.x, up.y, up.z).cross(bb::vec3(right.x, right.y, right.z));

								up.w = 0;
								right.w = 0;
								forward.w = 0;

								bb::vec4 light = objects[i].m_Color;
								bb::vec4 dark = light * 0.75f;
								dark.w = light.w;

								for (int j = 0; j < 8; ++j)
								{
									// 8 / 2pi = 1.2732
									float jj = ((j + 0) % 8) / 1.2732f; 
									float kk = ((j + 1) % 8) / 1.2732f;

									vertices[i * 48 + j * 6 + 0].pos   = objects[i].m_To;
									vertices[i * 48 + j * 6 + 0].color = (j%2)?dark:light;
									vertices[i * 48 + j * 6 + 1].pos = objects[i].m_From + ((right * sinf(jj)) + (forward * cosf(jj))) * objects[i].m_Radius;
									vertices[i * 48 + j * 6 + 1].color = (j % 2) ? dark : light;
									vertices[i * 48 + j * 6 + 2].pos = objects[i].m_From + ((right * sinf(kk)) + (forward * cosf(kk))) * objects[i].m_Radius;
									vertices[i * 48 + j * 6 + 2].color = (j % 2) ? dark : light;
									vertices[i * 48 + j * 6 + 3].pos = objects[i].m_From + ((right * sinf(jj)) + (forward * cosf(jj))) * objects[i].m_Radius;
									vertices[i * 48 + j * 6 + 3].color = (j % 2) ? light : dark;
									vertices[i * 48 + j * 6 + 4].pos = objects[i].m_From;
									vertices[i * 48 + j * 6 + 4].color = (j % 2) ? light : dark;
									vertices[i * 48 + j * 6 + 5].pos = objects[i].m_From + ((right * sinf(kk)) + (forward * cosf(kk))) * objects[i].m_Radius;
									vertices[i * 48 + j * 6 + 5].color = (j % 2) ? light : dark;
								}
							}
						}
					);

					m_BufTriWidgets.draw(context, scene->m_Cubes.data(), scene->m_Cubes.size(), 36,
						[](VertexPositionColor* vertices, const RenderQueue::SimpleWidgetItem* objects, size_t count)
						{
							for (size_t i = 0; i < count; ++i)
							{
								bb::vec4 light = objects[i].m_Color;
								bb::vec4 dark = light * 0.75f;
								dark.w = light.w;

								const bb::vec4 &pos = objects[i].m_Pos;
								const float    &size = objects[i].m_Size;

								bb::vec4 cube[8] = 
								{
									{ pos.x - size, pos.y - size, pos.z - size, 1.0f },
									{ pos.x - size, pos.y - size, pos.z + size, 1.0f },
									{ pos.x - size, pos.y + size, pos.z - size, 1.0f },
									{ pos.x - size, pos.y + size, pos.z + size, 1.0f },
									{ pos.x + size, pos.y - size, pos.z - size, 1.0f },
									{ pos.x + size, pos.y - size, pos.z + size, 1.0f },
									{ pos.x + size, pos.y + size, pos.z - size, 1.0f },
									{ pos.x + size, pos.y + size, pos.z + size, 1.0f },
								};

								static const int indices[24] =
								{
									0, 1, 5, 4,
									2, 3, 1, 0,
									6, 7, 3, 2,
									4, 5, 7, 6,
									1, 3, 7, 5,
									2, 0, 4, 6,
								};

								for (int j = 0; j < 6; ++j)
								{
									// 8 / 2pi = 1.2732
									float jj = ((j + 0) % 8) / 1.2732f;
									float kk = ((j + 1) % 8) / 1.2732f;

									vertices[i * 36 + j * 6 + 0].pos = cube[indices[j * 4 + 0]];
									vertices[i * 36 + j * 6 + 0].color = light;
									vertices[i * 36 + j * 6 + 1].pos = cube[indices[j * 4 + 1]];
									vertices[i * 36 + j * 6 + 1].color = light;
									vertices[i * 36 + j * 6 + 2].pos = cube[indices[j * 4 + 2]];
									vertices[i * 36 + j * 6 + 2].color = light;
									vertices[i * 36 + j * 6 + 3].pos = cube[indices[j * 4 + 0]];
									vertices[i * 36 + j * 6 + 3].color = dark;
									vertices[i * 36 + j * 6 + 4].pos = cube[indices[j * 4 + 2]];
									vertices[i * 36 + j * 6 + 4].color = dark;
									vertices[i * 36 + j * 6 + 5].pos = cube[indices[j * 4 + 3]];
									vertices[i * 36 + j * 6 + 5].color = dark;
								}
							}
						}
					);

					m_BufTriWidgets.end(context);
				}
			}
		}
	}

	void DeferredRenderer::renderParticles(const RenderQueue *scene, RenderTarget *target, ID3D11RenderTargetView *rtv) const
	{
		auto context = m_pRenderContext->getContext("DeferredRenderer::renderParticles");

		context->OMSetRenderTargets(1, &rtv, target->m_pDepthBufferViewReadOnly.Get());
		context->OMSetDepthStencilState(m_pGBufferDepthStencilState.Get(), 0);
		context->OMSetBlendState(m_pDecalBlendState.Get(), nullptr, 0xffffffff);

		ID3D11ShaderResourceView* srvs[8] = { 0 };
		ID3D11Buffer* procSOTarget[1] = { m_pParticleVBuffer[(target->m_LastUsedHistoryBuffer + 0) % 2].Get() };
		ID3D11Buffer* procSOSource[1] = { m_pParticleVBuffer[(target->m_LastUsedHistoryBuffer + 1) % 2].Get() };
		ID3D11Buffer* noBuffer[1] = { nullptr };
		UINT strides = (UINT)sizeof(VertexParticle);
		UINT begOffset = 0;
		UINT endOffset = (UINT)-1;

		//=========================================================
		// Process Particles
		//=========================================================
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		context->IASetInputLayout(m_pILParticles.Get());
		context->VSSetShader(m_pVSParticles.Get(), nullptr, 0);
		context->VSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		context->GSSetShader(m_pGSProcParticles.Get(), nullptr, 0);
		context->GSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		context->SOSetTargets(1, procSOTarget, &begOffset);
		context->PSSetShader(nullptr, nullptr, 0);
		
		// new particles
		if (scene->m_Particles.size() > 0)
		{
			m_BufParticles.begin(context);
			m_BufParticles.draw(context, scene->m_Particles.data(), scene->m_Particles.size(), 1,
				[](VertexParticle* vertices, const VertexParticle* objects, size_t count)
				{
					memcpy(vertices, objects, count * sizeof(VertexParticle));
				}
			);
			m_BufParticles.end(context);
		}

		// exisiting particles
		context->IASetVertexBuffers(0, 1, procSOSource, &strides, &begOffset);
		context->DrawAuto();

		//=========================================================
		// Draw Particles
		//=========================================================
		context->SOSetTargets(1, noBuffer, &begOffset);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
		context->IASetVertexBuffers(0, 1, procSOTarget, &strides, &begOffset);
		context->GSSetShader(m_pGSDrawParticles.Get(), nullptr, 0);
		context->GSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		context->PSSetShader(m_pPSParticles.Get(), nullptr, 0);
		context->PSSetShaderResources(0, 8, srvs);
		context->PSSetConstantBuffers(0, 1, m_pCBScene.GetAddressOf());
		context->DrawAuto();

		context->GSSetShader(nullptr, nullptr, 0);
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

	void DeferredRenderer::renderPostProcessing(const RenderQueue *scene, RenderTarget *target, ID3D11RenderTargetView** finalOut) const
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

				if (finalOut) *finalOut = rtvs[0];
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