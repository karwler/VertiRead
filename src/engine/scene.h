#pragma once

#include "utils/scrollAreas.h"
#include "utils/popups.h"
#include "utils/capturers.h"
#include "prog/library.h"
#include "prog/program.h"

class Scene {
public:
	Scene(const GeneralSettings& SETS=GeneralSettings());
	~Scene();
	
	void SwitchMenu(const vector<Object*>& objs, size_t focObj);
	void ResizeMenu();
	void Tick(float dSec);
	void OnMouseDown(EClick clickType, bool handleHold=true);
	void OnMouseUp();
	void OnMouseWheel(int ymov);

	GeneralSettings Settings() const;
	void LibraryPath(const string& dir);
	void PlaylistsPath(const string& dir);

	Program* getProgram();
	Library* getLibrary();
	vector<Object*> Objects() const;
	Object* FocusedObject() const;		// returns pointer to object focused by keyboard
	ListItem* SelectedButton() const;	// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)
	Popup* getPopup() const;
	void SetPopup(Popup* box);			// use nullptr to close

private:
	Program program;
	Library library;
	GeneralSettings sets;

	vector<Object*> objects;
	kk::sptr<Popup> popup;

	size_t focObject;		// id of object currently focused by keyboard
	ScrollArea* objectHold;	// pointer to object currently being dragged by mouse (nullptr if none is being held)

	void CheckObjectsClick(EClick clickType, bool handleHold);
	void CheckPopupClick();
	bool CheckSliderClick(ScrollArea* obj);
	void CheckListBoxClick(ListBox* obj, EClick clickType);
	void CheckTileBoxClick(TileBox* obj, EClick clickType);
	void CheckReaderBoxClick(ReaderBox* obj, EClick clickType, bool handleHold);
	void CheckPopupSimpleClick(PopupMessage* obj);
	void CheckPopupChoiceClick(PopupChoice* obj);
};
