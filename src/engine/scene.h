#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"
#include "utils/capturers.h"
#include "prog/library.h"
#include "prog/program.h"

class Scene {
public:
	Scene();
	~Scene();
	
	void SwitchMenu(EMenu newMenu, void* dat=nullptr);
	void ResizeMenu();
	void Tick();
	void OnMouseDown();
	void OnMouseUp();
	void OnMouseWheel(int ymov);

	Program* getProgram() const;
	Library* getLibrary() const;
	vector<Object*> Objects() const;
	Object* FocusedObject() const;			// returns pointer to object focused by keyboard
	ListItem* SelectedButton() const;		// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)
	Popup* getPopup() const;
	void SetPopup(Popup* box);				// use nullptr to close

private:
	kptr<Program> program;
	kptr<Library> library;

	vector<Object*> objects;
	kptr<Popup> popup;

	uint focObject;			// id of object currently focused by keyboard
	ScrollArea* objectHold;	// pointer to object currently being dragged by mouse (nullptr if none is being held)

	bool CheckSliderClick(ScrollArea* obj);
	bool CheckListBoxClick(ListBox* obj);
	bool CheckTileBoxClick(TileBox* obj);
	bool CheckReaderBoxClick(ReaderBox* obj);
	bool CheckPopupSimpleClick(PopupMessage* obj);
	bool CheckPopupChoiceClick(PopupChoice* obj);
};
