#pragma once

#include "utils/objects.h"
#include "utils/types.h"
#include "prog/program.h"
#include "prog/library.h"

class Scene {
public:
	Scene();
	~Scene();
	
	void SwitchMenu(EMenu newMenu, void* dat=nullptr);
	void OnMouseDown();
	void OnMouseUp();
	void OnMouseDrag();
	void OnMouseWheel(int ymov);

	Program* getProgram() const;
	const vector<Object*>& Objects() const;
	Object* FocusedObject() const;			// returns pointer to object focused by keyboard
	
private:
	kptr<Program> program;
	kptr<Library> library;
	vector<Object*> objects;

	uint focObject;			// id of object currently focused by keyboard
	ScrollArea* sliderHold;	// pointer to object currently being dragged by mouse (nullptr if none is being held)

	bool CheckButtonClick(Button* obj);
	bool CheckSliderClick(ScrollArea* obj);
	bool CheckListBoxClick(ListBox* obj);
	bool CheckTileBoxClick(TileBox* obj);
};
