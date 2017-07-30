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
	sptr(const sptr& b) :
		ptr(new T(*b.ptr))
	{}
	~sptr() {
		if (ptr)
			delete ptr;
	}

	T* get() {
		return ptr;
	}
	const T* get() const {
		return ptr;
	}
	T& val() {
		return *ptr;
	}
	const T& val() const {
		return *ptr;
	}

	operator T*() {
		return ptr;
	}
	operator const T*() const {
		return ptr;
	}
	operator T&() {
		return *ptr;
	}
	operator const T&() const {
		return *ptr;
	}

	T* operator->() {
		return ptr;
	}
	const T* operator->() const {
		return ptr;
	}
	T& operator*() {
		return *ptr;
	}
	const T& operator*() const {
		return *ptr;
	}

	T* operator=(T* p) {
		if (ptr)
			delete ptr;
		ptr = p;
		return ptr;
	}
	T* operator=(const T& v) {
		if (!ptr)
			ptr = new T;
		*ptr = v;
		return ptr;
	}
	T* operator=(const sptr& b) {
		if (!ptr)
			ptr = new T;
		*ptr = *b.ptr;
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

	void clear() {
		if (ptr) {
			delete ptr;
			ptr = nullptr;
		}
	}
	T* release() {
		T* t = ptr;
		ptr = nullptr;
		return t;
	}
	
protected:
	T* ptr;
};

}
