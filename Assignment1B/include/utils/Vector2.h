#pragma once
#include <cmath>
#include <algorithm>

struct Vector2f
{
public:
	Vector2f();
	Vector2f(float newX, float newY);
	explicit Vector2f(float value);

	template<typename Value>
	Vector2f operator* (Value v) const { return Vector2f(x * v, y * v); }
	template<typename Value>
	Vector2f operator/ (Value v) const { return Vector2f(x / v, y / v); }
	Vector2f operator- (const Vector2f& v) const { return Vector2f(x - v.x, y - v.y); }
	Vector2f operator+ (const Vector2f& v) const { return Vector2f(x + v.x, y + v.y); }
	bool operator==(const Vector2f& other) const;
	bool operator!=(const Vector2f& other) const;

	Vector2f normalized() const
	{
		const auto magnitudeValue = magnitude();

		//Check if result is different to 0
		if (magnitudeValue <= 0) return *this;
		return (*this) / magnitudeValue;
	}
	float distance(const Vector2f& other) const { auto dx = other.x - x; auto dy = other.y - y; return std::sqrt(dx * dx + dy * dy); }
	float manhattanDistance(const Vector2f& other) const { return std::abs(other.x - x) + std::abs(other.y - y); }
	float chebyshevDistance(const Vector2f& other) const { auto dx = other.x - x; auto dy = other.y - y; return std::max(std::abs(dx), std::abs(dy)); }


	float magnitude() const { return std::sqrt(x * x + y * y); }

	float x;
	float y;
};

inline Vector2f::Vector2f() : x{ 0 }, y{ 0 }
{
}

inline Vector2f::Vector2f(float newX, float newY) : x{ newX }, y{ newY }
{
}

inline Vector2f::Vector2f(float value) : x{ value }, y{ value }
{
}



inline bool Vector2f::operator==(const Vector2f& other) const {
	return x == other.x && y == other.y;
}

inline bool Vector2f::operator!=(const Vector2f& other) const {
	return !(*this == other);
}
