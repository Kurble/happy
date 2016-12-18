#include "../stdafx.h"
#include "../VertexTypes.h"

#include "happy_importer.h"

#include <fbxsdk.h>

using namespace std;

static bb::vec4 toVec4(const FbxVector4 &fbx)
{
	return bb::vec4((float)fbx.mData[0], (float)fbx.mData[1], (float)fbx.mData[2], (float)fbx.mData[3]);
}

static bb::vec3 toVec3(const FbxVector4 &fbx)
{
	return bb::vec3((float)fbx.mData[0], (float)fbx.mData[1], (float)fbx.mData[2]);
}

static bb::vec2 toVec2(const FbxVector2 &fbx)
{
	return bb::vec2((float)fbx.mData[0], (float)fbx.mData[1]);
}

void calculateTangents(happy::VertexPositionNormalTangentBinormalTexcoordIndicesWeights *vertices, unsigned count)
{
	bb::vec2 uv1 = (vertices[1].texcoord - vertices[0].texcoord).normalized() * bb::vec2(1, -1);
	bb::vec2 uv2 = (vertices[2].texcoord - vertices[0].texcoord).normalized() * bb::vec2(1, -1);

	float uvMatrix[] =
	{
		uv2.y, -uv1.y,
		-uv2.x,  uv1.x
	};
	float det = 1.0f / ((uv1.x * uv2.y) - (uv2.x * uv1.y));

	bb::vec4 pos1 = vertices[1].pos + (vertices[0].pos*-1);
	bb::vec4 pos2 = vertices[2].pos + (vertices[0].pos*-1);

	float posMatrix[] =
	{
		pos1.x, pos1.y, pos1.z,
		pos2.x, pos2.y, pos2.z
	};

	bb::vec3 tangent = bb::vec3(
		det * (uvMatrix[0] * posMatrix[0] + uvMatrix[1] * posMatrix[3]),
		det * (uvMatrix[0] * posMatrix[1] + uvMatrix[1] * posMatrix[4]),
		det * (uvMatrix[0] * posMatrix[2] + uvMatrix[1] * posMatrix[5]));
	bb::vec3 binormal = bb::vec3(
		det * (uvMatrix[2] * posMatrix[0] + uvMatrix[3] * posMatrix[3]),
		det * (uvMatrix[2] * posMatrix[1] + uvMatrix[3] * posMatrix[4]),
		det * (uvMatrix[2] * posMatrix[2] + uvMatrix[3] * posMatrix[5]));

	tangent.normalize();
	binormal.normalize();

	for (unsigned i = 0; i < count; ++i)
	{
		vertices[i].tangent = tangent;
		vertices[i].binormal = binormal;
	}
}

