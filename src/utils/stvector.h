#pragma once

#include <algorithm>
#include <cassert>

template <class T> concept StvecIterator = !(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>) && requires(T t) { ++t; *t; };

template <class T, size_t N>
class stvector {
public:
	template <class U, size_t C> friend class stvector;

	using value_type = T;

private:
	size_t cnt = 0;
	T vec[N];

public:
	stvector() = default;

	template <size_t C>
	stvector(const stvector<T, C>& sv) :
		cnt(sv.cnt)
	{
		assert(cnt <= N);
		std::copy_n(sv.vec, cnt, vec);
	}

	template <size_t C>
	stvector(stvector<T, C>&& sv) noexcept :
		cnt(sv.cnt)
	{
		assert(cnt <= N);
		std::move(sv.vec, sv.vec + sv.cnt, vec);
	}

	explicit stvector(size_t c) noexcept :
		cnt(c)
	{
		assert(cnt <= N);
	}

	stvector(size_t c, const T& v) :
		cnt(c)
	{
		assert(cnt <= N);
		std::fill_n(vec, c, v);
	}

	template <StvecIterator I>
	stvector(I beg, I end) :
		cnt(end - beg)
	{
		assert(cnt <= N);
		std::copy(beg, end, vec);
	}

	stvector(std::initializer_list<T> il) :
		cnt(il.size())
	{
		assert(cnt <= N);
		std::copy(il.begin(), il.end(), vec);
	}

	template <size_t C>
	stvector& operator=(const stvector<T, C>& st) {
		assert(st.cnt <= N);
		cnt = st.cnt;
		std::copy_n(st.vec, cnt, vec);
		return *this;
	}

	template <size_t C>
	stvector& operator=(stvector<T, C>&& st) noexcept {
		assert(st.cnt <= N);
		cnt = st.cnt;
		std::move(st.vec, st.vec + st.cnt, vec);
		return *this;
	}

	stvector& operator=(std::initializer_list<T> il) {
		assert(il.size() <= N);
		cnt = il.size();
		std::copy(il.begin(), il.end(), vec);
		return *this;
	}

	void assign(size_t c, const T& v) {
		assert(c <= N);
		cnt = c;
		std::fill_n(vec, c, v);
	}

	template <StvecIterator I>
	void assign(I beg, I end) {
		assert(end - beg <= N);
		cnt = end - beg;
		std::copy(beg, end, vec);
	}

	void assign(std::initializer_list<T> il) { assign(il.begin(), il.end()); }

	T& operator[](size_t i) noexcept {
		assert(i < cnt);
		return vec[i];
	}

	const T& operator[](size_t i) const noexcept {
		assert(i < cnt);
		return vec[i];
	}

	T& front() noexcept {
		assert(cnt);
		return vec[0];
	}

	const T& front() const noexcept {
		assert(cnt);
		return vec[0];
	}

	T& back() noexcept {
		assert(cnt);
		return vec[cnt - 1];
	}

	const T& back() const noexcept {
		assert(cnt);
		return vec[cnt - 1];
	}

	T* data() noexcept { return vec; }
	const T* data() const noexcept { return vec; }

	T* begin() noexcept { return vec; }
	const T* begin() const noexcept { return vec; }
	const T* cbegin() const noexcept { return vec; }
	T* end() noexcept { return vec + cnt; }
	const T* end() const noexcept { return vec + cnt; }
	const T* cend() const noexcept { return vec + cnt; }

	std::reverse_iterator<T*> rbegin() noexcept { return std::make_reverse_iterator(vec + cnt); }
	std::reverse_iterator<const T*> rbegin() const noexcept { return std::make_reverse_iterator(vec + cnt); }
	std::reverse_iterator<const T*> crbegin() const noexcept { return std::make_reverse_iterator(vec + cnt); }
	std::reverse_iterator<T*> rend() noexcept { return std::make_reverse_iterator(vec); }
	std::reverse_iterator<const T*> rend() const noexcept { return std::make_reverse_iterator(vec); }
	std::reverse_iterator<const T*> crend() const noexcept { return std::make_reverse_iterator(vec); }

