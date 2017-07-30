#pragma once\

#include "vec2.h"

namespace kk {

template <typename T>
class grid2 {
private:
	using uint = unsigned int;
	using vec2u = vec2<uint>;

public:
	grid2(const vec2u& size=0) : ptr(nullptr) {
		build(size);
	}
	grid2(uint rows, uint cols) : ptr(nullptr) {
		build(vec2u(cols, rows));
	}
	grid2(const vec2u& size, const T& val) : ptr(nullptr) {
		build(size);
		fill(val);
	}
	grid2(uint rows, uint cols, const T& val) : ptr(nullptr) {
		build(vec2u(cols, rows));
		fill(val);
	}
	grid2(const grid2& grid) : ptr(nullptr) {
		build(grid.dim);
		for (uint i=0; i!=length(); i++)
			ptr[i] = grid.ptr[i];
	}
	~grid2() {
		if (ptr)
			delete[] ptr;
	}

	grid2& operator=(const grid2& g) {
		build(g.dim);
		for (uint i=0; i!=length(); i++)
			ptr[i] = g.ptr[i];
		return *this;
	}
	grid2& operator=(const T& v) {
		fill(v);
		return *this;
	}

	T* begin() {
		return &ptr[0];
	}
	const T* begin() const {
		return &ptr[0];
	}
	T* end() {
		return &ptr[length()];
	}
	const T* end() const {
		return &ptr[length()];
	}

	uint length() const {
		return dim.x * dim.y;
	}
	const vec2u& size() const {
		return dim;
	}
	const T* arr() const {
		return ptr;
	}

	uint id(const vec2u& pos) const {
		return pos.y * dim.x + pos.x;
	}
	uint id(uint row, uint col) const {
		return row * dim.x + col;
	}
	vec2u loc(uint id) const {
		vec2u pos(id, id/dim.x);
		pos.x -= pos.y * dim.x;
		return pos;
	}
	bool find(const T* it, uint& id) const {
		for (uint i=0; i!=length(); i++)
			if (&ptr[i] == it) {
				id = i;
				return true;
			}
		return false;
	}

	T& operator[](uint i) {
		return ptr[i];
	}
	const T& operator[](uint i) const {
		return ptr[i];
	}
	T& operator[](const vec2u& p) {
		return ptr[p.y*dim.x+p.x];
	}
	const T& operator[](const vec2u& p) const {
		return ptr[p.y*dim.x+p.x];
	}

	T& at(uint i) {
		return ptr[i];
	}
	const T& at(uint i) const {
		return ptr[i];
	}
	T& at(const vec2u& pos) {
		return ptr[pos.y*dim.x+pos.x];
	}
	const T& at(const vec2u& pos) const {
		return ptr[pos.y*dim.x+pos.x];
	}
	T& at(uint row, uint col) {
		return ptr[row*dim.x+col];
	}
	const T& at(uint row, uint col) const {
		return ptr[row*dim.x+col];
	}

	void fill(const T& val) {
		for (uint i=0; i!=length(); i++)
			ptr[i] = val;
	}
	void fillRow(uint row, const T& val) {
		for (uint i=row*dim.x; i!=row*dim.x+dim.x; i++)
			ptr[i] = val;
	}
	void fillCol(uint col, const T& val) {
		for (uint i=col; i!=dim.y*dim.x+col; i+=dim.x)
			ptr[i] = val;
	}
	void fillRect(const vec2u& pos, const vec2u& size, const T& val) {
		for (uint i=0; i!=size.x*size.y; i++) {
			uint row = i / size.x;
			ptr[(row+pos.y)*dim.x + i-row*size.x + pos.x] = val;
		}
	}

	void clear() {
		dim = 0;
		if (ptr) {
			delete[] ptr;
			ptr = nullptr;
		}
	}
	bool build(const vec2u& size) {
		if (ptr)
			delete[] ptr;

		dim = size;
		ptr = dim.hasNull() ? nullptr : new T[length()];
		return ptr;
	}
	void resize(const vec2u& size) {
		uint tw = (size.x > dim.x) ? dim.x : size.x;
		uint th = (size.y > dim.y) ? dim.y : size.y;
		uint tl = tw * th;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				tp[i] = ptr[row*dim.x + i-row*tw];
			}
		}

