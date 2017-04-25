#include "stdafx.h"
#include "AssetLoaders.h"

namespace happy
{
	shared_ptr<RenderMesh> loadRenderMeshFromObjFile(RenderingContext *pRenderContext, fs::path objPath)
	{
		ifstream fin(objPath.c_str());

		vector<bb::vec4>                         positions;
		vector<bb::vec2>                         texcoords;
		vector<bb::vec3>                         normals;
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
					bb::vec4 v;

					fin >> v.x >> v.y >> v.z;
					fin.ignore(512, '\n');
					v.w = 1;

					 positions.push_back(v);
					break;
				}

				case 't':
				{
					bb::vec2 vt;
					
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
					bb::vec3 vn;

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


				bb::vec2 uv1 = (vertices[baseIndex+1].texcoord - vertices[baseIndex].texcoord);
				if (uv1.mag() > 0) uv1 = uv1.normalized() * bb::vec2(1, -1);

				bb::vec2 uv2 = (vertices[baseIndex+2].texcoord - vertices[baseIndex].texcoord);
				if (uv2.mag() > 0) uv2 = uv2.normalized() * bb::vec2(1, -1);

				float uvMatrix[] = 
				{
					 uv2.y, -uv1.y,
					-uv2.x,  uv1.x
				};
				float det = 1.0f / ((uv1.x * uv2.y) - (uv2.x * uv1.y));

				bb::vec4 pos1 = vertices[baseIndex + 1].pos + (vertices[baseIndex + 0].pos*-1);
				bb::vec4 pos2 = vertices[baseIndex + 2].pos + (vertices[baseIndex + 0].pos*-1);

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

		shared_ptr<RenderMesh> mesh = make_shared<RenderMesh>();
		mesh->setGeometry<VertexPositionNormalTangentBinormalTexcoord, Index16>(
			pRenderContext, 
			vertices.data(), vertices.size(), 
			indices.data(),  indices.size()
		);
		return mesh;
	}
}