void loadSkin(FbxMesh *mesh, string &skinOut)
{
	vector<happy::VertexPositionNormalTangentBinormalTexcoordIndicesWeights> uniqueVertices;
	unsigned controlPointCount = mesh->GetControlPointsCount();
	for (unsigned cp = 0; cp < controlPointCount; ++cp)
	{
		happy::VertexPositionNormalTangentBinormalTexcoordIndicesWeights v;
		v.pos = toVec4(mesh->GetControlPointAt(cp));
		v.pos.w = 1.0f;
		v.normal = bb::vec3(0, 0, 0);
		v.tangent = bb::vec3(0, 0, 0);
		v.binormal = bb::vec3(0, 0, 0);
		v.texcoord = bb::vec2(0, 0);
		v.indices[0] = (happy::Index16) - 1;
		v.indices[1] = (happy::Index16) - 1;
		v.indices[2] = (happy::Index16) - 1;
		v.indices[3] = (happy::Index16) - 1;
		v.weights = bb::vec4(0, 0, 0, 0);
		uniqueVertices.push_back(v);
	}

	ofstream fout;

	FbxSkin *skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (skin)
	{
		uint32_t boneCount = (unsigned)skin->GetClusterCount();

		cout << "Exporting skin with" << boneCount << " bones" << endl;
		if (boneCount == 0)
		{
			cout << "Error: no bones in mesh! Skipping..." << endl;
			return;
		}

		fout.open(skinOut.c_str(), ios::out | ios::binary);
		fout.write((const char*)&boneCount, sizeof(uint32_t));

		for (unsigned boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			if (boneIndex >= ((happy::Index16) - 1))
			{
				cout << "Warning: too many bones! Skipping..." << endl;
				break;
			}

			FbxCluster *cluster = skin->GetCluster(boneIndex);

			{
				FbxAMatrix bindPoseMatrix;
				cluster->GetTransformLinkMatrix(bindPoseMatrix);
				bb::mat4 m;
				for (int i = 0; i < 16; ++i) m.m[i] = (float)((double*)bindPoseMatrix)[i];

				fout.write((const char*)m.m, sizeof(float) * 16);
			}


			int    *boneVertexIndices = cluster->GetControlPointIndices();
			double *boneVertexWeights = cluster->GetControlPointWeights();

			int numIndices = cluster->GetControlPointIndicesCount();
			for (int i = 0; i < numIndices; ++i)
			{
				int vertexIndex = boneVertexIndices[i];
				double vertexWeight = boneVertexWeights[i];

				auto &meshVertex = uniqueVertices.at(vertexIndex);
				for (int slot = 0; slot < 4; ++slot)
				{
					if (meshVertex.indices[slot] == (happy::Index16) - 1)
					{
						meshVertex.indices[slot] = (happy::Index16)boneIndex;
						meshVertex.weights[slot] = (float)vertexWeight;
						break;
					}
					else if (slot == 3)
					{
						cout << "Warning: too many bone influences on vertex #" << vertexIndex << ", result might look weird." << endl;
					}
				}
			}
		}
	}
	else
	{
		return;
	}

	vector<happy::VertexPositionNormalTangentBinormalTexcoordIndicesWeights> meshVertices;
	vector<happy::Index16> meshIndices;

	FbxStringList uvSets;
	mesh->GetUVSetNames(uvSets);
	const char *uvSetName = nullptr;
	if (uvSets.GetCount() > 0)
	{
		uvSetName = uvSets.GetItemAt(0)->mString.Buffer();
	}

	unsigned polys = mesh->GetPolygonCount();
	for (unsigned p = 0; p < polys; ++p)
	{
		unsigned firstIdx = (unsigned)meshVertices.size();
		unsigned vertices = mesh->GetPolygonSize(p);
		for (unsigned v = 0; v < vertices; ++v)
		{
			unsigned uniqueVertexIndex = mesh->GetPolygonVertex(p, v);
			auto meshVertex = uniqueVertices.at(uniqueVertexIndex);

			FbxVector4 v4;
			FbxVector2 v2;
			bool um;

			mesh->GetPolygonVertexNormal(p, v, v4);
			meshVertex.normal = toVec3(v4);
			if (uvSetName)
			{
				mesh->GetPolygonVertexUV(p, v, uvSetName, v2, um);
				meshVertex.texcoord = toVec2(v2);
				meshVertex.texcoord.y = 1.0f - meshVertex.texcoord.y;
			}

			meshVertices.push_back(meshVertex);
		}

		calculateTangents(&meshVertices[firstIdx], vertices);

		for (unsigned tri = 1; tri < vertices - 1; ++tri)
		{
			meshIndices.push_back(firstIdx);
			meshIndices.push_back(firstIdx + tri + 1);
			meshIndices.push_back(firstIdx + tri);
		}
	}

	uint32_t exportVertexCount = (uint32_t)meshVertices.size();
	uint32_t exportIndexCount = (uint32_t)meshIndices.size();

	fout.write((const char*)&exportVertexCount, sizeof(uint32_t));
	for (unsigned v = 0; v < exportVertexCount; ++v)
	{
		fout.write((const char*)&meshVertices[v].pos, sizeof(bb::vec4));
		fout.write((const char*)&meshVertices[v].normal, sizeof(bb::vec3));
		fout.write((const char*)&meshVertices[v].tangent, sizeof(bb::vec3));
		fout.write((const char*)&meshVertices[v].binormal, sizeof(bb::vec3));
		fout.write((const char*)&meshVertices[v].texcoord, sizeof(bb::vec2));
		fout.write((const char*)&meshVertices[v].indices[0], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[1], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[2], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[3], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].weights, sizeof(bb::vec4));
	}

	fout.write((const char*)&exportIndexCount, sizeof(uint32_t));
	for (unsigned i = 0; i < exportVertexCount; ++i)
	{
		fout.write((const char*)&meshIndices[i], sizeof(happy::Index16));
	}
	fout.close();
}