	bool empty() const noexcept { return !cnt; }
	size_t size() const noexcept { return cnt; }
	constexpr size_t max_size() const noexcept { return N; }
	void clear() noexcept { cnt = 0; }

	T* insert(const T* pos, const T& v) {
		assert(cnt + 1 <= N && pos >= vec && pos <= vec + cnt);
		T* it = const_cast<T*>(pos);
		std::move_backward(it, vec + cnt, vec + cnt + 1);
		*it = v;
		++cnt;
		return it;
	}

	T* insert(const T* pos, T&& v) noexcept {
		assert(cnt + 1 <= N && pos >= vec && pos <= vec + cnt);
		T* it = const_cast<T*>(pos);
		std::move_backward(it, vec + cnt, vec + cnt + 1);
		*it = std::move(v);
		++cnt;
		return it;
	}

	T* insert(const T* pos, size_t c, const T& v) {
		assert(cnt + c <= N && pos >= vec && pos <= vec + cnt);
		T* it = const_cast<T*>(pos);
		std::move_backward(it, vec + cnt, vec + cnt + c);
		std::fill_n(it, c, v);
		cnt += c;
		return it;
	}

	template <StvecIterator I>
	T* insert(const T* pos, I beg, I end) {
		assert(cnt + (end - beg) <= N && pos >= vec && pos <= vec + cnt);
		T* it = const_cast<T*>(pos);
		size_t c = end - beg;
		std::move_backward(it, vec + cnt, vec + cnt + c);
		std::copy(beg, end, it);
		cnt += c;
		return it;
	}

	T* insert(const T* pos, std::initializer_list<T> il) { return insert(pos, il.begin(), il.end()); }

	template <class... A>
	T* emplace(const T* pos, A&&... args) {
		assert(cnt + 1 <= N && pos >= vec && pos <= vec + cnt);
		T* it = const_cast<T*>(pos);
		std::move_backward(it, vec + cnt, vec + cnt + 1);
		*it = T(std::forward<A>(args)...);
		++cnt;
		return it;
	}

	T* erase(const T* pos) noexcept {
		assert(cnt);
		T* it = const_cast<T*>(pos);
		std::move(it + 1, vec + cnt, it);
		--cnt;
		return it;
	}

	T* erase(const T* beg, const T* end) noexcept {
		assert(beg >= vec && beg <= vec + cnt && end >= vec && end <= vec + cnt && beg <= end);
		size_t c = end - beg;
		T* ib = const_cast<T*>(beg);
		T* ie = const_cast<T*>(end);
		std::move(ie, vec + cnt, ib);
		cnt -= c;
		return ib;
	}

	void push_back(const T& v) {
		assert(cnt + 1 <= N);
		vec[cnt++] = v;
	}

	void push_back(T&& v) noexcept {
		assert(cnt + 1 <= N);
		vec[cnt++] = std::move(v);
	}

	template <class... A>
	T& emplace_back(A&&... args) {
		assert(cnt + 1 <= N);
		return vec[cnt++] = T(std::forward<A>(args)...);
	}

	void pop_back() noexcept {
		assert(cnt);
		--cnt;
	}

	void resize(size_t c) noexcept {
		assert(c <= N);
		cnt = c;
	}

	void resize(size_t c, const T& v) {
		assert(c <= N);
		if (c > cnt)
			std::fill_n(vec + cnt, c - cnt, v);
		cnt = c;
	}

	template <size_t C>
	void swap(stvector<T, C>& sv) noexcept {
		assert(sv.cnt <= N && cnt <= C);
		T tv[N < C ? N : C];
		size_t tc = cnt;
		std::move(vec, vec + tc, tv);
		cnt = sv.cnt;
		std::move(sv.vec, sv.vec + sv.cnt, vec);
		sv.cnt = tc;
		std::move(tv, tv + tc, sv.vec);
	}
};
