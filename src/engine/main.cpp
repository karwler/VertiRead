#include "world.h"

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	World::args = getWords(wtos(pCmdLine), false);
#else
int main(int argc, char** argv) {
	for (int i=0; i!=argc; i++)
		World::args.push_back(argv[i]);
#endif
	try {
		World::engine = new Engine;
		return World::engine->Run();
	} catch (int err) {
		World::engine->Cleanup();
		cerr << "returning " << err << endl;
		return err;
	}
}
