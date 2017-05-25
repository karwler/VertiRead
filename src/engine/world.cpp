#include "world.h"

Engine World::base;
vector<string> World::args;

Engine* World::engine() {
	return &base;
}

AudioSys* World::audioSys() {
	return base.getAudioSys();
}

InputSys* World::inputSys() {
	return base.getInputSys();
}

WindowSys* World::winSys() {
	return base.getWindowSys();
}

Scene* World::scene() {
	return base.getScene();
}

Library*World::library() {
	return base.getScene()->getLibrary();
}

Program* World::program() {
	return base.getScene()->getProgram();
}
