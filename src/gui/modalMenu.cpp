/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2018 stujones11, Stuart Jones <stujones111@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <cstdlib>
#include "client/renderingengine.h"
#include "modalMenu.h"
#include "gettext.h"
#include "porting.h"
#include "settings.h"

GUIModalMenu::GUIModalMenu(gui::IGUIEnvironment* env, gui::IGUIElement* parent,
	s32 id, IMenuManager *menumgr, bool remap_dbl_click) :
		IGUIElement(gui::EGUIET_ELEMENT, env, parent, id,
				core::rect<s32>(0, 0, 100, 100)),
		m_menumgr(menumgr),
		m_remap_dbl_click(remap_dbl_click)
{
	m_gui_scale = std::max(g_settings->getFloat("gui_scaling"), 0.5f);
	const float screen_dpi_scale = RenderingEngine::getDisplayDensity();
	m_gui_scale *= screen_dpi_scale;

	setVisible(true);
	m_menumgr->createdMenu(this);

	m_doubleclickdetect[0].time = 0;
	m_doubleclickdetect[1].time = 0;

	m_doubleclickdetect[0].pos = v2s32(0, 0);
	m_doubleclickdetect[1].pos = v2s32(0, 0);
}

GUIModalMenu::~GUIModalMenu()
{
	m_menumgr->deletingMenu(this);
}

void GUIModalMenu::allowFocusRemoval(bool allow)
{
	m_allow_focus_removal = allow;
}

bool GUIModalMenu::canTakeFocus(gui::IGUIElement *e)
{
	return (e && (e == this || isMyChild(e))) || m_allow_focus_removal;
}

void GUIModalMenu::draw()
{
	if (!IsVisible)
		return;

	video::IVideoDriver *driver = Environment->getVideoDriver();
	v2u32 screensize = driver->getScreenSize();
	if (screensize != m_screensize_old) {
		m_screensize_old = screensize;
		regenerateGui(screensize);
	}

	drawMenu();
}

/*
	This should be called when the menu wants to quit.

	WARNING: THIS DEALLOCATES THE MENU FROM MEMORY. Return
	immediately if you call this from the menu itself.

	(More precisely, this decrements the reference count.)
*/
void GUIModalMenu::quitMenu()
{
	allowFocusRemoval(true);
	// This removes Environment's grab on us
	Environment->removeFocus(this);
	m_menumgr->deletingMenu(this);
	this->remove();
}

bool GUIModalMenu::DoubleClickDetection(const SEvent &event)
{
	/* The following code is for capturing double-clicks of the mouse button
	 * and translating the double-click into an EET_KEY_INPUT_EVENT event
	 * -- which closes the form -- under some circumstances.
	 *
	 * There have been many github issues reporting this as a bug even though it
	 * was an intended feature.  For this reason, remapping the double-click as
	 * an ESC must be explicitly set when creating this class via the
	 * /p remap_dbl_click parameter of the constructor.
	 */

	if (!m_remap_dbl_click)
		return false;

	if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
		m_doubleclickdetect[0].pos = m_doubleclickdetect[1].pos;
		m_doubleclickdetect[0].time = m_doubleclickdetect[1].time;

		m_doubleclickdetect[1].pos = m_pointer;
		m_doubleclickdetect[1].time = porting::getTimeMs();
	} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
		u64 delta = porting::getDeltaMs(
			m_doubleclickdetect[0].time, porting::getTimeMs());
		if (delta > 400)
			return false;

		double squaredistance = m_doubleclickdetect[0].pos.
			getDistanceFromSQ(m_doubleclickdetect[1].pos);

		if (squaredistance > (30 * 30)) {
			return false;
		}

		SEvent translated{};
		// translate doubleclick to escape
		translated.EventType            = EET_KEY_INPUT_EVENT;
		translated.KeyInput.Key         = KEY_ESCAPE;
		translated.KeyInput.Control     = false;
		translated.KeyInput.Shift       = false;
		translated.KeyInput.PressedDown = true;
		translated.KeyInput.Char        = 0;
		OnEvent(translated);

		return true;
	}

	return false;
}

static bool isChild(gui::IGUIElement *tocheck, gui::IGUIElement *parent)
{
	while (tocheck) {
		if (tocheck == parent) {
			return true;
		}
		tocheck = tocheck->getParent();
	}
	return false;
}

bool GUIModalMenu::preprocessEvent(const SEvent &event)
{
	if (event.EventType == EET_MOUSE_INPUT_EVENT) {
		s32 x = event.MouseInput.X;
		s32 y = event.MouseInput.Y;
		gui::IGUIElement *hovered =
				Environment->getRootGUIElement()->getElementFromPoint(
						core::position2d<s32>(x, y));
		if (!isChild(hovered, this)) {
			if (DoubleClickDetection(event)) {
				return true;
			}
		}
	}
	return false;
}
