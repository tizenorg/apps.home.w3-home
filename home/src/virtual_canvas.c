/*
 * Samsung API
 * Copyright (c) 2013 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Flora License, Version 1.1 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <dlog.h>
#include <stdbool.h>
#include <bundle.h>

#include "log.h"
#include "util.h"
#include "virtual_canvas.h"

#define QUALITY_N_COMPRESS "quality=100 compress=1"
#define PRIVATE_DATA_KEY_DATA "dt"



HAPI Evas *virtual_canvas_create(int w, int h)
{
	Ecore_Evas *internal_ee;
	Evas *internal_e;

	// Create virtual canvas
	internal_ee = ecore_evas_buffer_new(w, h);
	if (!internal_ee) {
		_D("Failed to create a new canvas buffer\n");
		return NULL;
	}

	ecore_evas_alpha_set(internal_ee, EINA_TRUE);
	ecore_evas_manual_render_set(internal_ee, EINA_TRUE);

	// Get the "Evas" object from a virtual canvas
	internal_e = ecore_evas_get(internal_ee);
	if (!internal_e) {
		ecore_evas_free(internal_ee);
		_D("Faield to get Evas object\n");
		return NULL;
	}

	return internal_e;
}



static bool _flush_data_to_file(Evas *e, char *data, const char *filename, int w, int h)
{
	Evas_Object *output;

	output = evas_object_image_add(e);
	if (!output) {
		_D("Failed to create an image object\n");
		return false;
	}

	evas_object_image_colorspace_set(output, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_alpha_set(output, EINA_TRUE);
	evas_object_image_size_set(output, w, h);
	evas_object_image_smooth_scale_set(output, EINA_TRUE);
	evas_object_image_data_set(output, data);
	evas_object_image_data_update_add(output, 0, 0, w, h);

	if (evas_object_image_save(output, filename, NULL, QUALITY_N_COMPRESS) == EINA_FALSE) {
		evas_object_del(output);
		_SD("Failed to save a captured image (%s)\n", filename);
		return false;
	}

	evas_object_del(output);

	if (access(filename, F_OK) != 0) {
		_SD("File %s is not found\n", filename);
		return false;
	}

	return true;
}



EAPI bool virtual_canvas_flush_to_file(Evas *e, const char *filename, int w, int h)
{
	void *data;
	Ecore_Evas *internal_ee;

	internal_ee = ecore_evas_ecore_evas_get(e);
	if (!internal_ee) {
		_D("Failed to get ecore evas\n");
		return false;
	}

	ecore_evas_manual_render(internal_ee);

	// Get a pointer of a buffer of the virtual canvas
	data = (void *) ecore_evas_buffer_pixels_get(internal_ee);
	if (!data) {
		_D("Failed to get pixel data\n");
		return false;
	}

	return _flush_data_to_file(e, data, filename, w, h);
}



#define INDICATOR_HEIGHT 30.0
static Evas_Object *_load_file(Evas *e, const char *file, int w, int h)
{
	Evas_Object *output = evas_object_image_filled_add(e);
	retv_if(NULL == output, NULL);

	int file_w, file_h;

	evas_object_image_smooth_scale_set(output, EINA_TRUE);
	evas_object_image_load_size_set(output, w, h);
	evas_object_image_file_set(output, file, NULL);

	int ret = evas_object_image_load_error_get(output);
	if (EVAS_LOAD_ERROR_NONE != ret) _E("Cannot load a file (%d, %s)", ret, file);

	evas_object_image_size_get(output, &file_w, &file_h);
	evas_object_image_fill_set(output, 0, 0, file_w, file_h);

	float file_r = (float) file_w / (float) file_h;
	float canvas_r = (float) w / ((float) h + INDICATOR_HEIGHT);

	int new_w = file_w;
	if (file_r > canvas_r) new_w = (float) file_h * (float) canvas_r;

	float r = (float) w / (float) new_w;
	file_w = (float) file_w * r;
	file_h = (float) file_h * r;

	evas_object_resize(output, file_w, file_h);
	evas_object_move(output, 0, (-INDICATOR_HEIGHT * r));
	evas_object_show(output);

	return output;
}



void _del_ee_cb(Ecore_Evas *ee)
{
	Evas_Object *eoi = ecore_evas_data_get(ee, PRIVATE_DATA_KEY_DATA);
	evas_object_del(eoi);
}



HAPI void *virtual_canvas_load_file_to_data(Evas *e, const char *file, int w, int h)
{
	Evas_Object *eoi = _load_file(e, file, w, h);
	retv_if(NULL == eoi, NULL);

	Ecore_Evas *internal_ee = ecore_evas_ecore_evas_get(e);
	if (NULL == internal_ee) {
		evas_object_del(eoi);
		return NULL;
	}

	ecore_evas_data_set(internal_ee, PRIVATE_DATA_KEY_DATA, eoi);
	ecore_evas_manual_render(internal_ee);
	ecore_evas_callback_pre_free_set(internal_ee, _del_ee_cb);

	// Get a pointer of a buffer of the virtual canvas
	return (void *) ecore_evas_buffer_pixels_get(internal_ee);
}



HAPI bool virtual_canvas_destroy(Evas *e)
{
	Ecore_Evas *ee;

	ee = ecore_evas_ecore_evas_get(e);
	if (!ee) {
		_D("Failed to ecore evas object\n");
		return false;
	}

	ecore_evas_free(ee);
	return true;
}



// End of a file
