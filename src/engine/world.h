#pragma once

#include "windowSys.h"

// class that makes accessing stuff easier
class World {
public:
	static DrawSys* drawSys() { return windowSys.getDrawSys(); }
	static InputSys* inputSys() { return windowSys.getInputSys(); }
	static WindowSys* winSys() { return &windowSys; }
	static Scene* scene() { return windowSys.getScene(); }
	static Program* program() { return windowSys.getProgram(); }
	static ProgState* state() { return windowSys.getProgram()->getState(); }
	static const vector<string>& getArgs() { return args; }
	static const string& getArg(sizt id) { return args[id]; }
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
};
