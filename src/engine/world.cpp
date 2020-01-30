#include "world.h"

#if defined(_WIN32) && defined(NDEBUG)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
		World::setArgs(argc, argv, cwtos);
		LocalFree(argv);
	}
#elif defined(_WIN32) && defined(DEBUG)
int wmain(int argc, wchar** argv) {
	World::setArgs(argc - 1, argv + 1, cwtos);
#else
int main(int argc, char** argv) {
	World::setArgs(argc - 1, argv + 1, stos);
#endif
	return World::winSys()->start();
}
