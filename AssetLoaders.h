#pragma once

#include "RenderingContext.h"
#include "RenderMesh.h"

namespace happy
{
	
	RenderMesh loadRenderMeshFromObj(RenderingContext *pRenderContext, std::string objPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

	ComPtr<ID3D11ShaderResourceView> loadCubemapWIC(RenderingContext *pRenderContext, std::string filePath[6]);

	ComPtr<ID3D11ShaderResourceView> loadCubemapWICFolder(RenderingContext *pRenderContext, std::string folderPath, std::string format);

	ComPtr<ID3D11ShaderResourceView> loadTextureWIC(RenderingContext *pRenderContext, std::string filePath);

}