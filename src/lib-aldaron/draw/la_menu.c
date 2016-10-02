/* Lib Aldaron --- Copyright (c) 2016 Jeron A. Lau */
/* This file must be distributed with the GNU LESSER GENERAL PUBLIC LICENSE. */
/* DO NOT REMOVE THIS NOTICE */

#include <la_config.h>
#ifdef LA_FEATURE_DISPLAY

#include "JLGRprivate.h"
#include "la_menu.h"
#include "la_text.h"

#define MENU_REDRAW_NONE -1
#define MENU_REDRAW_ALL -2

void la_draw_resize(la_window_t *, uint32_t, uint32_t);

static inline void jlgr_menubar_shadow__(la_menu_t* menu) {
	// Clear Texture.
	jl_gl_clear(menu->window, 0.f, 0.f, 0.f, 0.f);
	// Draw Shadows.
	for(menu->redraw = 0; menu->redraw < 10; menu->redraw++) {
		la_menu_fn_t _draw_icon_;
		if(!(_draw_icon_ = menu->redrawfn[menu->redraw]))
			break;

		// Draw shadow
		la_ro_move(&menu->shadow, (la_v3_t) {
			.895 - (.1 * menu->redraw), 0.005, 0. });
		la_ro_draw(&menu->shadow);
		// Draw Icon
		_draw_icon_(menu);
	}
}

// Run whenever a redraw is needed for an icon.
static void jlgr_menubar_draw_(la_menu_t* menu) {
	if(menu->redraw == MENU_REDRAW_ALL) {
		// If needed, draw shadow.
		jlgr_menubar_shadow__(menu);
	}else if(menu->redraw != MENU_REDRAW_NONE) {
		// Redraw only the selected icon.
		if(menu->redrawfn[menu->redraw])
			((la_menu_fn_t)menu->redrawfn[menu->redraw])(menu);
	}
	// Done.
	menu->redraw = MENU_REDRAW_NONE;
}

void la_menu_init(la_menu_t* menu, la_window_t* window) {
	float shadow_color[] = { 0.f, 0.f, 0.f, .75f };
	int i;

	menu->window = window;

	// Make the shadow vertex object.
	la_ro_plain_rect(window, &menu->shadow, shadow_color, .1f, .1f);
	// Make the icon vertex object.
	la_ro_image_rect(window, &menu->icon, window->textures.icon, .1f, .1f);
	la_ro_change_image(&menu->icon, window->textures.icon, 16, 16,
		JLGR_ID_UNKNOWN, 0);
	// Clear the menubar & make pre-renderer.
	for( i = 0; i < 10; i++) {
		menu->inputfn[i] = NULL;
		menu->redrawfn[i] = NULL;
	}
	// Make the menubar.
	la_ro_rect(window, &menu->menubar, 1.f, .11f);

	la_ro_pr(menu, window, &menu->menubar, (la_fn_t) jlgr_menubar_draw_);
}

static void jlgr_menubar_text__(la_menu_t* menu, float* color, float y,
	const char* text)
{
	la_text(menu->window,
		LA_PXMOVE("%f", "%f") LA_PXSIZE("%f")
		LA_PXCOLOR("%f", "%f", "%f", "%f") "%s",
		.9 - (.1 * menu->redraw), y, .1 / ((double)(strlen(text)-1)),
		color[0], color[1], color[2], color[3], text);
}

static void jlgr_menu_flip_draw__(la_menu_t* menu) {
	la_menu_drawicon(menu, menu->window->textures.icon, JLGR_ID_FLIP_IMAGE);
}

static void jlgr_menu_flip_press__(la_menu_t* menu) {
	if(!menu->window->input.mouse.h && !menu->window->input.touch.h) return;
	if(!menu->window->input.mouse.p && !menu->window->input.touch.p) return;
//
	const void* primary = la_safe_get_pointer(&menu->window->protected
		.functions.primary);
	const void* secondary = la_safe_get_pointer(&menu->window->protected
		.functions.secondary);

	la_safe_set_pointer(&menu->window->protected.functions.primary,
		secondary);
	la_safe_set_pointer(&menu->window->protected.functions.secondary,
		primary);
//	jlgr_notify(menu->window, GMessage[menu->window->sg.cs]);
}

static void la_menu_name_drawa__(la_menu_t* menu) {
	la_menu_drawicon(menu, menu->window->textures.icon, 21);
}

static void la_menu_name_drawb__(la_menu_t* menu) {
	la_menu_drawicon(menu, menu->window->textures.icon, 16);
}

