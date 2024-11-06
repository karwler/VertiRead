#pragma once

#include <utility>

template <class T> concept SthndType = std::is_pointer_v<T> || std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>;

template <SthndType T>
struct DefaultHandleClose {
	void operator()(T) const noexcept {}
};

template <SthndType T, class C = DefaultHandleClose<T>, T invalid = T(0)>
class sthandle {
private:
	T hnd = invalid;
	C close;

public:
	sthandle() = default;
	sthandle(sthandle&& sh) noexcept : hnd(sh.hnd) { sh.hnd = invalid; }
	sthandle(T h) noexcept : hnd(h) {}
	template <class... A> sthandle(T h, A&&... carg) noexcept : hnd(h), close(std::forward<A>(carg)...) {}

	~sthandle() {
		if (hnd != invalid)
			close(hnd);
	}

	sthandle& operator=(sthandle&& sh) noexcept {
		if (hnd != invalid)
			close(hnd);
		hnd = sh.hnd;
		sh.hnd = invalid;
		return *this;
	}

	T get() const noexcept { return hnd; }
	operator T() const noexcept { return hnd; }
	T* operator&() noexcept { return &hnd; }
	const T* operator&() const noexcept { return &hnd; }

	T release() noexcept {
		T ret = hnd;
		hnd = invalid;
		return ret;
	}

	void reset() noexcept {
		if (hnd != invalid) {
			close(hnd);
			hnd = invalid;
		}
	}
};
