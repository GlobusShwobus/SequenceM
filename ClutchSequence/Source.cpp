#include "SequenceM.h"
#include "Stopwatch.h"
#include <iostream>

template <typename T>
concept IS_MATHMATICAL_VECTOR_T = std::_Is_any_of_v<std::remove_cv_t<T>, short, int, long, long long, float, double, long double>;

template <typename T>
concept IS_MATHMATICAL_T = std::_Is_any_of_v<std::remove_cv_t<T>, short, unsigned short, int, unsigned int, long int, unsigned long, long long, unsigned long long, float, double, long double>;

template <typename T>
	requires IS_MATHMATICAL_VECTOR_T<T>
class Vec2M {

public:
	//CONSTRUCTORS
	constexpr Vec2M()noexcept = default;
	constexpr Vec2M(T X, T Y)noexcept :x(X), y(Y) {}

	template <typename S>
	constexpr Vec2M(const Vec2M<S>& rhs)noexcept :x(rhs.x), y(rhs.y) {}

	template <typename S>
	constexpr Vec2M& operator=(const Vec2M<S>& rhs)noexcept {
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	//OPERATORS
	template <typename S>
	constexpr Vec2M operator+(const Vec2M<S>& rhs)const noexcept {
		return Vec2M(x + rhs.x, y + rhs.y);
	}

	template<typename S>
	constexpr Vec2M& operator+=(const Vec2M<S>& rhs)noexcept {
		x += rhs.x;
		y += rhs.y;
		return *this;
	}

	template <typename S>
	constexpr Vec2M operator-(const Vec2M<S>& rhs)const noexcept {
		return Vec2M(x - rhs.x, y - rhs.y);
	}
	constexpr Vec2M operator-()const noexcept {
		return Vec2M(-x, -y);
	}

	template<typename S>
	constexpr Vec2M& operator-=(const Vec2M<S>& rhs)noexcept {
		x -= rhs.x;
		y -= rhs.y;
		return *this;
	}

	template<typename S>
	constexpr Vec2M operator*(const S scalar)const noexcept {
		return Vec2M(x * scalar, y * scalar);
	}

	template<typename S>
	constexpr Vec2M& operator*=(const S scalar)noexcept {
		x *= scalar;
		y *= scalar;
		return *this;
	}

	template<typename S>
	constexpr Vec2M operator/(const S scalar)const noexcept {
		return Vec2M(x / scalar, y / scalar);
	}

	template<typename S>
	constexpr Vec2M& operator/=(const S scalar)noexcept {
		x /= scalar;
		y /= scalar;
		return *this;
	}

	template<typename S>
	constexpr bool operator ==(const Vec2M<S>& rhs)const noexcept {
		return x == rhs.x && y == rhs.y;
	}

	template <typename S>
	constexpr bool operator!=(const Vec2M<S>& rhs)const noexcept {
		return !(*this == rhs);
	}

public:

	T x = 0;
	T y = 0;
};

template<typename T, typename U> requires IS_MATHMATICAL_T<T>
Vec2M<U> operator*(T scalar, const Vec2M<U>& v)noexcept {
	return Vec2M<U>(v.x * scalar, v.y * scalar);
}

template<typename T, typename U> requires IS_MATHMATICAL_T<T>
Vec2M<U> operator/(T scalar, const Vec2M<U>& v)noexcept {
	return Vec2M<U>(v.x / scalar, v.y / scalar);
}
using vec2s = Vec2M<short>;
using vec2i = Vec2M<int>;
using vec2l = Vec2M<long>;
using vec2ll = Vec2M<long long>;
using vec2f = Vec2M<float>;
using vec2d = Vec2M<double>;
using vec2ld = Vec2M<long double>;


template<typename T> requires IS_MATHMATICAL_VECTOR_T<T>
class Rectangle {
public:
	constexpr Rectangle()noexcept = default;
	constexpr Rectangle(T X, T Y, T W, T H) noexcept :x(X), y(Y), w(W), h(H) {}

	template <typename S, typename U> requires IS_MATHMATICAL_VECTOR_T<U>
	constexpr Rectangle(const Vec2M<S>& pos, U W, U H) noexcept : x(pos.x), y(pos.y), w(W), h(H) {}

	template <typename S, typename U> requires IS_MATHMATICAL_VECTOR_T<U>
	constexpr Rectangle(U X, U Y, const Vec2M<S>& dim) noexcept : x(X), y(Y), w(dim.x), h(dim.y) {}

	template <typename S, typename U>
	constexpr Rectangle(const Vec2M<S>& pos, const Vec2M<U>& dim)noexcept :x(pos.x), y(pos.y), w(dim.x), h(dim.y) {}

	template<typename S>
	constexpr Rectangle(const Rectangle<S>& rhs)noexcept :x(rhs.x), y(rhs.y), w(rhs.w), h(rhs.h) {}

	template<typename S>
	constexpr Rectangle& operator=(const Rectangle<S>& rhs)noexcept {
		x = rhs.x;
		y = rhs.y;
		w = rhs.w;
		h = rhs.h;
		return *this;
	}
	template<typename S>
	constexpr bool operator==(const Rectangle<S>& rhs)const noexcept {
		return w = rhs.w && h = rhs.h;
	}
	template<typename S>
	constexpr bool operator!=(const Rectangle<S>& rhs) const noexcept {
		return!(*this == rhs);
	}