static void la_menu_name_drawc__(la_menu_t* menu) {
	float offset = .9f - (.1f * menu->redraw) + 0.005f;

	la_menu_drawicon(menu, menu->window->textures.icon, 20);
	la_text(menu->window, LA_PXSIZE("0.04") LA_PXMOVE("%f", "0.005") "%s", offset, menu->window->wm.windowTitle[0]);
	la_text(menu->window, LA_PXSIZE("0.04") LA_PXMOVE("%f", "0.055") "%s", offset, menu->window->wm.windowTitle[1]);
}

static void jlgr_menu_slow_draw__(la_menu_t* menu) {
	float color[] = { .5, .5, 1., 1. };
//	char formated[80];

	// Draw the icon based on whether on time or not.
	la_menu_drawicon(menu, menu->window->textures.icon,
		/*menu->window->sg.on_time?*/JLGR_ID_GOOD_IMAGE/*:JLGR_ID_SLOW_IMAGE*/);
	// Report the seconds that passed.
	jlgr_menubar_text__(menu, color, 0., "DRAW");
	jlgr_menubar_text__(menu, color, .05, "MAIN");
}

static void jlgr_menu_slow_loop__(la_menu_t* menu) {
	menu->redraw = menu->cursor;
}

//
// Exported Functions
//

/**
 * Draw the menu bar.  Should be paired with jlgr_menu_loop().
 * @param jlgr: The library context
 * @param resize: Is window is being resized?
**/
void la_menu_draw(la_menu_t* menu, uint8_t resize) {
	if(resize) menu->redraw = MENU_REDRAW_ALL;

	// Pre-Render
	if(menu->redraw != MENU_REDRAW_NONE) {
		la_ro_pr(menu, menu->window, &menu->menubar, (la_fn_t)
			jlgr_menubar_draw_);
	}
	// Draw Pre-Rendered
	la_ro_pr_draw(&menu->menubar, 0);
}

/**
 * Check for menubar input.  Should be paired with la_menu_draw().
 * @param jlgr: The library context
**/
void la_menu_loop(la_menu_t* menu) {
	const float mouse_x = la_safe_get_float(&menu->window->mouse_x);
	const float mouse_y = la_safe_get_float(&menu->window->mouse_y);
	const uint8_t selected = (uint8_t) ((1. - mouse_x) / .1);

	for(menu->cursor = 0; menu->cursor < 10; menu->cursor++) {
		// If A NULL function then, stop looping menubar.
		if( !(menu->inputfn[menu->cursor]) ) break;
		// Run the input loop.
		if(menu->cursor == selected && mouse_y < .1)
			((la_menu_fn_t)menu->inputfn[menu->cursor])(menu);
	}
}

void la_menu_drawicon(la_menu_t* menu, uint32_t tex, uint8_t c) {
	la_v3_t tr = { .9 - (.1 * menu->redraw), 0., 0. };

	la_ro_image_rect(menu->window, &menu->icon, tex, .1f, .1f);
	la_ro_change_image(&menu->icon, tex, 16, 16, c, 0);
	la_ro_move(&menu->icon, tr);
	la_ro_draw(&menu->icon);
}

void la_menu_dont(la_menu_t* menu) { }

/**
 * Add an icon to the menubar
 *
 * @param jlgr: the libary context
 * @param inputfn: The function to run when the icon is / isn't pressed.
 * @param rdr: the function to run when redraw is called.
**/
void la_menu_addicon(la_menu_t* menu, la_menu_fn_t inputfn, la_menu_fn_t rdr) {
	uint8_t i;

	for(i = 0; i < 10; i++) if(!menu->inputfn[i]) break;
	// Set functions for: draw, press, not press
	menu->inputfn[i] = inputfn;
	menu->redrawfn[i] = rdr;
}

/**
 * Add the flip screen icon to the menubar.
 * @param jlgr: the libary context
**/
void la_menu_addicon_flip(la_menu_t* menu) {
	la_menu_addicon(menu, jlgr_menu_flip_press__, jlgr_menu_flip_draw__);	
}

/**
 * Add slowness detector to the menubar.
 * @param jlgr: the libary context
**/
void la_menu_addicon_slow(la_menu_t* menu) {
	la_menu_addicon(menu, jlgr_menu_slow_loop__, jlgr_menu_slow_draw__);
}

/**
 * Add program title to the menubar.
 * @param jlgr: the libary context
**/
void la_menu_addicon_name(la_menu_t* menu) {
	int i;
	la_menu_addicon(menu, la_menu_dont, la_menu_name_drawa__);
	for(i = 0; i < 3; i++)
		la_menu_addicon(menu, la_menu_dont, la_menu_name_drawb__);
	la_menu_addicon(menu, la_menu_dont, la_menu_name_drawc__);
}

#endif
