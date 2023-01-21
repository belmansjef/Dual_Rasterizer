#include "pch.h"

#include "Vector2.h"
#include <cassert>

namespace dae {
	const Vector2 Vector2::UnitX = Vector2{ 1, 0 };
	const Vector2 Vector2::UnitY = Vector2{ 0, 1 };
	const Vector2 Vector2::Zero = Vector2{ 0, 0 };

	Vector2::Vector2(float _x, float _y) : x(_x), y(_y) {}


	Vector2::Vector2(const Vector2& from, const Vector2& to) : x(to.x - from.x), y(to.y - from.y) {}

	float Vector2::Magnitude() const
	{
		return sqrtf(x * x + y * y);
	}

	float Vector2::SqrMagnitude() const
	{
		return x * x + y * y;
	}

	float Vector2::Normalize()
	{
		const float m = Magnitude();
		x /= m;
		y /= m;

		return m;
	}

	Vector2 Vector2::Normalized() const
	{
		const float m = Magnitude();
		return { x / m, y / m};
	}

	/// <summary>
	/// Return intersection (if any) of two lines
	/// </summary>
	/// <param name="v1">Start point first line</param>
	/// <param name="v2">End point first</param>
	/// <param name="p1">Start point second line</param>
	/// <param name="p2">End point second line</param>
	/// <returns></returns>
	// Source: https://rosettacode.org/wiki/Sutherland-Hodgman_polygon_clipping#C++
	Vector2 Vector2::Intersection(const Vector2& v1, const Vector2& v2, const Vector2& p1, const Vector2& p2)
	{
		return ((p1 - p2) * Cross(v1, v2) - (v1 - v2) * Cross(p1, p2)) *
			(1.0f / Cross(v1 - v2, p1 - p2));
	}

	/// <summary>
	/// Used to check if point is to the right side of line
	/// </summary>
	/// <param name="point">The point to be checked</param>
	/// <param name="v1">Start point of the line</param>
	/// <param name="v2">End point of the line</param>
	/// <returns></returns>
	bool Vector2::IsInside(const Vector2& point, const Vector2& v1, const Vector2& v2)
	{
		return (Cross(v1 - v2, point) + Cross(v2, v1)) <= 0.0f;
	}

	float Vector2::Dot(const Vector2& v1, const Vector2& v2)
	{
		return v1.x * v2.x + v1.y * v2.y;
	}

	float Vector2::Cross(const Vector2& v1, const Vector2& v2)
	{
		return v1.x * v2.y - v1.y * v2.x;
	}

#pragma region Operator Overloads
	Vector2 Vector2::operator*(float scale) const
	{
		return { x * scale, y * scale };
	}

	Vector2 Vector2::operator/(float scale) const
	{
		return { x / scale, y / scale };
	}

	Vector2 Vector2::operator+(const Vector2& v) const
	{
		return { x + v.x, y + v.y };
	}

	Vector2 Vector2::operator-(const Vector2& v) const
	{
		return { x - v.x, y - v.y };
	}

	Vector2 Vector2::operator-() const
	{
		return { -x ,-y };
	}

	Vector2& Vector2::operator*=(float scale)
	{
		x *= scale;
		y *= scale;
		return *this;
	}

	Vector2& Vector2::operator/=(float scale)
	{
		x /= scale;
		y /= scale;
		return *this;
	}

	Vector2& Vector2::operator-=(const Vector2& v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	Vector2& Vector2::operator+=(const Vector2& v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	float& Vector2::operator[](int index)
	{
		assert(index <= 1 && index >= 0);
		return index == 0 ? x : y;
	}

	float Vector2::operator[](int index) const
	{
		assert(index <= 1 && index >= 0);
		return index == 0 ? x : y;
	}
#pragma endregion
}