	constexpr float get_half_width()const noexcept {
		return w * 0.5f;
	}
	constexpr float get_half_height()const noexcept {
		return h * 0.5f;
	}
	constexpr float get_center_x_axis()const noexcept {
		return x + get_half_width();
	}
	constexpr float get_center_y_axis()const noexcept {
		return y + get_half_height();
	}

	constexpr vec2f get_center_position()const noexcept {
		return vec2f(get_center_x_axis(), get_center_y_axis());
	}
	constexpr Vec2M<T> get_position()const noexcept {
		return Vec2M<T>(x, y);
	}
	constexpr Vec2M<T> get_dimensions()const noexcept {
		return Vec2M<T>(w, h);
	}

	template<typename S> requires IS_MATHMATICAL_T<S>
	constexpr void set_position(S X, S Y)noexcept {
		x = X;
		y = Y;
	}
	template<typename S> requires IS_MATHMATICAL_T<S>
	constexpr void set_dimensions(S W, S H)noexcept {
		w = W;
		h = H;
	}

public:
	T x = 0;
	T y = 0;
	T w = 0;
	T h = 0;
};

using rectS = Rectangle<short>;
using rectI = Rectangle<int>;
using rectL = Rectangle<long>;
using rectLL = Rectangle<long long>;
using rectF = Rectangle<float>;
using rectD = Rectangle<double>;
using rectLD = Rectangle<long double>;

template <typename T, typename U>
constexpr auto dot_vector(const Vec2M<T>& v1, const Vec2M<U>& v2)noexcept {
	return (v1.x * v2.x) + (v1.y * v2.y);
}

template <typename T>
inline float length_vector(const Vec2M<T>& v)noexcept {
	return std::sqrt(static_cast<float>((v.x * v.x) + (v.y * v.y)));
}
template <typename T>
constexpr vec2d reciprocal_vector(const Vec2M<T>& v)noexcept {
	return vec2d(1.0f / v.x, 1.0f / v.y);
}

template <typename T>
constexpr auto inverse_vector(const Vec2M<T>& v)noexcept {
	return -v;
}

template <typename T>
inline vec2d unit_vector(const Vec2M<T>& v) noexcept {
	const float length = length_vector(v);
	return vec2d(v.x / length, v.y / length);
}

template <typename T>
constexpr vec2i sign_vector(const Vec2M<T>& v)noexcept {
	vec2i sign;

	if (v.x > 0) sign.x = 1;
	else if (v.x < 0) sign.x = -1;

	if (v.y > 0) sign.y = 1;
	else if (v.y < 0) sign.y = -1;

	return sign;
}
template <typename T, typename U> requires IS_MATHMATICAL_T<U>
constexpr bool intersects_rectangle(const Rectangle<T>& rect, U X, U Y)noexcept {
	return (
		X >= rect.x &&
		Y >= rect.y &&
		X < rect.x + rect.w &&
		Y < rect.y + rect.h);
}

template <typename T, typename U>
constexpr bool intersects_rectangle(const Rectangle<T>& rect, Vec2M<U>& pos)noexcept {
	return intersects_rectangle(rect, pos.x, pos.y);
}

template <typename T, typename U>
constexpr bool intersects_rectangle(const Rectangle<T>& a, const Rectangle<U>& b)noexcept {
	return (
		a.x < b.x + b.w &&
		a.x + a.w > b.x &&
		a.y < b.y + b.h &&
		a.y + a.h > b.y
		);
}

template <typename T, typename U, typename S>
constexpr bool intersects_rectangle(const Rectangle<T>& a, const Rectangle<U>& b, rectF& output)noexcept {

	float dx = a.get_center_x_axis() - b.get_center_x_axis();
	float dy = a.get_center_y_axis() - b.get_center_y_axis();

	float overLapX = a.get_half_width() + b.get_half_width() - std::fabs(dx);
	float overLapY = a.get_half_height() + b.get_half_height() - std::fabs(dy);

	if (overLapX < 0.0f && overLapY < 0.0f) return false;

	output = { dx,dy,overLapX, overLapY };

	return true;
}
template <typename T>
	requires IS_MATHMATICAL_VECTOR_T<T>
class Transform {

public:

	constexpr Transform()noexcept = default;

	template<typename S>
	constexpr Transform(Rectangle<S> rectangle, Vec2M<S> vec)noexcept :mBox(std::move(rectangle)), mVelocity(std::move(vec)) {}


	template<typename S>
	constexpr Transform(const Transform<S>& rhs)noexcept :mBox(rhs.mBox), mVelocity(rhs.mVelocity) {}

	template <typename S>
	constexpr Transform& operator=(const Transform<S>& rhs)noexcept {
		mBox = rhs.mBox;
		mVelocity = rhs.mVelocity;
		return *this;
	}
public:
	Rectangle<T> mBox;
	Vec2M<T> mVelocity;
};

int main() {

	Rectangle<double> rec = { 1,1,1,1 };
	Vec2M<double> vec = { 1,1 };

	Transform<double> dt(rectI{ 1,1,1,1 }, vec2i{1,1});
	Transform<int> di(rec, vec);
	Transform<float> df = di;

	//di = dt;

	return 0;
}