void loadAnim(FbxScene *scene, FbxMesh *mesh, string &animOut)
{
	FbxAnimEvaluator *animEvaluator = scene->GetAnimationEvaluator();
	FbxSkin *skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);

	if (skin)
	{
		uint32_t boneCount = min((unsigned)((happy::Index16) - 1), (unsigned)skin->GetClusterCount());
		uint32_t frameCount = 0;
		float frameCountPrecise = 0;
		float fps = 0;

		vector<FbxNode*> bones;

		auto timeMode = FbxTime::EMode::eFrames30;
		
		for (unsigned boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			FbxCluster *cluster = skin->GetCluster(boneIndex);
			bones.push_back(cluster->GetLink());
			FbxTimeSpan localInterval;
			bones.back()->GetAnimationInterval(localInterval);

			fps = (float)localInterval.GetDuration().GetFrameRate(timeMode);
			frameCount = max(frameCount, (unsigned)localInterval.GetDuration().GetFrameCount(timeMode));
			frameCountPrecise = fmaxf(frameCountPrecise, localInterval.GetDuration().GetFrameCountPrecise(timeMode));
		}

		cout << "Exporting animation with " << boneCount << " bones and " << frameCount << " frames." << endl;

		ofstream fout(animOut, ios::out | ios::binary);
		fout.write((const char*)&fps, sizeof(float));
		fout.write((const char*)&frameCount, sizeof(uint32_t));
		//fout.write((const char*)&frameCountPrecise, sizeof(float));
		fout.write((const char*)&boneCount, sizeof(uint32_t));

		for (unsigned frameIndex = 0; frameIndex < frameCount; ++frameIndex)
		{
			FbxTime time;
			time.SetFrame(frameIndex, timeMode);

			for (unsigned boneIndex = 0; boneIndex < boneCount; ++boneIndex)
			{
				bb::mat4 m;
				FbxAMatrix mat = bones[boneIndex]->EvaluateGlobalTransform(time);
				for (int i = 0; i < 16; ++i) m.m[i] = (float)(((double*)mat)[i]);

				fout.write((const char*)&m.m, sizeof(bb::mat4));
			}
		}
	}
}

void loadNode(FbxScene *scene, FbxNode *fbxNode, string &skinOut, string &animOut)
{
	int numAttributes = fbxNode->GetNodeAttributeCount();
	for (int i = 0; i < numAttributes; i++)
	{
		FbxNodeAttribute *nodeAttributeFbx = fbxNode->GetNodeAttributeByIndex(i);
		FbxNodeAttribute::EType attributeType = nodeAttributeFbx->GetAttributeType();

		cout << "Processing node \"" << fbxNode->GetName() << "\"..." << endl;

		switch (attributeType)
		{
		case FbxNodeAttribute::eMesh:
		{
			if (skinOut.length() > 0) loadSkin((FbxMesh*)nodeAttributeFbx, skinOut);

			if (animOut.length() > 0) loadAnim(scene, (FbxMesh*)nodeAttributeFbx, animOut);
			break;
		}
		}
	}

	// Load the child nodes
	int numChildren = fbxNode->GetChildCount();
	for (int i = 0; i < numChildren; i++)
	{
		loadNode(scene, fbxNode->GetChild(i), skinOut, animOut);
	}
}

int fbxImporter(string fbxPath, string skinOut, string animOut)
{
	FbxManager    *sdk = FbxManager::Create();
	FbxIOSettings *ios = FbxIOSettings::Create(sdk, "");
	sdk->SetIOSettings(ios);

	// TODO configure IOS

	// load the fbx scene into memory
	FbxScene *scene = FbxScene::Create(sdk, "happy scene");
	{
		FbxImporter *imp = FbxImporter::Create(sdk, "");
		if (!imp->Initialize(fbxPath.c_str(), -1, sdk->GetIOSettings()))
		{
			cout << "Failed to initialize FBX sdk importer" << endl;
			cout << "Error: " << imp->GetStatus().GetErrorString() << endl;
			return -1;
		}

		imp->Import(scene);
		imp->Destroy();
	}

	loadNode(scene, scene->GetRootNode(), skinOut, animOut);
	return 0;
}