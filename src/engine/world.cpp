#include "world.h"
#include <clocale>
#include <locale>

WindowSys World::windowSys;
vector<string> World::vals;
uset<string> World::flags;
umap<string, string> World::opts;

const uset<string> World::checkFlags = {};
const umap<string, string> World::checkOpts = {};
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
	setlocale(LC_ALL, "");
	std::locale::global(std::locale(""));
	std::cout.imbue(std::locale());
	std::cerr.imbue(std::locale());
	return World::winSys()->start();
}
