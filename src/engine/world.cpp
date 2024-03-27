#include "world.h"
#include "optional/fontconfig.h"
#include "optional/glib.h"
#include "optional/mupdf.h"
#include "optional/poppler.h"
#include "optional/secret.h"
#include "optional/smbclient.h"
#include "optional/ssh2.h"
#ifdef WITH_ICU
#include "utils/compare.h"
#endif
#ifdef _WIN32
#include <windows.h>
#endif

template <Integer C, class F>
pair<vector<string>, uset<string>> getArgs(int argc, C** argv, F conv) {
	vector<string> vals;
	uset<string> flags;
	for (int i = 1; i < argc; ++i) {
		if (argv[i][0] == '-') {
			C* pos;
			for (pos = argv[i] + 1; *pos == '-'; ++pos);
			if (*pos)
				flags.insert(conv(pos));
		} else if (C* pos = argv[i] + (argv[i][0] == '\\' && argv[i][1] == '-'); *pos)
			vals.push_back(conv(pos));
	}
	return pair(std::move(vals), std::move(flags));
}

#ifdef _WIN32
#ifdef __MINGW32__
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	vector<string> vals;
	uset<string> flags;
#ifdef __MINGW32__
	if (strfilled(lpCmdLine))
		if (int argc; LPWSTR* argv = CommandLineToArgvW(sstow(lpCmdLine).data(), &argc)) {
#else
	if (strfilled(pCmdLine))
		if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
#endif
			std::tie(vals, flags) = getArgs(argc, argv, swtos);
			LocalFree(argv);
		}
#else
int main(int argc, char** argv) {
	auto [vals, flags] = getArgs(argc, argv, [](const char* str) -> string { return str; });
#endif
	setlocale(LC_CTYPE, "");
#ifdef WITH_ICU
	Strcomp::init();
#endif
	int rc = World::winSys()->start(std::move(vals), std::move(flags));
#ifdef WITH_ICU
	Strcomp::free();
#endif
#ifdef CAN_SFTP
	closeLibssh2();
#endif
#ifdef CAN_SMB
	closeSmbclient();
#endif
#ifdef CAN_SECRET
	closeLibsecret();
#endif
#ifdef CAN_MUPDF
	closeMupdf();
#endif
#ifdef CAN_POPPLER
	closePoppler();
#endif
#if defined(CAN_SECRET) || defined(CAN_POPPLER)
	closeGlib();
#endif
#ifdef CAN_FONTCFG
	closeFontconfig();
#endif
	return rc;
}
