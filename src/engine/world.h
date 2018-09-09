#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
public:
	static FileSys* fileSys() { return windowSys.getFileSys(); }
	static DrawSys* drawSys() { return windowSys.getDrawSys(); }
	static InputSys* inputSys() { return windowSys.getInputSys(); }
	static WindowSys* winSys() { return &windowSys; }
	static Scene* scene() { return windowSys.getScene(); }
	static Program* program() { return windowSys.getProgram(); }
	static ProgState* state() { return windowSys.getProgram()->getState(); }
	static Browser* browser() { return windowSys.getProgram()->getBrowser(); }
	static Settings* sets() { return windowSys.getSets(); }
	static const vector<string>& getArgs() { return args; }
	static const string& getArg(sizt id) { return args[id]; }

	template <class F, class... A>
	static void prun(F func, A... args) {
		return run(program(), func, args...);
	}
	
	template <class F, class... A>
	static void srun(F func, A... args) {
		return run(state(), func, args...);
	}

#ifdef _WIN32
#ifdef _DEBUG
	static void setArgs(int argc, wchar** argv);
#else
	static void setArgs(wchar* argstr);
#endif
#else
	static void setArgs(int argc, char** argv);
#endif
private:
	static WindowSys windowSys;			// the thing on which everything runs
	static vector<string> args;

	template <class C, class F, class... A>
	static void run(C* obj, F func, A... args) {
		if (func)
			(obj->*func)(args...);
	}
};
