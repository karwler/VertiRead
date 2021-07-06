#include "world.h"
#ifdef _WIN32
#include <windows.h>
#endif

template <class C, class F>
void World::setArgs(int argc, C** argv, F conv) {
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			if (string key = conv(argv[i] + 1); checkOpts.count(key) && i + 1 < argc)
				opts.emplace(key, conv(argv[++i]));
			else if (checkFlags.count(key))
				flags.insert(key);
			else
				vals.push_back(conv(argv[i]));
		} else
			vals.push_back(conv(argv[i]));
	}
	vals.shrink_to_fit();
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).c_str(), &argc)) {
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
		World::setArgs(argc, argv, swtos);
		LocalFree(argv);
	}
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv, stos);
#endif
	return World::winSys()->start();
}
