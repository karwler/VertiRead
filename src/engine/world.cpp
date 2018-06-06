#include "world.h"
#ifdef _WIN32
#include <windows.h>
#endif

WindowSys World::windowSys;
vector<string> World::args;

#ifdef _WIN32
#ifdef _DEBUG
void World::setArgs(int argc, wchar** argv) {
	args.resize(argc-1);
	for (sizt i=0; i<args.size(); i++)
		args[i] = wtos(argv[1+i]);
}
#else
void World::setArgs(wchar* argstr) {
	int argc;
	LPWSTR* argv = CommandLineToArgvW(argstr, &argc);
	
	args.resize(argc);
	for (sizt i=0; i<args.size(); i++) {
		args[i] = wtos(argv[i]);
		cout << args[i] << endl;
	}
	LocalFree(argv);
}
#endif
#else
void World::setArgs(int argc, char** argv) {
	args.resize(argc-1);
	for (sizt i=0; i<args.size(); i++)
		args[i] = argv[1+i];
}
#endif

#ifdef _WIN32
#ifdef _DEBUG
int wmain(int argc, wchar** argv) {
	World::setArgs(argc, argv);
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::setArgs(pCmdLine);
#endif
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv);
#endif
	return World::winSys()->start();
}
