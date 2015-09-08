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

/**
 * Smart callback
 * "selected"
 * "dnd"
 */

struct add_viewer_event_info {
	struct {
		Evas_Object *obj;
	} move;

	struct {
		const char *widget_id;
		const char *content;
		int size_type;
		int duplicated;
		Evas_Object *image;
	} pkg_info;
};

enum ADD_VIEWER_CONF_OPTION {
	ADD_VIEWER_CONF_DND = 0x01
};

extern void evas_object_add_viewer_init(void);
extern void evas_object_add_viewer_fini(void);
extern Evas_Object *evas_object_add_viewer_add(Evas_Object *parent);
extern void evas_object_add_viewer_conf_set(int type, int flag);
extern int evas_object_add_viewer_access_action(Evas_Object *obj, int type, void *info);
extern int evas_object_add_viewer_reload(void);

/* End of a file */
