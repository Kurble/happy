#include "stdafx.h"
#include "AssetLoaders.h"

namespace happy
{
	RenderMesh loadRenderMeshFromObj(RenderingContext *pRenderContext, std::string objPath, std::string albedoMetallicPath, std::string normalRougnessPath)
	{
		ifstream fin(objPath.c_str());

		vector<Vec4>                         positions;
		vector<Vec2>                         texcoords;
		vector<Vec3>                         normals;
		vector<VertexPositionNormalTangentBinormalTexcoord> vertices;
		vector<Index16>                      indices;

		if (!fin.is_open()) throw exception("Unable to open obj file");

		while (!fin.eof())
		{
			switch ((char)fin.get())
			{
			case 'v':
			{
				switch ((char)fin.get())
				{
				case ' ':
				{
					Vec4 v;

					fin >> v.x >> v.y >> v.z;
					fin.ignore(512, '\n');
					v.w = 1;

					 positions.push_back(v);
					break;
				}

				case 't':
				{
					Vec2 vt;
					
					fin >> vt.x;
					if (fin.peek() != '\n')
					{
						fin >> vt.y;
						fin.clear();
					}
					if (fin.peek() != '\n')
					{
						float w;
						fin >> w;
						fin.clear();
					}
					fin.ignore(512, '\n');

					vt.y = 1.0f - vt.y;
					texcoords.push_back(vt);
					break;
				}

				case 'n':
				{
					Vec3 vn;

					fin >> vn.x >> vn.y >> vn.z;
					fin.ignore(512, '\n');

					float length = 1.0f / std::sqrt(vn.x*vn.x + vn.y*vn.y + vn.z*vn.z);
					vn.x *= length;
					vn.y *= length;
					vn.z *= length;

					normals.push_back(vn);
					break;
				}

				default:
				{
					fin.ignore(512, '\n');
				}
				}
				break;
			}

			case 'f':
			{
				fin.ignore(2048, ' ');
				std::stringstream line;

				char cline[2048];
				fin.getline(cline, 2048, '\n');

				line << cline;

				int count = 0;
				Index16 baseIndex = (Index16)vertices.size();

				while (!line.eof())
				{
					char vertex[128];
					line.getline(vertex, 128, ' ');

					int v = 0;
					int vt = 0;
					int vn = 0;

					sscanf_s(vertex, "%d/%d", &v, &vt);
					sscanf_s(vertex, "%d//%d", &v, &vn);
					sscanf_s(vertex, "%d/%d/%d", &v, &vt, &vn);

					if (v)
					{
						vertices.emplace_back();
						if (v)  vertices.back().pos = positions[v - 1];
						if (vt) vertices.back().texcoord = texcoords[vt - 1];
						if (vn) vertices.back().normal = normals[vn - 1];

						count++;
					}
				}


				Vec2 uv1 = (vertices[baseIndex+1].texcoord - vertices[baseIndex].texcoord).normalized() * Vec2(1, -1);
				Vec2 uv2 = (vertices[baseIndex+2].texcoord - vertices[baseIndex].texcoord).normalized() * Vec2(1, -1);

				float uvMatrix[] = 
				{
					 uv2.y, -uv1.y,
					-uv2.x,  uv1.x
				};
				float det = 1.0f / ((uv1.x * uv2.y) - (uv2.x * uv1.y));

				Vec4 pos1 = vertices[baseIndex + 1].pos + (vertices[baseIndex + 0].pos*-1);
				Vec4 pos2 = vertices[baseIndex + 2].pos + (vertices[baseIndex + 0].pos*-1);

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
				
				for (int i = 0; i < count; ++i) 
				{
					vertices[baseIndex + i].tangent = tangent;
					vertices[baseIndex + i].binormal = binormal;
					if (i >= 2)
					{
						indices.push_back(baseIndex);
						indices.push_back(baseIndex + i);
						indices.push_back(baseIndex + (i - 1));
					}
				}

				break;
			}

			case '\n':
			{
				break;
			}

			default:
			{
				fin.ignore(512, '\n');
			}
			}
		}

		RenderMesh mesh;
		mesh.setGeometry<VertexPositionNormalTangentBinormalTexcoord, Index16>(
			pRenderContext, 
			vertices.data(), vertices.size(), 
			indices.data(),  indices.size()
		);
		if (albedoMetallicPath.length()) mesh.setAlbedoRoughnessMap(pRenderContext, loadTextureWIC(pRenderContext, albedoMetallicPath));
		if (normalRougnessPath.length()) mesh.setNormalMetallicMap( pRenderContext, loadTextureWIC(pRenderContext, normalRougnessPath));
		return mesh;
	}
}