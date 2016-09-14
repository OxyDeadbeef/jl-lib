/*
 * JL_Lib
 * Copyright (c) 2015 Jeron A. Lau 
*/
/** \file
 * JLGRsg.c
 *	sg AKA. Simple Graphics does the window handling.
**/
#include "JLGRprivate.h"
#include "SDL_image.h"
#include "la_buffer.h"

// SG Prototypes
void jl_gl_draw_prendered(la_window_t* jlgr, jl_vo_t* pv);

// Constants
	//ALL IMAGES: 1024x1024
	#define TEXTURE_WH 1024*1024 
	//1bpp Bitmap = 1048832 bytes (Color Key(256)*RGBA(4), 1024*1024)
	#define IMG_FORMAT_LOW 1048832 
	//2bpp HQ bitmap = 2097664 bytes (Color Key(256*2=512)*RGBA(4), 2*1024*1024)
	#define IMG_FORMAT_MED 2097664
	//3bpp Picture = 3145728 bytes (No color key, RGB(3)*1024*1024)
	#define IMG_FORMAT_PIC 3145728
	//
	#define IMG_SIZE_LOW (1+strlen(JL_IMG_HEADER)+(256*4)+(1024*1024)+1)
	
//Functions:

//Get a pixels RGBA values from a surface and xy
uint32_t _jl_sg_gpix(/*in */ SDL_Surface* surface, int32_t x, int32_t y) {
	int32_t bpp = surface->format->BytesPerPixel;
	uint8_t *p = (uint8_t *)surface->pixels + (y * surface->pitch) + (x * bpp);
	uint32_t color_orig;
	uint32_t color;
	uint8_t* out_color = (void*)&color;

	if(bpp == 1) {
		color_orig = *p;
	}else if(bpp == 2) {
		color_orig = *(uint16_t *)p;
	}else if(bpp == 3) {
		if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
			color_orig = p[0] << 16 | p[1] << 8 | p[2];
		else
			color_orig = p[0] | p[1] << 8 | p[2] << 16;
	}else{ // 4
		color_orig = *(uint32_t *)p;
	}
	SDL_GetRGBA(color_orig, surface->format, &(out_color[0]),
		&(out_color[1]), &(out_color[2]), &(out_color[3]));
	return color;
}

SDL_Surface* la_window_makesurface(la_window_t* jlgr, data_t* data) {
	SDL_Surface *image;
	SDL_RWops *rw;

	la_print("File Size = %d", data->size);
	rw = SDL_RWFromMem(data->data + data->curs, data->size);
	if ((image = IMG_Load_RW(rw, 1)) == NULL)
		la_panic("Couldn't load image: %s", IMG_GetError());

	return image;
}

void _jl_sg_load_jlpx(la_window_t* jlgr,data_t* data,void **pixels,int *w,int *h) {
	SDL_Surface *image;
	uint32_t color = 0;
	data_t pixel_data;
	int i, j;

	if(data->data[0] == 0) la_panic("NO DATA!");

	image = la_window_makesurface(jlgr, data);
	// Covert SDL_Surface.
	jl_data_init(jlgr->jl, &pixel_data, image->w * image->h * 4);
	for(i = 0; i < image->h; i++) {
		for(j = 0; j < image->w; j++) {
			color = _jl_sg_gpix(image, j, i);
			jl_data_saveto(&pixel_data, 4, &color);
		}
	}
	//Set Return values
	*pixels = la_buffer_tostring(&pixel_data);
	*w = image->w;
	*h = image->h;
	// Clean-up
	SDL_free(image);
}

//Load the images in the image file
static inline uint32_t jl_sg_add_image__(la_window_t* jlgr, data_t* data) {
	void *fpixels = NULL;
	int fw;
	int fh;
	uint32_t rtn;

	la_print("size = %d", data->size);
//load textures
	_jl_sg_load_jlpx(jlgr, data, &fpixels, &fw, &fh);
	la_print("creating image....");
	rtn = jl_gl_maketexture(jlgr, fpixels, fw, fh, 0);
	la_print("created image!");
	return rtn;
}

/**
 * Load an image from a zipfile.
 * @param jlgr: The library context
 * @param zipdata: data for a zip file.
 * @param filename: Name of the image file in the package.
 * @returns: Texture object.
*/
uint32_t jl_sg_add_image(la_window_t* jlgr, data_t* zipdata, const char* filename) {
	la_buffer_t img;

	// Load image into "img"
	if(jl_file_pk_load_fdata(jlgr->jl, &img, zipdata, filename))
		la_panic("add-image: pk_load_fdata failed!");

	la_print("Loading Image....");
	uint32_t rtn = jl_sg_add_image__(jlgr, &img);
	la_print("Loaded Image!");
	return rtn;
}

void la_window_icon(la_window_t* window,la_buffer_t* buffer,const char* fname) {
	la_buffer_t img;

	// Load data
	if(jl_file_pk_load_fdata(window->jl, &img, buffer, fname))
		la_panic("add-image: pk_load_fdata failed!");
	// Load image
	SDL_Surface* image = la_window_makesurface(window, &img);
	// Set icon
	SDL_SetWindowIcon(window->wm.window, image);
	// Free image
	SDL_free(image);
}