		if (build(size))
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				ptr[row*dim.x + i-row*tw] = tp[i];
			}
		if (tp)
			delete[] tp;
	}

	void rowPre(uint amt=1) {
		uint tl = length();
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[i];
		}

		if (build(vec2u(dim.x, dim.y+amt)))
			for (uint i=0; i!=tl; i++)
				ptr[amt*dim.x+i] = tp[i];
		if (tp)
			delete[] tp;
	}
	void rowApp(uint amt=1) {
		uint tl = length();
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[i];
		}

		if (build(vec2u(dim.x, dim.y+amt)))
			for (uint i=0; i!=tl; i++)
				ptr[i] = tp[i];
		if (tp)
			delete[] tp;
	}
	void rowIns(uint row, uint amt=1) {
		if (row > dim.y)
			row = dim.y;

		uint tll = row * dim.x;
		T* tpl = nullptr;
		if (tll != 0) {
			tpl = new T[tll];
			for (uint i=0; i!=tll; i++)
				tpl[i] = ptr[i];
		}
		uint tlr = (dim.y-row) * dim.x;
		T* tpr = nullptr;
		if (tlr != 0) {
			tpr = new T[tlr];
			for (uint i=0; i!=tlr; i++)
				tpr[i] = ptr[row*dim.x+i];
		}

		if (build(vec2u(dim.x, dim.y+amt))) {
			for (uint i=0; i!=tll; i++)
				ptr[i] = tpl[i];
			for (uint i=0; i!=tlr; i++)
				ptr[(row+amt)*dim.x+i] = tpr[i];
		}
		if (tpl)
			delete[] tpl;
		if (tpr)
			delete[] tpr;
	}

	void rowDelF(uint amt=1) {
		if (amt > dim.y)
			amt = dim.y;

		uint tl = (dim.y - amt) * dim.x;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[amt*dim.x+i];
		}

		if (build(vec2u(dim.x, dim.y-amt)))
			for (uint i=0; i!=tl; i++)
				ptr[i] = tp[i];
		if (tp)
			delete[] tp;
	}
	void rowDelL(uint amt=1) {
		if (amt > dim.y)
			amt = dim.y;

		uint tl = (dim.y - amt) * dim.x;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[i];
		}

		if (build(vec2u(dim.x, dim.y-amt)))
			for (uint i=0; i!=tl; i++)
				ptr[i] = tp[i];
		if (tp)
			delete[] tp;
	}
	void rowDel(uint row, uint amt=1) {
		if (row >= dim.y)
			return;
		if (row + amt > dim.y)
			amt = dim.y - row;

		uint tll = row * dim.x;
		T* tpl = nullptr;
		if (tll != 0) {
			tpl = new T[tll];
			for (uint i=0; i!=tll; i++)
				tpl[i] = ptr[i];
		}
		uint tlr = (dim.y-row-amt) * dim.x;
		T* tpr = nullptr;
		if (tlr != 0) {
			tpr = new T[tlr];
			for (uint i=(row+amt)*dim.x; i!=length(); i++)
				tpr[i] = ptr[i];
		}

		if (build(vec2u(dim.x, dim.y+amt))) {
			for (uint i=0; i!=tll; i++)
				ptr[i] = tpl[i];
			for (uint i=0; i!=tlr; i++)
				ptr[row*dim.x+i] = tpr[i];
		}
		if (tpl)
			delete[] tpl;
		if (tpr)
			delete[] tpr;
	}

	void colPre(uint amt=1) {
		uint tw = dim.x;
		uint tl = tw * dim.y;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[i];
		}

		if (build(vec2u(dim.x+amt, dim.y)))
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				ptr[row*dim.x + i-row*tw + amt] = tp[i];
			}
		if (tp)
			delete[] tp;
	}
	void colApp(uint amt=1) {
		uint tw = dim.x;
		uint tl = tw * dim.y;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++)
				tp[i] = ptr[i];
		}

		if (build(vec2u(dim.x+amt, dim.y)))
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				ptr[row*dim.x + i-row*tw] = tp[i];
			}
		if (tp)
			delete[] tp;
	}
	void colIns(uint col, uint amt=1) {
		if (col > dim.x)
			col = dim.x;

		uint twl = col;
		uint tll = twl * dim.y;
		T* tpl = nullptr;
		if (tll != 0) {
			tpl = new T[tll];
			for (uint i=0; i!=tll; i++) {
				uint row = i / twl;
				tpl[i] = ptr[row*dim.x + i-row*twl];
			}
		}
		uint twr = dim.x - col;
		uint tlr = twr * dim.y;
		T* tpr = nullptr;
		if (tlr != 0) {
			tpr = new T[tlr];
			for (uint i=0; i!=tlr; i++) {
				uint row = i / twr;
				tpr[i] = ptr[row*dim.x + i-row*twr + col];
			}
		}

		if (build(vec2u(dim.x+amt, dim.y))) {
			for (uint i=0; i!=tll; i++) {
				uint row = i / twl;
				ptr[row*dim.x + i-row*twl] = tpl[i];
			}
			for (uint i=0; i!=tlr; i++) {
				uint row = i / twr;
				ptr[row*dim.x + i-row*twr + col+amt] = tpr[i];
			}
		}
		if (tpl)
			delete[] tpl;
		if (tpr)
			delete[] tpr;
	}

	void colDelF(uint amt=1) {
		if (amt > dim.y)
			amt = dim.y;

		uint tw = dim.x - amt;
		uint tl = tw * dim.y;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				tp[i] = ptr[row*dim.x + i-row*tw + amt];
			}
		}

		if (build(vec2u(dim.x-amt, dim.y)))
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				ptr[row*dim.x + i-row*tw] = tp[i];
			}
		if (tp)
			delete[] tp;
	}
	void colDelL(uint amt=1) {
		if (amt > dim.y)
			amt = dim.y;

		uint tw = dim.x - amt;
		uint tl = tw * dim.y;
		T* tp = nullptr;
		if (tl != 0) {
			tp = new T[tl];
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				tp[i] = ptr[row*dim.x + i-row*tw];
			}
		}

		if (build(vec2u(dim.x-amt, dim.y)))
			for (uint i=0; i!=tl; i++) {
				uint row = i / tw;
				ptr[row*dim.x + i-row*tw] = tp[i];
			}
		if (tp)
			delete[] tp;
	}
	void colDel(uint col, uint amt=1) {
		if (col >= dim.x)
			return;
		if (col + amt > dim.x)
			amt = dim.x - col;

		uint twl = col;
		uint tll = twl * dim.y;
		T* tpl = nullptr;
		if (tll != 0) {
			tpl = new T[tll];
			for (uint i=0; i!=tll; i++) {
				uint row = i / twl;
				tpl[i] = ptr[row*dim.x + i-row*twl];
			}
		}

		uint twr = dim.x - col - amt;
		uint tlr = twr * dim.y;
		T* tpr = nullptr;
		if (tlr != 0) {
			tpr = new T[tlr];
			for (uint i=0; i!=tlr; i++) {
				uint row = i / twr;
				tpr[i] = ptr[row*dim.x + i-row*twr + col+amt];
			}
		}

		if (build(vec2u(dim.x-amt, dim.y))) {
			for (uint i=0; i!=tll; i++) {
				uint row = i / twl;
				ptr[row*dim.x + i-row*twl] = tpl[i];
			}
			for (uint i=0; i!=tlr; i++) {
				uint row = i / twr;
				ptr[row*dim.x + i-row*twr + col] = tpr[i];
			}
		}
		if (tpl)
			delete[] tpl;
		if (tpr)
			delete[] tpr;
	}

protected:
	vec2u dim;
	T* ptr;
};

template <typename A, typename B>
bool operator==(const grid2<A>& a, const grid2<B>& b) {
	if (a.size() != b.size())
		return false;

	for (uint i=0; i!=a.length(); i++)
		if (a[i] != b[i])
			return false;
	return true;
}
template <typename A, typename B>
bool operator!=(const grid2<A>& a, const grid2<B>& b) {
	if (a.size() != b.size())
		return true;

	for (uint i=0; i!=a.length(); i++)
		if (a[i] != b[i])
			return true;
	return false;
}

}
