#include "world.h"
#include "utils/compare.h"

template <class C, class F>
void World::setArgs(int argc, C** argv, F conv) {
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			C* pos;
			for (pos = argv[i] + 1; *pos == '-'; ++pos);
			if (*pos)
				flags.insert(conv(pos));
		} else if (C* pos = argv[i] + (argv[i][0] == '\\' && argv[i][1] == '-'); *pos)
			vals.push_back(conv(pos));
	}
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR lpCmdLine, int) {
	if (lpCmdLine && lpCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).c_str(), &argc)) {
#else
int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR pCmdLine, int) {
	if (pCmdLine && pCmdLine[0])
		if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
			World::setArgs(argc, argv, swtos);
			LocalFree(argv);
		}
#else
int main(int argc, char** argv) {
	World::setArgs(argc, argv, stos);
#endif
	setlocale(LC_CTYPE, "");
#ifdef WITH_ICU
	StrNatCmp::init();
#endif
	int rc = World::winSys()->start();
#ifdef WITH_ICU
	StrNatCmp::free();
#endif
	return rc;
}
