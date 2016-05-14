#pragma once

#include "RenderMesh.h"

namespace happy
{
	class Resources
	{
	public:
		Resources(const string basePath, RenderingContext *pRenderContext);

		RenderingContext* getContext() const;

		RenderMesh getRenderMesh(std::string objPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		RenderMesh getSkin(std::string skinPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		ComPtr<ID3D11ShaderResourceView> getCubemap(std::string filePath[6]);

		ComPtr<ID3D11ShaderResourceView> getCubemapFolder(std::string folderPath, std::string format);

		ComPtr<ID3D11ShaderResourceView> getTexture(std::string filePath);

		std::string getFilePath(std::string localPath);

	private:
		RenderingContext *m_pRenderContext;
		string m_BasePath;

		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedCubemaps;
		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedTextures;
		vector<pair<string, RenderMesh>> m_CachedRenderMeshes;
	};
}