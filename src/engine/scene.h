#pragma once

#include "utils/scrollAreas.h"
#include "library.h"
#include "prog/program.h"

// handles more backend UI interactions, works with widgets (UI elements), and contains Program and Library
class Scene {
public:
	Scene();
	~Scene();
	
	void switchMenu(const vector<Widget*>& wgts);
	void resizeMenu();
	void tick(float dSec);
	void onMouseMove(const vec2i& mPos, const vec2i& mMov);
	void onMouseDown(const vec2i& mPos, ClickType click);
	void onMouseUp(const vec2i& mPos, ClickType click);
	void onMouseWheel(const vec2i& wMov);
	void onMouseLeave();

	Program& getProgram();
	Library& getLibrary();
	const vector<Widget*>& getWidgets() const;
	Widget* getFocWidget();
	bool isDraggingSlider(ScrollArea* wgt) const;	// whether wgt's slider is currently being dragged
	ListItem* selectedButton();		// find scroll area with selectable items and get the first selected one (returns nullptr if nothing found)

	Popup* getPopup();
	void setPopup(Popup* newPopup, Capturer* capture=nullptr);

private:
	Program program;
	Library library;

	vector<Widget*> widgets;	// main widgets
	kk::sptr<Popup> popup;		// placeholder for popup and it's widgets

	Widget* focWidget;	// currently focused widget (should be widget over which the cursor is poistioned)
	ClickStamp stamp;	// data about last mouse click
	bool dragSlider;	// whether a slider is currently being dragged

	void resizeWidgets(vector<Widget*>& wgts);
	void updateFocWidget();

	void checkScrollAreaItemsClick(const vec2i& mPos, ScrollAreaItems* wgt, ClickType click);
	void checkReaderBoxClick(const vec2i& mPos, ReaderBox* wgt, ClickType click);
	void checkReaderBoxDoubleClick(const vec2i& mPos, ReaderBox* wgt);
};
