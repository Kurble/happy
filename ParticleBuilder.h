#pragma once
#include "VertexTypes.h"

namespace happy
{
	class ParticleBuilder
	{
		VertexParticle model;
		float m_presim;

	public:
		ParticleBuilder()
		{
			// initialize defaults:
			presimulate(0);
			point(bb::vec3(0, 0, 0));
			rotation(0);
			life(1);
			size(0.1f);
			brownian(0, 0, 0);
			velocity(bb::vec3(0, 0, 0));
			friction(0.0f);
			gravity(bb::vec3(0, 0, 0));
			spin(0);
			color(bb::vec4(1, 1, 1, 1));
			colorStops(bb::vec4(0, 0.33f, 0.66f, 1));
			texture(bb::vec2(0, 0), bb::vec2(1, 1));
		}

		ParticleBuilder& point(bb::vec3 p)                 { model.attrPos.x = p.x, model.attrPos.y = p.y, model.attrPos.z = p.z; return *this; }
		ParticleBuilder& rotation(float rotation)          { model.attrPos.w = rotation; return *this;}
		ParticleBuilder& life(float life)                  { model.attr2.w = life; return *this; }
		ParticleBuilder& presimulate(float presim)         { m_presim = presim; return *this; }
		ParticleBuilder& size(float size)                  { size1(size).size2(size).size3(size).size4(size); return *this; }
		ParticleBuilder& size1(float size)                 { model.attr0.x = size; return *this; }
		ParticleBuilder& size2(float size)                 { model.attr0.y = size; return *this; }
		ParticleBuilder& size3(float size)                 { model.attr0.z = size; return *this; }
		ParticleBuilder& size4(float size)                 { model.attr0.w = size; return *this; }
		ParticleBuilder& velocity(bb::vec3 v)              { model.attr1.x = v.x, model.attr1.y = v.y, model.attr1.z = v.z; return *this; }
		ParticleBuilder& friction(float friction)          { model.attr3.w = friction; return *this; }
		ParticleBuilder& gravity(bb::vec3 g)               { model.attr3 = bb::vec4(g.x, g.y, g.z, model.attr3.w); return *this; }
		ParticleBuilder& spin(float spin)                  { model.attr1.w = spin; return *this; }
		ParticleBuilder& brownian(float scale, float speed, float friction) 
		                                                   { model.attr2.x = speed; model.attr2.y = friction; model.attr2.z = scale; return *this; }
		ParticleBuilder& color(bb::vec4 color)             { return color1(color).color2(color).color3(color).color4(color); }
		ParticleBuilder& color1(bb::vec4 color)            { model.attr4 = color; return *this; }
		ParticleBuilder& color2(bb::vec4 color)            { model.attr5 = color; return *this; }
		ParticleBuilder& color3(bb::vec4 color)            { model.attr6 = color; return *this; }
		ParticleBuilder& color4(bb::vec4 color)            { model.attr7 = color; return *this; }
		ParticleBuilder& colorStops(bb::vec4 s)            { model.attr8 = s; return *this; }
		ParticleBuilder& texture(bb::vec2 uv, bb::vec2 wh) { model.tex = bb::vec4(uv.x, uv.y, wh.x, wh.y); return *this; }

		VertexParticle build()
		{
			VertexParticle result = model;
			result.attr8.x = model.attr2.w - model.attr8.x;
			result.attr8.y = model.attr2.w - model.attr8.y;
			result.attr8.z = model.attr2.w - model.attr8.z;
			result.attr8.w = model.attr2.w - model.attr8.w;
			result.attr5 = model.attr5 - model.attr4;
			result.attr6 = model.attr6 - model.attr5;
			result.attr7 = model.attr7 - model.attr6;
			result.attr0.y = model.attr0.y - model.attr0.x;
			result.attr0.z = model.attr0.z - model.attr0.y;
			result.attr0.w = model.attr0.w - model.attr0.z;

			if (m_presim)
			{
				float velocity = bb::vec3{ result.attr1.x, result.attr1.y, result.attr1.z }.length();
				float damping = velocity > 0 ? fmaxf(0.0f, velocity - result.attr1.w * m_presim) / velocity : 1;
				result.attrPos = result.attrPos + result.attr1 * m_presim;
				result.attr1.x = result.attr1.x * damping + result.attr3.x * m_presim;
				result.attr1.y = result.attr1.y * damping + result.attr3.y * m_presim;
				result.attr1.z = result.attr1.z * damping + result.attr3.z * m_presim;
				result.attr2.w -= m_presim;
				result.attr2.x = fmaxf(0, result.attr2.x - result.attr2.y * m_presim);
			}

			return result;
		}
	};
}
