#pragma once
#include "widget/Widget.hpp"

using namespace std;



namespace rack {

struct CKSSFour : app::SvgSwitch {
	CKSSFour() {
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/CKSSFour_0.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/CKSSFour_1.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/CKSSFour_2.svg")));
		addFrame(APP->window->loadSvg(asset::plugin(pluginInstance,"res/ComponentLibrary/CKSSFour_3.svg")));
		sw->wrap();
		box.size = sw->box.size;
	}
};

} // namespace rack
