#pragma once
#include "Math.h"
#include "vector"

namespace dae
{
	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	enum class CullMode { BackFace, FrontFace, None, SIZE = 3 };

	enum class ShadingMode { FinalColor, ObservedArea, Diffuse, Specular, SIZE = 4 };

	struct Vertex_In
	{
		Vector3 position;
		ColorRGB color;
		Vector3 normal;
		Vector3 tangent;
		Vector2 uv;
		Vector3 viewDirection;
	};

	struct Vertex_Out
	{
		Vector4 position;
		ColorRGB color;
		Vector2 uv;
		Vector3 normal;
		Vector3 tangent;
		Vector3 viewDirection;

		static Vertex_Out Interpolate(const std::vector<Vertex_Out>& verts, float w0, float w1, float w2, bool shouldInterpolateDepth = false)
		{
			Vertex_Out interpolatedVertex;

			const float interpolatedViewSpaceDepth
			{
				1.f / (
					((1.f / verts[0].position.w) * w0) +
					((1.f / verts[1].position.w) * w1) +
					((1.f / verts[2].position.w) * w2)
					)
			};

			if (shouldInterpolateDepth)
			{
				interpolatedVertex.position.z =
				{
					(
						((verts[0].position.z / verts[0].position.w) * w0) +
						((verts[1].position.z / verts[1].position.w) * w1) +
						((verts[2].position.z / verts[2].position.w) * w2)
					) * interpolatedViewSpaceDepth
				};

				interpolatedVertex.position.w = interpolatedViewSpaceDepth;
			}


			interpolatedVertex.uv =
			{
				(
					((verts[0].uv / verts[0].position.w) * w0) +
					((verts[1].uv / verts[1].position.w) * w1) +
					((verts[2].uv / verts[2].position.w) * w2)
				) * interpolatedViewSpaceDepth
			};

			interpolatedVertex.normal =
			{
				((
					((verts[0].normal / verts[0].position.w) * w0) +
					((verts[1].normal / verts[1].position.w) * w1) +
					((verts[2].normal / verts[2].position.w) * w2)
				) * interpolatedViewSpaceDepth)
			};

			interpolatedVertex.tangent =
			{
				((
					((verts[0].tangent / verts[0].position.w) * w0) +
					((verts[1].tangent / verts[1].position.w) * w1) +
					((verts[2].tangent / verts[2].position.w) * w2)
				) * interpolatedViewSpaceDepth)
			};

			interpolatedVertex.viewDirection =
			{
				((
					((verts[0].viewDirection / verts[0].position.w) * w0) +
					((verts[1].viewDirection / verts[1].position.w) * w1) +
					((verts[2].viewDirection / verts[2].position.w) * w2)
				) * interpolatedViewSpaceDepth).Normalized()
			};

			return interpolatedVertex;
		}
	};

	enum class FilteringMode
	{
		Point,
		Linear,
		Anisotropic,

		SIZE = 3
	};

	enum class RenderType
	{
		Hardware,
		Software,

		SIZE = 2
	};

	struct RenderInfo
	{
		// Common variables
		RenderType renderType { RenderType::Hardware };
		bool useUniformClear{ false };
		ColorRGB clearColor{};
		bool doRotate{ true };
		bool showFPS{ false };

		// Hardware variables
		FilteringMode textureFiltering { FilteringMode::Point };
		bool renderFireFX{ true };

		// Software variables
		ShadingMode shadingMode{ ShadingMode::FinalColor };
		bool useFastCulling			{ true };
		bool useClipping			{ false };
		bool useNormalMap			{ true };
		bool visualizeDepthBuffer	{ false };
		bool visualizeBoundingBox	{ false };
	};

	struct Bounds
	{
		typedef int OutCode;

		const int INSIDE = 0;
		const int LEFT = 1;
		const int RIGHT = 2;
		const int BOTTOM = 4;
		const int TOP = 8;

		Vector2 min{}, max{};

		Bounds() = default;
		Bounds(Vector2 _min, Vector2 _max)
			: max(_max)
			, min(_min)
		{}

		// Determine where the point is relative to the rect formed by the bounds
		OutCode ComputeOutCode(const Vector2& point) const
		{
			OutCode out_code = 0;

			if (point.x < min.x)
				out_code |= LEFT;
			else if (point.x > max.x)
				out_code |= RIGHT;

			if (point.y < min.y)
				out_code |= BOTTOM;
			else if (point.y > max.y)
				out_code |= TOP;

			return out_code;
		}

		// Clip the given line to the rect formed by the bounds
		// https://en.wikipedia.org/wiki/Cohen%E2%80%93Sutherland_algorithm
		bool LineClip(Vector2& v0, Vector2& v1)
		{
			OutCode outcode0 = ComputeOutCode(v0);
			OutCode outcode1 = ComputeOutCode(v1);
			bool accept = false;

			while (true) {
				if (!(outcode0 | outcode1)) {
					// bitwise OR is 0: both points inside window; trivially accept and exit loop
					accept = true;
					break;
				}
				else if (outcode0 & outcode1) {
					// bitwise AND is not 0: both points share an outside zone (LEFT, RIGHT, TOP,
					// or BOTTOM), so both must be outside window; exit loop (accept is false)
					break;
				}
				else {
					// failed both tests, so calculate the line segment to clip
					// from an outside point to an intersection with clip edge
					float x{}, y{};

					// At least one endpoint is outside the clip rectangle; pick it.
					OutCode outcodeOut = outcode1 > outcode0 ? outcode1 : outcode0;

					// Now find the intersection point;
					if (outcodeOut & TOP) {         // point is above the clip window
						x = v0.x + (v1.x - v0.x) * (max.y - v0.y) / (v1.y - v0.y);
						y = max.y;
					}
					else if (outcodeOut & BOTTOM) { // point is below the clip window
						x = v0.x + (v1.x - v0.x) * (min.y - v0.y) / (v1.y - v0.y);
						y = min.y;
					}
					else if (outcodeOut & RIGHT) {  // point is to the right of clip window
						y = v0.y + (v1.y - v0.y) * (max.x - v0.x) / (v1.x - v0.x);
						x = max.x;
					}
					else if (outcodeOut & LEFT) {   // point is to the left of clip window
						y = v0.y + (v1.y - v0.y) * (min.x - v0.x) / (v1.x - v0.x);
						x = min.x;
					}

					// Now we move outside point to intersection point to clip
					// and get ready for next pass.
					if (outcodeOut == outcode0) {
						v0 = { x,y };
						outcode0 = ComputeOutCode(v0);
					}
					else {
						v1 = { x,y };
						outcode1 = ComputeOutCode(v1);
					}
				}
			}
			return accept;
		}
	};
}
