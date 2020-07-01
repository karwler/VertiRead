#include "world.h"

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
		World::setArgs(argc, argv, cwtos);
		LocalFree(argv);
	}
#else
int main(int argc, char** argv) {
	World::setArgs(argc - 1, argv + 1, stos);
#endif
	return World::winSys()->start();
}
