#include "world.h"

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
	// set World::args
#else
int main(int argc, char** argv) {
#endif
	for (int i=0; i!=argc; i++)
		World::args.push_back(argv[i]);
	World::engine = new Engine;
	return World::engine->Run();
}
