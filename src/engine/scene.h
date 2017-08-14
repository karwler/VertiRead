#pragma once

#include "utils/scrollAreas.h"
#include "utils/capturers.h"
#include "prog/library.h"
#include "prog/program.h"

// handles more backend UI interactions, works with Objects (UI elements), and contains Program and Library
class Scene {
public:
	Scene(const GeneralSettings& SETS=GeneralSettings());
	~Scene();
	
	void switchMenu(const vector<Object*>& objs);
	void resizeMenu();
	void tick(float dSec);
	void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	void onMouseDown(const vec2i& mPos, ClickType click);
	void onMouseUp(const vec2i& mPos, ClickType click);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();

	Program& getProgram();
	Library& getLibrary();
	const vector<Object*>& getObjects() const;
	Object* getFocObject();
	bool isDraggingSlider(ScrollArea* obj) const;	// whether obj's slider is currently being dragged
	ListItem* selectedButton();		// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)

	Popup* getPopup();
	void setPopup(Popup* newPopup, Capturer* capture=nullptr);

private:
	Program program;
	Library library;

	vector<Object*> objects;	// main objects
	kk::sptr<Popup> popup;		// placeholder for popup and it's objects

	Object* focObject;	// currently focused object (should be object over which the cursor is poistioned)
	ClickStamp stamp;	// data about last mouse click
	bool dragSlider;	// whether a slider is currently being dragged

	void resizeObjects(vector<Object*>& objs);

	void checkScrollAreaItemsClick(const vec2i& mPos, ScrollAreaItems* obj, ClickType click);
	void checkReaderBoxClick(const vec2i& mPos, ReaderBox* obj, ClickType click);
	void checkReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* obj);
};
