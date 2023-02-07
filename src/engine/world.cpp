#include "world.h"
#ifdef _WIN32
#include <windows.h>
#endif

template <class C, class F>
void World::setArgs(int argc, C** argv, F conv) {
	vals.resize(argc - 1);
	std::transform(argv + 1, argv + argc, vals.begin(), conv);
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).c_str(), &argc)) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
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
