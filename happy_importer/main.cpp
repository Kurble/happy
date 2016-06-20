#include <iostream>
#include <fstream>
#include <vector>

#include <fbxsdk.h>

#include "../stdafx.h"
#include "../VertexTypes.h"

using namespace std;

int importer(string fbxPath, string skinOutPath, string animOutPath);

static Vec4 toVec4(const FbxVector4 &fbx)
{
	return Vec4((float)fbx.mData[0], (float)fbx.mData[1], (float)fbx.mData[2], (float)fbx.mData[3]);
}

static Vec3 toVec3(const FbxVector4 &fbx)
{
	return Vec3((float)fbx.mData[0], (float)fbx.mData[1], (float)fbx.mData[2]);
}

static Vec2 toVec2(const FbxVector2 &fbx)
{
	return Vec2((float)fbx.mData[0], (float)fbx.mData[1]);
}

int main(int argc, char** argv)
{
	if (argc >= 3)
	{
		string exec = argv[0];
		string input = argv[1];
		string skin = "";
		string anim = "";
		for (int o = 2; o < argc; o += 2)
		{
			string option = argv[o];
			if (option == "-s") skin = argv[o + 1];
			if (option == "-a") anim = argv[o + 1];
		}

		importer(input, skin, anim);
	}
	else
	{
		cout << "usage: happy_importer <input> [-s <skin output>] [-a <anim output>]" << endl;
		return 0;
	}
}

void calculateTangents(happy::VertexPositionNormalTangentBinormalTexcoordIndicesWeights *vertices, unsigned count)
{
	Vec2 uv1 = (vertices[1].texcoord - vertices[0].texcoord).normalized() * Vec2(1, -1);
	Vec2 uv2 = (vertices[2].texcoord - vertices[0].texcoord).normalized() * Vec2(1, -1);

	float uvMatrix[] =
	{
		uv2.y, -uv1.y,
		-uv2.x,  uv1.x
	};
	float det = 1.0f / ((uv1.x * uv2.y) - (uv2.x * uv1.y));

	Vec4 pos1 = vertices[1].pos + (vertices[0].pos*-1);
	Vec4 pos2 = vertices[2].pos + (vertices[0].pos*-1);

	float posMatrix[] =
	{
		pos1.x, pos1.y, pos1.z,
		pos2.x, pos2.y, pos2.z
	};

	Vec3 tangent = Vec3(
		det * (uvMatrix[0] * posMatrix[0] + uvMatrix[1] * posMatrix[3]),
		det * (uvMatrix[0] * posMatrix[1] + uvMatrix[1] * posMatrix[4]),
		det * (uvMatrix[0] * posMatrix[2] + uvMatrix[1] * posMatrix[5]));
	Vec3 binormal = Vec3(
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
		v.pos.x *= -1;
		v.pos.y *= -1;
		v.pos.w = 1.0f;
		v.normal = Vec3(0, 0, 0);
		v.tangent = Vec3(0, 0, 0);
		v.binormal = Vec3(0, 0, 0);
		v.texcoord = Vec2(0, 0);
		v.indices[0] = (happy::Index16)-1;
		v.indices[1] = (happy::Index16)-1;
		v.indices[2] = (happy::Index16)-1;
		v.indices[3] = (happy::Index16)-1;
		v.weights = Vec4(0, 0, 0, 0);
		uniqueVertices.push_back(v);
	}

	ofstream fout(skinOut.c_str(), ios::out | ios::binary);

	FbxSkin *skin = (FbxSkin*)mesh->GetDeformer(0, FbxDeformer::eSkin);
	if (skin)
	{
		uint32_t boneCount = (unsigned)skin->GetClusterCount();
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
				FbxAMatrix bindPoseMatrix = cluster->GetLink()->EvaluateGlobalTransform();
				Mat4 m;
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
					if (meshVertex.indices[slot] == (happy::Index16)-1)
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
		uint32_t nope = 0;
		fout.write((const char*)&nope, sizeof(uint32_t));
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
			meshVertex.normal.x *= -1;
			meshVertex.normal.y *= -1;
			if (uvSetName)
			{
				mesh->GetPolygonVertexUV(p, v, uvSetName, v2, um);
				meshVertex.texcoord = toVec2(v2);
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
		fout.write((const char*)&meshVertices[v].pos, sizeof(Vec4));
		fout.write((const char*)&meshVertices[v].normal, sizeof(Vec3));
		fout.write((const char*)&meshVertices[v].tangent, sizeof(Vec3));
		fout.write((const char*)&meshVertices[v].binormal, sizeof(Vec3));
		fout.write((const char*)&meshVertices[v].texcoord, sizeof(Vec2));
		fout.write((const char*)&meshVertices[v].indices[0], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[1], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[2], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].indices[3], sizeof(happy::Index16));
		fout.write((const char*)&meshVertices[v].weights, sizeof(Vec4));
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
		uint32_t boneCount = min((unsigned)((happy::Index16)-1), (unsigned)skin->GetClusterCount());
		uint32_t frameCount = 0;

		vector<FbxNode*> bones;

		float fps = 0;
		for (unsigned boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			FbxCluster *cluster = skin->GetCluster(boneIndex);
			bones.push_back(cluster->GetLink());
			FbxTimeSpan localInterval;
			bones.back()->GetAnimationInterval(localInterval);

			fps = (float)localInterval.GetDuration().GetFrameRate(FbxTime::EMode::eDefaultMode);
			frameCount = max(frameCount, (unsigned)localInterval.GetDuration().GetFrameCount());
		}

		ofstream fout(animOut, ios::out | ios::binary);
		fout.write((const char*)&fps, sizeof(float));
		fout.write((const char*)&frameCount, sizeof(uint32_t));
		fout.write((const char*)&boneCount, sizeof(uint32_t));

		for (unsigned frameIndex = 0; frameIndex < frameCount; ++frameIndex)
		{
			FbxTime time;
			time.SetFrame(frameIndex);

			for (unsigned boneIndex = 0; boneIndex < boneCount; ++boneIndex)
			{
				Mat4 m;
				FbxAMatrix mat = bones[boneIndex]->EvaluateGlobalTransform(time);
				for (int i = 0; i < 16; ++i) m.m[i] = (float)(((double*)mat)[i]);

				fout.write((const char*)&m.m, sizeof(Mat4));
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

int importer(string fbxPath, string skinOut,string animOut)
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