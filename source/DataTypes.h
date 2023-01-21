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
		bool useFastCulling			{ true  };
		bool useClipping			{ true  };
		bool useNormalMap			{ true  };
		bool visualizeDepthBuffer	{ false };
		bool visualizeBoundingBox	{ false };
		bool useMultiThreading		{ true  };
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
	};
}