static void jl_sg_draw_up(jl_t* jl, uint8_t resize, void* data) {
	la_window_t* jlgr = jl->jlgr;

	// Clear the screen.
	jl_gl_clear(jl->jlgr, 0., .5, .66, 1.);
	// Run the screen's redraw function
	jl_thread_mutex_lock(&jlgr->protected.mutex);
	(jlgr->sg.cs == JL_SCR_UP) ? ((jl_fnct)jlgr->protected.functions.redraw.lower)(jl) :
		((jl_fnct)jlgr->protected.functions.redraw.upper)(jl);
	jl_thread_mutex_unlock(&jlgr->protected.mutex);
}

static void jl_sg_draw_dn(jl_t* jl, uint8_t resize, void* data) {
	la_window_t* jlgr = jl->jlgr;

	// Clear the screen.
	jl_gl_clear(jlgr, 1., .5, 0., 1.);
	// Run the screen's redraw function
	jl_thread_mutex_lock(&jlgr->protected.mutex);
	(jlgr->sg.cs == JL_SCR_UP) ? ((jl_fnct)jlgr->protected.functions.redraw.upper)(jl) :
		((jl_fnct)jlgr->protected.functions.redraw.lower)(jl);
	jl_thread_mutex_unlock(&jlgr->protected.mutex);
	// Draw Menu Bar & Mouse
	if(!resize) _jlgr_loopa(jl->jlgr);
}

// Double screen loop
static void _jl_sg_loop_ds(la_window_t* jlgr) {
	// Draw upper screen - alternate screen
	jlgr_sprite_redraw(jlgr, &jlgr->sg.bg.up, NULL);
	jlgr_sprite_draw(jlgr, &jlgr->sg.bg.up);
	// Draw lower screen - default screen
	jlgr_sprite_redraw(jlgr, &jlgr->sg.bg.dn, NULL);
	jlgr_sprite_draw(jlgr, &jlgr->sg.bg.dn);
}

// Single screen loop
static void _jl_sg_loop_ss(la_window_t* jlgr) {
	// Draw lower screen - default screen
	jlgr_sprite_redraw(jlgr, &jlgr->sg.bg.dn, NULL);
	jlgr_sprite_draw(jlgr, &jlgr->sg.bg.dn);
}

// Run the current loop.
void _jl_sg_loop(la_window_t* jlgr) {
	jl_gl_clear(jlgr, 0.f, 0.f, 0.f, 1.);
	((jlgr_fnct)jlgr->sg.loop)(jlgr);
}

static void jl_sg_init_ds_(jl_t* jl) {
	la_window_t* jlgr = jl->jlgr;
	jl_rect_t rcrd = {
		0.f, 0.f,
		1.f, .5f * jlgr->wm.ar
	};

	jlgr_sprite_resize(jlgr, &jlgr->sg.bg.dn, &rcrd);
	rcrd.y = .5f * jlgr->wm.ar;
	jlgr_sprite_resize(jlgr, &jlgr->sg.bg.up, &rcrd);
	// Set double screen loop.
	jlgr->sg.loop = _jl_sg_loop_ds;
	if(jlgr->sg.cs == JL_SCR_SS) jlgr->sg.cs = JL_SCR_DN;
}

static void jl_sg_init_ss_(jl_t* jl) {
	la_window_t* jlgr = jl->jlgr;
	jl_rect_t rcrd = {
		0.f, 0.f,
		1.f, jlgr->wm.ar
	};

	jlgr_sprite_resize(jlgr, &jlgr->sg.bg.dn, &rcrd);
	// Set single screen loop.
	jlgr->sg.loop = _jl_sg_loop_ss;
	jlgr->sg.cs = JL_SCR_SS;
}

void jl_sg_resz__(jl_t* jl) {
	la_window_t* jlgr = jl->jlgr;

	// Check screen count.
	if(jlgr->sg.cs == JL_SCR_SS)
		jl_sg_init_ss_(jl);
	else
		jl_sg_init_ds_(jl);
}

void jl_sg_init__(la_window_t* jlgr) {
	jl_rect_t rc = { 0., 0., 1., jl_gl_ar(jlgr) };
	jl_t* jl = jlgr->jl;

	// Initialize redraw routines to do nothing.
	jl_thread_mutex_lock(&jlgr->protected.mutex);
	jlgr->protected.functions.redraw = (jlgr_redraw_t){jl_dont, jl_dont, jl_dont, jl_dont};
	jl_thread_mutex_unlock(&jlgr->protected.mutex);
	// Create upper and lower screens
	jlgr_sprite_init(jlgr, &jlgr->sg.bg.up, rc,
		jlgr_sprite_dont, jl_sg_draw_up, NULL, 0, NULL, 0);
	jlgr->sg.bg.up.rs = 1;
	jlgr_sprite_init(jlgr, &jlgr->sg.bg.dn, rc,
		jlgr_sprite_dont, jl_sg_draw_dn, NULL, 0, NULL, 0);
	jlgr->sg.bg.dn.rs = 1;
	// Resize.
	jlgr->sg.cs = JL_SCR_SS; // JL_SCR_DN
	jl_sg_resz__(jl);
}
