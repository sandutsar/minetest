/*
Copyright (C) 2014 sapier
Copyright (C) 2024 grorp, Gregor Parzefall
		<gregor.parzefall@posteo.de>

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

#pragma once

#include "irrlichttypes.h"
#include <IEventReceiver.h>
#include <IGUIButton.h>
#include <IGUIEnvironment.h>
#include <IrrlichtDevice.h>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "itemdef.h"
#include "client/game.h"

using namespace irr;
using namespace irr::core;
using namespace irr::gui;

enum class TapState
{
	None,
	ShortTap,
	LongTap,
};

typedef enum
{
	jump_id = 0,
	crunch_id,
	zoom_id,
	aux1_id,
	after_last_element_id,
	settings_starter_id,
	rare_controls_starter_id,
	fly_id,
	noclip_id,
	fast_id,
	debug_id,
	camera_id,
	range_id,
	minimap_id,
	toggle_chat_id,
	chat_id,
	inventory_id,
	drop_id,
	exit_id,
	joystick_off_id,
	joystick_bg_id,
	joystick_center_id
} touch_gui_button_id;

typedef enum
{
	AHBB_Dir_Top_Bottom,
	AHBB_Dir_Bottom_Top,
	AHBB_Dir_Left_Right,
	AHBB_Dir_Right_Left
} autohide_button_bar_dir;

#define MIN_DIG_TIME_MS 500
#define BUTTON_REPEAT_DELAY 0.2f
#define SETTINGS_BAR_Y_OFFSET 5
#define RARE_CONTROLS_BAR_Y_OFFSET 5

// Our simulated clicks last some milliseconds so that server-side mods have a
// chance to detect them via l_get_player_control.
// If you tap faster than this value, the simulated clicks are of course shorter.
#define SIMULATED_CLICK_DURATION_MS 50

extern const std::string button_image_names[];
extern const std::string joystick_image_names[];

struct button_info
{
	float repeat_counter;
	float repeat_delay;
	EKEY_CODE keycode;
	std::vector<size_t> ids;
	IGUIButton *gui_button = nullptr;
	bool immediate_release;

	enum {
		NOT_TOGGLEABLE,
		FIRST_TEXTURE,
		SECOND_TEXTURE
	} toggleable = NOT_TOGGLEABLE;
	std::vector<std::string> textures;
};

class AutoHideButtonBar
{
public:
	AutoHideButtonBar(IrrlichtDevice *device, IEventReceiver *receiver);

	void init(ISimpleTextureSource *tsrc, const std::string &starter_img, int button_id,
			const v2s32 &UpperLeft, const v2s32 &LowerRight,
			autohide_button_bar_dir dir, float timeout);

	~AutoHideButtonBar();

	// add button to be shown
	void addButton(touch_gui_button_id id, const wchar_t *caption,
			const std::string &btn_image);

	// add toggle button to be shown
	void addToggleButton(touch_gui_button_id id, const wchar_t *caption,
			const std::string &btn_image_1, const std::string &btn_image_2);

	// detect button bar button events
	bool isButton(const SEvent &event);

	// step handler
	void step(float dtime);

	// return whether the button bar is active
	bool active() { return m_active; }

	// deactivate the button bar
	void deactivate();

	// hide the whole button bar
	void hide();

	// unhide the button bar
	void show();

private:
	ISimpleTextureSource *m_texturesource = nullptr;
	irr::video::IVideoDriver *m_driver;
	IGUIEnvironment *m_guienv;
	IEventReceiver *m_receiver;
	button_info m_starter;
	std::vector<std::shared_ptr<button_info>> m_buttons;

	v2s32 m_upper_left;
	v2s32 m_lower_right;

	// show button bar
	bool m_active = false;
	bool m_visible = true;

	// button bar timeout
	float m_timeout = 0.0f;
	float m_timeout_value = 3.0f;
	bool m_initialized = false;
	autohide_button_bar_dir m_dir = AHBB_Dir_Right_Left;
};

class TouchScreenGUI
{
public:
	TouchScreenGUI(IrrlichtDevice *device, IEventReceiver *receiver);
	~TouchScreenGUI();

	void translateEvent(const SEvent &event);
	void applyContextControls(const TouchInteractionMode &mode);

	void init(ISimpleTextureSource *tsrc);

	double getYawChange()
	{
		double res = m_camera_yaw_change;
		m_camera_yaw_change = 0;
		return res;
	}

	double getPitchChange() {
		double res = m_camera_pitch_change;
		m_camera_pitch_change = 0;
		return res;
	}

	/**
	 * Returns a line which describes what the player is pointing at.
	 * The starting point and looking direction are significant,
	 * the line should be scaled to match its length to the actual distance
	 * the player can reach.
	 * The line starts at the camera and ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> getShootline() { return m_shootline; }

	float getMovementDirection() { return m_joystick_direction; }
	float getMovementSpeed() { return m_joystick_speed; }

	void step(float dtime);
	inline void setUseCrosshair(bool use_crosshair) { m_draw_crosshair = use_crosshair; }

	void setVisible(bool visible);
	void hide();
	void show();

	void resetHotbarRects();
	void registerHotbarRect(u16 index, const rect<s32> &rect);
	std::optional<u16> getHotbarSelection();

private:
	bool m_initialized = false;
	IrrlichtDevice *m_device;
	IGUIEnvironment *m_guienv;
	IEventReceiver *m_receiver;
	ISimpleTextureSource *m_texturesource;
	v2u32 m_screensize;
	s32 button_size;
	double m_touchscreen_threshold;
	bool m_visible; // is the whole touch screen gui visible

	std::unordered_map<u16, rect<s32>> m_hotbar_rects;
	std::optional<u16> m_hotbar_selection = std::nullopt;

	// value in degree
	double m_camera_yaw_change = 0.0;
	double m_camera_pitch_change = 0.0;

	/**
	 * A line starting at the camera and pointing towards the selected object.
	 * The line ends on the camera's far plane.
	 * The coordinates do not contain the camera offset.
	 */
	line3d<f32> m_shootline;

	bool m_has_move_id = false;
	size_t m_move_id;
	bool m_move_has_really_moved = false;
	u64 m_move_downtime = 0;
	// m_move_pos stays valid even after m_move_id has been released.
	v2s32 m_move_pos;

	bool m_has_joystick_id = false;
	size_t m_joystick_id;
	bool m_joystick_has_really_moved = false;
	float m_joystick_direction = 0.0f; // assume forward
	float m_joystick_speed = 0.0f; // no movement
	bool m_joystick_status_aux1 = false;
	bool m_fixed_joystick = false;
	bool m_joystick_triggers_aux1 = false;
	bool m_draw_crosshair = false;
	std::shared_ptr<button_info> m_joystick_btn_off = nullptr;
	std::shared_ptr<button_info> m_joystick_btn_bg = nullptr;
	std::shared_ptr<button_info> m_joystick_btn_center = nullptr;

	button_info m_buttons[after_last_element_id];

	// gui button detection
	touch_gui_button_id getButtonID(s32 x, s32 y);

	// gui button by eventID
	touch_gui_button_id getButtonID(size_t eventID);

	// check if a button has changed
	void handleChangedButton(const SEvent &event);

	// initialize a button
	void initButton(touch_gui_button_id id, const rect<s32> &button_rect,
			const std::wstring &caption, bool immediate_release,
			float repeat_delay = BUTTON_REPEAT_DELAY);

	// initialize a joystick button
	std::shared_ptr<button_info> initJoystickButton(touch_gui_button_id id,
			const rect<s32> &button_rect, int texture_id,
			bool visible = true);

	// handle a button event
	void handleButtonEvent(touch_gui_button_id bID, size_t eventID, bool action);

	// handle pressing hotbar items
	bool isHotbarButton(const SEvent &event);

	// handle release event
	void handleReleaseEvent(size_t evt_id);

	// apply joystick status
	void applyJoystickStatus();

	// map to store the IDs and original positions of currently pressed pointers
	std::unordered_map<size_t, v2s32> m_pointer_downpos;
	// map to store the IDs and positions of currently pressed pointers
	std::unordered_map<size_t, v2s32> m_pointer_pos;

	// settings bar
	AutoHideButtonBar m_settings_bar;

	// rare controls bar
	AutoHideButtonBar m_rare_controls_bar;

	v2s32 getPointerPos();
	void emitMouseEvent(EMOUSE_INPUT_EVENT type);
	TouchInteractionMode m_last_mode = TouchInteractionMode_END;
	TapState m_tap_state = TapState::None;

	bool m_dig_pressed = false;
	u64 m_dig_pressed_until = 0;

	bool m_place_pressed = false;
	u64 m_place_pressed_until = 0;
};

extern TouchScreenGUI *g_touchscreengui;
