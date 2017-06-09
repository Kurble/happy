#pragma once
#include "VertexTypes.h"

namespace happy
{
	class ParticleBuilder
	{
		VertexParticle model;

	public:
		ParticleBuilder()
		{
			// initialize defaults:
			point(bb::vec3(0, 0, 0));
			rotation(0);
			life(1);
			size(0.1f);
			grow(0.0f);
			wiggle(0.0f);
			velocity(bb::vec3(0, 0, 0));
			friction(0.0f);
			gravity(bb::vec3(0, 0, 0));
			spin(0);
			color(bb::vec4(1, 1, 1, 1));
			colorStops(bb::vec4(0, 0.33f, 0.66f, 1));
			texture(bb::vec2(0, 0), bb::vec2(1, 1));
		}

		ParticleBuilder& point(bb::vec3 p)                 { model.positionRotation.x = p.x, model.positionRotation.y = p.y, model.positionRotation.z = p.z; return *this; }
		ParticleBuilder& rotation(float rotation)          { model.positionRotation.w = rotation; return *this;}
		ParticleBuilder& life(float life)                  { model.lifeSizeGrowWiggle.x = life; return *this; }
		ParticleBuilder& size(float size)                  { model.lifeSizeGrowWiggle.y = size; return *this; }
		ParticleBuilder& grow(float grow)                  { model.lifeSizeGrowWiggle.z = grow; return *this; }
		ParticleBuilder& wiggle(float wiggle)              { model.lifeSizeGrowWiggle.w = wiggle; return *this; }
		ParticleBuilder& velocity(bb::vec3 v)              { model.velocityFriction.x = v.x, model.velocityFriction.y = v.y, model.velocityFriction.z = v.z; return *this; }
		ParticleBuilder& friction(float friction)          { model.velocityFriction.w = friction; return *this; }
		ParticleBuilder& gravity(bb::vec3 g)               { model.gravitySpin = bb::vec4(g.x, g.y, g.z, model.gravitySpin.w); return *this; }
		ParticleBuilder& spin(float spin)                  { model.gravitySpin.w = spin; return *this; }
		ParticleBuilder& color(bb::vec4 color)             { return color1(color).color2(color).color3(color).color4(color); }
		ParticleBuilder& color1(bb::vec4 color)            { model.color1 = color; return *this; }
		ParticleBuilder& color2(bb::vec4 color)            { model.color2 = color; return *this; }
		ParticleBuilder& color3(bb::vec4 color)            { model.color3 = color; return *this; }
		ParticleBuilder& color4(bb::vec4 color)            { model.color4 = color; return *this; }
		ParticleBuilder& colorStops(bb::vec4 s)            { model.stops = s; return *this; }
		ParticleBuilder& texture(bb::vec2 uv, bb::vec2 wh) { model.texCoord = bb::vec4(uv.x, uv.y, wh.x, wh.y); return *this; }

		VertexParticle build()
		{
			VertexParticle result = model;
			result.stops.x = model.lifeSizeGrowWiggle.x - model.stops.x;
			result.stops.y = model.lifeSizeGrowWiggle.x - model.stops.y;
			result.stops.z = model.lifeSizeGrowWiggle.x - model.stops.z;
			result.stops.w = model.lifeSizeGrowWiggle.x - model.stops.w;
			result.color2 = model.color2 - model.color1;
			result.color3 = model.color3 - model.color2;
			result.color4 = model.color4 - model.color3;
			return result;
		}
	};
}
