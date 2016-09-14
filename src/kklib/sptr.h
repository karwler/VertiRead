#pragma once

namespace kk {

template <typename T>
class sptr {
public:
	sptr(T* p=nullptr) :
		ptr(p)
	{}
	sptr(const T& v) :
		ptr(new T(v))
	{}
	~sptr() {
		if (ptr)
			delete ptr;
	}

	T* get() const {
		return ptr;
	}
	T& val() const {
		return *ptr;
	}

	operator T*() const {
		return ptr;
	}
	operator T&() const {
		return *ptr;
	}

	T* operator->() const {
		return ptr;
	}
	T& operator*() const {
		return *ptr;
	}

	T* reset(T* p=nullptr) {
		if (ptr)
			delete ptr;
		ptr = p;
		return ptr;
	}
	T* operator=(sptr& b) {
		if (ptr)
			delete ptr;
		ptr = b.p;
		return ptr;
	}
	T* operator=(T* p) {
		if (ptr)
			delete ptr;
		ptr = p;
		return ptr;
	}

	void swap(sptr& b) {
		T* tmp = b.ptr;
		b.ptr = ptr;
		ptr = tmp;
	}
	void swap(T* p) {
		T* tmp = p;
		p = ptr;
		ptr = tmp;
	}

	T* release() {
		T* t = ptr;
		ptr = nullptr;
		return t;
	}
private:
	T* ptr;
};

}
