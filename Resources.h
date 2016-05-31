#pragma once

#include "RenderMesh.h"
#include "RenderSkin.h"
#include "Animation.h"
#include "TextureHandle.h"

namespace happy
{
	class Resources
	{
	public:
		Resources(const string basePath, RenderingContext *pRenderContext);

		RenderingContext* getContext() const;

		RenderMesh getRenderMesh(std::string objPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		RenderSkin getSkin(std::string skinPath, std::string albedoRoughnessPath, std::string normalMetallicPath);

		Animation getAnimation(std::string animationPath);

		TextureHandle getCubemap(std::string filePath[6]);

		TextureHandle getCubemapFolder(std::string folderPath, std::string format);

		TextureHandle getTexture(std::string filePath);

		std::string getFilePath(std::string localPath);

	private:
		RenderingContext *m_pRenderContext;
		string m_BasePath;

		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedCubemaps;
		vector<pair<string, ComPtr<ID3D11ShaderResourceView>>> m_CachedTextures;
		vector<pair<string, RenderMesh>> m_CachedRenderMeshes;
		vector<pair<string, RenderSkin>> m_CachedRenderSkins;
		vector<pair<string, Animation>> m_CachedAnimations;
	};
}