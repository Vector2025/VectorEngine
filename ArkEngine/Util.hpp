#pragma once

#include <SFML/System/Vector2.hpp>
#include <type_traits>
#include <mutex>
#include <vector>
#include <sstream>
#include "gsl.hpp"
//#include <experimental/generator>

#define log_impl(fmt, ...) printf(__FUNCTION__ ": " fmt "\n", __VA_ARGS__ )
#define log(fmt, ...)  log_impl(fmt, __VA_ARGS__)

#define log_err_impl(fmt, ...) do { log(fmt, __VA_ARGS__); std::exit(EXIT_FAILURE); } while(false)
#define log_err(fmt, ...) log_err_impl(fmt, __VA_ARGS__)

#define INSTANCE(type) type type::instance;

#define COPYABLE(type) \
	type(const type&) = default; \
	type& operator=(const type&) = default;

#define MOVABLE(type) \
	type(type&&) = default; \
	type& operator=(type&&) = default;

struct NonCopyable {
	NonCopyable() = default;
	NonCopyable(const NonCopyable&) = delete;
	NonCopyable& operator=(const NonCopyable&) = delete;
};

struct NonMovable {
	NonMovable() = default;
	NonMovable(NonMovable&&) = delete;
	NonMovable& operator=(NonMovable&&) = delete;
};

template <typename...Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <typename...Ts> overloaded(Ts...)->overloaded<Ts...>;

template <typename T, typename U>
sf::Vector2<T> operator*(sf::Vector2<T> v1, sf::Vector2<U> v2)
{
	return { v1.x * v2.x, v1.y * v2.y };
}

template <typename T, typename U>
sf::Vector2<T> operator/(sf::Vector2<T> v1, sf::Vector2<U> v2)
{
	return { v1.x / v2.x, v1.y / v2.y };
}

template <typename T>
sf::Vector2f centerOfRect(sf::Rect<T> rect)
{
	return { rect.left + rect.width / 2.f, rect.top + rect.height / 2.f };
}

inline std::vector<std::string> splitOnSpace(std::string string)
{
	std::stringstream ss(string);
	std::vector<std::string> v;
	std::string s;
	while (ss >> s)
		v.push_back(s);
	return v;
}

template <typename T, typename U>
inline void erase(T& range, U elem)
{
	range.erase(std::remove(range.begin(), range.end(), elem), range.end());
}

template <typename T, typename F>
inline void erase_if(T& range, F predicate)
{
	range.erase(std::remove_if(range.begin(), range.end(), predicate), range.end());
}

struct PolarVector {
	float magnitude;
	float angle;
};

inline sf::Vector2f normalize(sf::Vector2f vec)
{
	return vec / std::hypot(vec.x, vec.y);
}

inline float toRadians(float deg)
{
	return deg / 180 * 3.14159f;
}

inline float toDegrees(float rad)
{
	return rad / 3.14159f * 180;
}

inline PolarVector toPolar(sf::Vector2f vec)
{
	auto[x, y] = vec;
	auto magnitude = std::sqrt(x * x + y * y);
	auto angle = std::atan2(y, x);
	return { magnitude, angle };
}

inline sf::Vector2f toCartesian(PolarVector vec)
{
	auto[r, fi] = vec;
	auto x = r * std::cos(fi);
	auto y = r * std::sin(fi);
	return { x, y };
}

template <typename T>
class Sync {

public:
	template <typename... Args>
	Sync(Args&&... args) : var(std::forward<Args>(args)...) {  }

	T& var() { return var_; }

	T* operator->()
	{
		std::lock_guard<std::mutex> lk();
		return &var_;
	}

	void lock() { mtx.lock(); }
	void unlock() { mtx.unlock(); }

private:
	std::mutex mtx;
	T var_;
};

/* Params
 * f : function, if is member func then pass 'this' as the next args
 * this: if f is member func
 * v : range with member 'count', represents how many elements in each span
 * spans...
*/
template <typename F, typename...Args>
inline void forEachSpan(F f, Args&&...args)
{
	if constexpr (std::is_member_function_pointer_v<F>)
		forEachSpanImplMemberFunc(f, std::forward<Args>(args)...);
	else 
		forEachSpanImplNormalFunc(f, std::forward<Args>(args)...);
}

template<typename F, typename V, typename...Spans>
inline void forEachSpanImplNormalFunc(F f, V& v, Spans&...spans)
{
	int pos = 0;
	for (auto p : v) {
		size_t count;
		if constexpr (std::is_pointer_v<V::value_type>)
			count = p->count;
		else
			count = p.count;

		std::invoke(
			f,
			*p,
			gsl::span<Spans::value_type>{ spans.data() + pos, count } ...);

		if constexpr (std::is_pointer_v<V::value_type>)
			pos += p->count;
		else
			pos += p.count;
	}
}

template<typename F, class C, typename V, typename...Spans>
inline void forEachSpanImplMemberFunc(F f, C* pThis, V& v, Spans&...spans)
{
	int pos = 0;
	for (auto p : v) {
		size_t count;
		if constexpr (std::is_pointer_v<V::value_type>)
			count = p->count;
		else
			count = p.count;

		std::invoke(
			f,
			pThis,
			*p,
			gsl::span<Spans::value_type>{ spans.data() + pos, count } ...);

		if constexpr (std::is_pointer_v<V::value_type>)
			pos += p->count;
		else
			pos += p.count;
	}
}

#if false
template <typename R, typename... Args>
std::function<R(Args...)> memo(R(*fn)(Args...))
{
	std::map<std::tuple<Args...>, R> table;
	return [fn, table](Args... args) mutable -> R {
		auto argt = std::make_tuple(args...);
		auto memoized = table.find(argt);
		if (memoized == table.end()) {
			auto result = fn(args...);
			table[argt] = result;
			return result;
		} else {
			return memoized->second;
		}
	};
}
#endif