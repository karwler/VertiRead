#include "world.h"

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
#else
int main(int argc, char**) {
#endif
	World::engine = new Engine;
	return World::engine->Run();
}
