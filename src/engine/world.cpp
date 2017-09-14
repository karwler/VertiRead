#include "world.h"

vector<string> World::args;
Base World::engine;

Base* World::base() {
	return &engine;
}

AudioSys* World::audioSys() {
	return engine.getAudioSys();
}

DrawSys* World::drawSys() {
	return engine.getWindowSys()->getDrawSys();
}

InputSys* World::inputSys() {
	return engine.getInputSys();
}

WindowSys* World::winSys() {
	return engine.getWindowSys();
}

Scene* World::scene() {
	return engine.getScene();
}

Library* World::library() {
	return &engine.getScene()->getLibrary();
}

Program* World::program() {
	return &engine.getScene()->getProgram();
}
