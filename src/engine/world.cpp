#include "world.h"

kptr<Engine> World::engine;

void World::PrintInfo() {
	cout << "Platform: " << SDL_GetPlatform() << endl;
	cout << "CPU count: " << SDL_GetCPUCount() << " - " << SDL_GetCPUCacheLineSize() << endl;
	cout << "RAM: " << SDL_GetSystemRAM() << "MB" << endl;
	cout << "\nVideo Drivers:" << endl;
	for (int i = 0; i != SDL_GetNumVideoDrivers(); i++)
		cout << SDL_GetVideoDriver(i) << endl;
	cout << "\nRenderers:" << endl;
	for (int i = 0; i != SDL_GetNumRenderDrivers(); i++)
		cout << getRendererName(i) << endl;
	cout << "\nAudio Devices:" << endl;
	for (int i = 0; i != SDL_GetNumAudioDevices(0); i++)
		cout << SDL_GetAudioDeviceName(i, 0) << endl;
	cout << "\nAudio Drivers:" << endl;
	for (int i = 0; i != SDL_GetNumAudioDrivers(); i++)
		cout << SDL_GetAudioDriver(i) << endl;
	cout << endl;
}

AudioSys* World::audioSys() {
	return engine->getAudioSys();
}
InputSys* World::inputSys() {
	return engine->getInputSys();
}
WindowSys* World::winSys() {
	return engine->getWindowSys();
}
Scene* World::scene() {
	return engine->getScene();
}
Program* World::program() {
	return engine->getScene()->getProgram();
}
