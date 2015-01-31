/*
 * Samsung API
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the License);
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <libintl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include <Elementary.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Edje.h>

#include <ail.h>

#include <dynamicbox.h>
#include <dynamicbox_service.h>
#include <dynamicbox_errno.h>
#include <dynamicbox_buffer.h>

#if defined(LOG_TAG)
#undef LOG_TAG
#endif
#define LOG_TAG "DYNAMICBOX_EVAS"
#include <dlog.h>

#include "dynamicbox_evas.h"

#if !defined(SECURE_LOGD)
#define SECURE_LOGD LOGD
#endif

#if !defined(SECURE_LOGW)
#define SECURE_LOGW LOGW
#endif

#if !defined(SECURE_LOGE)
#define SECURE_LOGE LOGE
#endif

#if !defined(S_)
#define S_(str) dgettext("sys_string", str)
#endif

#if !defined(T_)
#define T_(str) dgettext(PACKAGE, str)
#endif

#if !defined(N_)
#define N_(str) (str)
#endif

#if !defined(_)
#define _(str) gettext(str)
#endif


#if !defined(DbgPrint)
#define DbgPrint(format, arg...)	SECURE_LOGD(format, ##arg)
#endif

#if !defined(ErrPrint)
#define ErrPrint(format, arg...)	SECURE_LOGE(format, ##arg)
#endif

#if !defined(WarnPrint)
#define WarnPrint(format, arg...)	SECURE_LOGW(format, ##arg)
#endif

#if !defined(DYNAMICBOX_EVAS_RESOURCE_EDJ)
#define DYNAMICBOX_EVAS_RESOURCE_EDJ "/usr/share/dynamicbox-evas/res/edje/dynamicbox.edj"
#endif

#if !defined(DYNAMICBOX_EVAS_UNKNOWN)
#define DYNAMICBOX_EVAS_UNKNOWN "/usr/apps/org.tizen.w-home/res/images/unknown.png"
#endif

#if !defined(DYNAMICBOX_EVAS_RESOURCE_GBAR)
#define DYNAMICBOX_EVAS_RESOURCE_GBAR "gbar"
#endif

#if !defined(DYNAMICBOX_EVAS_RESOURCE_LB)
#define DYNAMICBOX_EVAS_RESOURCE_LB "dynamicbox"
#endif

#if !defined(DYNAMICBOX_EVAS_RESOURCE_IMG)
#define DYNAMICBOX_EVAS_RESOURCE_IMG "dynamicbox,image"
#endif

#if !defined(DYNAMICBOX_EVAS_RESOURCE_OVERLAY_LOADING)
#define DYNAMICBOX_EVAS_RESOURCE_OVERLAY_LOADING "overlay"
#endif

#define DEFAULT_OVERLAY_COUNTER 2
#define DEFAULT_OVERLAY_WAIT_TIME 1.0f

#define DBOX_CLASS_NAME "dynamicbox"

#define DBOX_KEEP_BUFFER    -2

#define DEFAULT_CLUSTER "user,created"
#define DEFAULT_CATEGORY "default"

int errno;

/*!
 * \note
 * Detect click event if the pointer does moved in this region (x , y < 5 pixels)
 */
#define CLICK_REGION 22

static struct {
    Evas_Smart_Class sc;
    Evas_Smart *smart;
    Eina_List *list;
    struct {
	Eina_List *delete_list;
	Eina_List *create_list;
    } raw_event;
    int screen_width;
    int screen_height;

    union _conf {
	struct _field {
	    unsigned int user_view_port: 1;
	    unsigned int force_to_buffer: 1;
	    unsigned int support_gbar: 1;
	    unsigned int manual_pause_resume: 1;
	    unsigned int use_fixed_size: 1;
	    unsigned int easy_mode: 1;
	    unsigned int is_scroll_x: 1;
	    unsigned int is_scroll_y: 1;
	    unsigned int auto_feed: 1;
	    unsigned int delayed_pause_resume: 1;
	    unsigned int sensitive_move: 1;
	    unsigned int render_animator: 1;
	    unsigned int auto_render_selector: 1;

	    unsigned int reserved: 19;
	} field;
	unsigned int mask;
    } conf;

    Evas_Object *win;
    Ecore_Animator *renderer;
    Eina_List *dbox_dirty_objects;
    Eina_List *gbar_dirty_objects;
} s_info = {
    .sc = EVAS_SMART_CLASS_INIT_NAME_VERSION(DBOX_CLASS_NAME),
    .smart = NULL,
    .list = NULL,
    .raw_event = {
	.delete_list = NULL,
	.create_list = NULL,
    },
    .conf = {
	.mask = 0,
    },
    .screen_width = 720,
    .screen_height = 1280,
    .win = NULL,
    .renderer = NULL,
    .dbox_dirty_objects = NULL,
    .gbar_dirty_objects = NULL,
};

struct access_ret_cb_data {
    Evas_Object *obj;
    void (*ret_cb)(Evas_Object *, int, void *);
    void *data;
};

struct acquire_data {
    struct widget_data *data;
    Evas_Object *content;
    int w;
    int h;
};

enum CANCEL_CLICK {
    CANCEL_DISABLED = 0x0,
    CANCEL_USER = 0x01,
    CANCEL_PROCESSED = 0x02
};

struct widget_data {
    enum {
	WIDGET_DATA_CREATED = 0x00beef00,
	WIDGET_DATA_DELETED = 0x0d0e0a0d,
    } state;
    struct dynamicbox *handle;
    Evas *e;
    Evas_Object *stage;	/*!< Do not resize thie directly, it should be resized via XX_update_geometry */
    Evas_Object *parent;

    Evas_Object *dbox_layout;	/*!< Layout of Dynamicbox content part */
    Evas_Object *gbar_layout;	/*!< Layout of GBAR content part */

    Evas_Object *dynamicbox;	/*!< Container object */

    struct view_port {
	int x;
	int y;
	int w;
	int h;
    } view_port;

    char *dbox_id;
    char *content;
    char *cluster;
    char *category;
    double period;

    void *dbox_fb;
    void *gbar_fb;

    unsigned int gbar_pixmap;
    unsigned int dbox_pixmap;

    unsigned int *gbar_extra;
    unsigned int *dbox_extra;
    int gbar_extra_cnt;
    int dbox_extra_cnt;

    int dbox_latest_idx; /* -1 = primary buffer, 0 ~ = extra buffer */
    int gbar_latest_idx; /* -1 = primary buffer, 0 ~ = extra buffer */

    struct down {
	int x;
	int y;

	struct {
	    int x;
	    int y;
	    int w;
	    int h;
	} geo;
    } down;

    int x;
    int y;

    int dbox_width;
    int dbox_height;
    int size_type;

    union {
	struct {
	    unsigned int pressed: 1;                     /**< Mouse is pressed */
	    unsigned int touch_effect: 1;                /**< Requires to play touch effect */
	    unsigned int mouse_event: 1;                 /**< Requires to feed mouse event */
	    unsigned int scroll_x: 1;                    /**< */
	    unsigned int scroll_y: 1;
	    unsigned int faulted: 1;
	    unsigned int flick_down: 1;
	    unsigned int gbar_created: 1;

	    unsigned int created: 1;
	    unsigned int deleted: 1;
	    unsigned int dbox_pixmap_acquire_requested: 1;
	    unsigned int gbar_pixmap_acquire_requested: 1;
	    unsigned int cancel_scroll_x: 1;
	    unsigned int cancel_scroll_y: 1;
	    unsigned int cancel_click: 2;
	    unsigned int disable_preview: 1;
	    unsigned int disable_loading: 1;
	    unsigned int disable_text: 1;
	    unsigned int dbox_overlay_loaded: 1;
	    unsigned int gbar_overlay_loaded: 1;

	    unsigned int freeze_visibility: 1;

	    unsigned int dbox_dirty: 1;
	    unsigned int gbar_dirty: 1;

	    unsigned int send_delete: 1;
	    unsigned int permanent_delete: 1;

	    unsigned int reserved: 5;
	} field;	/* Do we really has performance loss? */

	unsigned int flags;
    } is;

    int refcnt;
    int overlay_update_counter;
    Ecore_Timer *overlay_timer;
    int freezed_visibility;
};

enum EFFECT_MASK {
    EFFECT_NONE = 0x0,
    EFFECT_WIDTH = 0x01,
    EFFECT_HEIGHT = 0x02,
    EFFECT_BOTH = 0x03,
    EFFECT_MOVE = 0x10
};

struct animation_data {
    Ecore_Timer *timer;
    Evas_Object *obj;

    unsigned int effect_mask;
    int w;
    int h;
    int x;
    int y;
};

struct raw_event_cbdata {
    void (*cb)(struct dynamicbox_evas_raw_event_info *info, void *data);
    void *data;
};

static int dynamicbox_fault_handler(enum dynamicbox_fault_type fault, const char *pkgname, const char *filename, const char *funcname, void *data);
static int dynamicbox_event_handler(struct dynamicbox *handle, enum dynamicbox_event_type event, void *data);

static void dbox_created_cb(struct dynamicbox *handle, int ret, void *cbdata);
static void dbox_overlay_loading(struct widget_data *data);
static void dbox_overlay_faulted(struct widget_data *data);
static void dbox_overlay_disable(struct widget_data *data, int no_timer);

static void gbar_overlay_loading(struct widget_data *data);
static void gbar_overlay_disable(struct widget_data *data);

static void update_dbox_geometry(struct acquire_data *acquire_data);
static void update_gbar_geometry(struct acquire_data *acquire_data);
static void update_stage_geometry(struct acquire_data *acquire_data);
static void animator_del_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info);

static void remove_dbox_dirty_object_list(struct widget_data *data);
static void remove_gbar_dirty_object_list(struct widget_data *data);
static void append_dbox_dirty_object_list(struct widget_data *data, int idx);
static void append_gbar_dirty_object_list(struct widget_data *data, int idx);
static void dynamicbox_event_dbox_updated(struct widget_data *data);
static void dynamicbox_event_gbar_updated(struct widget_data *data);

static struct widget_data *get_smart_data(Evas_Object *dynamicbox)
{
    if (dynamicbox && evas_object_smart_type_check(dynamicbox, DBOX_CLASS_NAME)) {
	struct widget_data *data;

	data = evas_object_smart_data_get(dynamicbox);
	if (data) {
	    if (data->state == WIDGET_DATA_CREATED) {
		return data;
	    }

	    ErrPrint("smart data is not valid\n");
	} else {
	    ErrPrint("smart data is not exists\n");
	}
    }

    return NULL;
}

static inline void reset_scroller(struct widget_data *data)
{
    Evas_Object *scroller;

    if (!data->dbox_layout) {
	return;
    }

    scroller = elm_object_part_content_get(data->dbox_layout, "scroller");
    if (!scroller) {
	return;
    }

    elm_scroller_region_show(scroller, 0, data->dbox_height >> 1, data->dbox_width, data->dbox_height);
}

static void invoke_raw_event_callback(enum dynamicbox_evas_raw_event_type type, const char *pkgname, Evas_Object *dynamicbox, int error)
{
    struct dynamicbox_evas_raw_event_info info;
    struct raw_event_cbdata *cbdata;
    Eina_List *l;
    Eina_List *n;

    info.pkgname = pkgname;
    info.dynamicbox = dynamicbox;
    info.error = error;
    info.type = type;

    switch (type) {
	case DYNAMICBOX_EVAS_RAW_DELETE:
	    EINA_LIST_FOREACH_SAFE(s_info.raw_event.delete_list, l, n, cbdata) {
		if (cbdata->cb) {
		    cbdata->cb(&info, cbdata->data);
		}
	    }
	    break;
	case DYNAMICBOX_EVAS_RAW_CREATE:
	    EINA_LIST_FOREACH_SAFE(s_info.raw_event.create_list, l, n, cbdata) {
		if (cbdata->cb) {
		    cbdata->cb(&info, cbdata->data);
		}
	    }
	    break;
	default:
	    break;
    }
}

static int find_size_type(struct widget_data *data, int w, int h)
{
    int cnt = DBOX_NR_OF_SIZE_LIST;
    int i;
    int _w[DBOX_NR_OF_SIZE_LIST];
    int _h[DBOX_NR_OF_SIZE_LIST];
    int type = DBOX_SIZE_TYPE_UNKNOWN;
    int find;
    int ret_type = DBOX_SIZE_TYPE_UNKNOWN;
    int delta;

    if (dynamicbox_service_get_supported_sizes(data->dbox_id, &cnt, _w, _h) < 0) {
	ErrPrint("No available sizes: %s\n", data->dbox_id);
	return DBOX_SIZE_TYPE_UNKNOWN;
    }

    find = 0x7FFFFFFF;
    for (i = 0; i < cnt; i++) {
	type = dynamicbox_service_size_type(_w[i], _h[i]);

	if (!s_info.conf.field.easy_mode) {
	    switch (type) {
		case DBOX_SIZE_TYPE_EASY_1x1:
		case DBOX_SIZE_TYPE_EASY_3x1:
		case DBOX_SIZE_TYPE_EASY_3x3:
		    continue;
		default:
		    break;
	    }
	} else {
	    switch (type) {
		case DBOX_SIZE_TYPE_EASY_1x1:
		case DBOX_SIZE_TYPE_EASY_3x1:
		case DBOX_SIZE_TYPE_EASY_3x3:
		    break;
		default:
		    continue;
	    }
	}

	delta = abs(_w[i] - w) + abs(_h[i] - h);
	if (delta < find) {
	    find = delta;
	    ret_type = type;
	}
    }

    return ret_type;
}

static Eina_Bool effect_animator_cb(void *_data)
{
    struct animation_data *data = _data;
    int w;
    int h;
    int x;
    int y;
    int move_x = 0;
    int move_y = 0;

    evas_object_geometry_get(data->obj, &x, &y, &w, &h);
    if (data->w == w && data->h == h) {
	evas_object_event_callback_del(data->obj, EVAS_CALLBACK_DEL, animator_del_cb);
	evas_object_data_del(data->obj, "animation");
	free(data);
	return ECORE_CALLBACK_CANCEL;
    }

    if (data->effect_mask & EFFECT_WIDTH) {
	if (w < data->w) {
	    if (data->w - w > 100) {
		w += 20;
		move_x = 20;
	    } else if (data->w - w > 10) {
		w += 8;
		move_x = 8;
	    } else {
		w++;
		move_x = 1;
	    }
	} else if (w > data->w) {
	    if (w - data->w > 100) {
		w -= 20;
		move_x = -20;
	    } else if (w - data->w > 10) {
		w -= 8;
		move_x = -8;
	    } else {
		w--;
		move_x = -1;
	    }
	}
    } else {
	w = data->w;
    }

    if (data->effect_mask & EFFECT_HEIGHT) {
	if (h < data->h) {
	    if (data->h - h > 100) {
		h += 20;
		move_y = 20;
	    } else if (data->h - h > 10) {
		h += 8;
		move_y = 8;
	    } else {
		h++;
		move_y = 1;
	    }
	} else if (h > data->h) {
	    if (h - data->h > 100) {
		h -= 20;
		move_y = -20;
	    } else if (h - data->h > 10) {
		h -= 8;
		move_y = -8;
	    } else {
		h--;
		move_y = -1;
	    }
	}
    } else {
	h = data->h;
    }

    if (data->effect_mask & EFFECT_MOVE) {
	if (move_x) {
	    x -= move_x;
	}
	if (move_y) {
	    y -= move_y;
	}
	evas_object_move(data->obj, x, y);
    }

    evas_object_resize(data->obj, w, h);
    return ECORE_CALLBACK_RENEW;
}

static void animator_del_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    struct animation_data *data;

    data = evas_object_data_del(obj, "animation");
    if (data) {
	ecore_timer_del(data->timer);
	free(data);
    }
}

static void effect_size_get(Evas_Object *obj, int *w, int *h)
{
    struct animation_data *data;

    data = evas_object_data_get(obj, "animation");
    if (data) {
	*w = data->w;
	*h = data->h;
    } else {
	evas_object_geometry_get(obj, NULL, NULL, w, h);
    }
}

static void effect_resize(Evas_Object *obj, int w, int h, unsigned int effect_mask)
{
    struct animation_data *data;
    int ow;
    int oh;
    int ox;
    int oy;

    evas_object_geometry_get(obj, &ox, &oy, &ow, &oh);

    data = evas_object_data_get(obj, "animation");
    if (data) {
	if (ow == w && oh == h) {
	    evas_object_event_callback_del(obj, EVAS_CALLBACK_DEL, animator_del_cb);
	    evas_object_data_del(obj, "animation");
	    free(data);
	    return;
	}
	/*!
	 * \note
	 * Update to new size
	 */
	data->w = w;
	data->h = h;
	return;
    } else if (ow == w && oh == h) {
	return;
    }

    data = malloc(sizeof(*data));
    if (!data) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return;
    }

    data->obj = obj;
    data->w = w;
    data->h = h;
    data->x = ox;
    data->y = oy;
    data->effect_mask = effect_mask;

    data->timer = ecore_timer_add(1.0f/60.0f, effect_animator_cb, data);
    if (!data->timer) {
	free(data);
	return;
    }

    evas_object_data_set(obj, "animation", data);
    evas_object_event_callback_add(obj, EVAS_CALLBACK_DEL, animator_del_cb, data);
}

struct widget_data *widget_ref(struct widget_data *data)
{
    data->refcnt++;
    return data;
}

static void dump_handle_list(void)
{
    Eina_List *l = NULL;
    Eina_List *n;
    Evas_Object *dynamicbox;
    struct widget_data *data;

    DbgPrint("============== DUMP ===============");
    EINA_LIST_FOREACH_SAFE(s_info.list, l, n, dynamicbox) {
	data = evas_object_smart_data_get(dynamicbox);
	if (!data) {
	    continue;
	}

	DbgPrint("data[%p] %s (%s)\n", data, data->dbox_id, data->is.field.faulted ? "faulted" : "loaded");
    }
    DbgPrint("===================================");
}

struct widget_data *widget_unref(struct widget_data *data)
{
    data->refcnt--;
    DbgPrint("refcnt: %d (%s)\n", data->refcnt, data->dbox_id);
    if (data->refcnt != 0) {
	return data;
    }

    DbgPrint("Destroy widget data %p(%s)\n", data, data->dbox_id);
    free(data->content);
    free(data->dbox_id);
    free(data->cluster);
    free(data->category);

    if (data->overlay_timer) {
	ecore_timer_del(data->overlay_timer);
	data->overlay_timer = NULL;
    }

    if (data->stage) {
	DbgPrint("Remove Stage\n");
	evas_object_del(data->stage);
    }

    if (data->dbox_layout) {
	Evas_Object *content;

	content = elm_object_part_content_unset(data->dbox_layout, "dynamicbox,content");
	if (content) {
	    Evas_Object *front;

	    front = elm_object_part_content_unset(content, "front,content");
	    if (front) {
		DbgPrint("Front image object is deleted\n");
		evas_object_del(front);
	    }

	    DbgPrint("Content object deleted\n");
	    evas_object_del(content);
	}

	content = elm_object_part_content_unset(data->dbox_layout, "overlay,content");
	if (content) {
	    DbgPrint("Overlay is deleted\n");
	    evas_object_del(content);
	}


	DbgPrint("Remove DBOX Layout\n");
	evas_object_del(data->dbox_layout);
    }

    if (data->gbar_layout) {
	Evas_Object *content;
	content = elm_object_part_content_get(data->gbar_layout, "gbar,content");
	if (content) {
	    Evas_Object *overlay;
	    overlay = elm_object_part_content_unset(content, "overlay,content");
	    if (overlay) {
		DbgPrint("Overlay is deleted\n");
		evas_object_del(overlay);
	    }
	}
	DbgPrint("Remove GBAR Layout\n");
	evas_object_del(data->gbar_layout);
    }

    if (data->dbox_fb) {
	dynamicbox_release_buffer(data->dbox_fb);
	data->dbox_fb = NULL;
    }

    if (data->gbar_fb) {
	dynamicbox_release_buffer(data->gbar_fb);
	data->gbar_fb = NULL;
    }

    data->state = WIDGET_DATA_DELETED;
    free(data);
    dump_handle_list();
    return NULL;
}

static void gbar_down_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *down = event_info;
    struct widget_data *data = cbdata;
    struct dynamicbox_mouse_event_info minfo;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_geometry_get(obj, &data->down.geo.x, &data->down.geo.y, &data->down.geo.w, &data->down.geo.h);

    data->x = data->down.x = down->canvas.x;
    data->y = data->down.y = down->canvas.y;
    data->is.field.pressed = 1;
    if (s_info.conf.field.auto_render_selector) {
	DbgPrint("Change to direct render\n");
	s_info.conf.field.render_animator = 0;
    }

    if (s_info.conf.field.auto_feed) {
	minfo.x = (double)data->down.geo.x;
	minfo.y = (double)data->down.geo.y;
	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_SET, &minfo);
    } else {
	minfo.x = (double)(down->canvas.x - data->down.geo.x) / (double)data->down.geo.w;
	minfo.y = (double)(down->canvas.y - data->down.geo.y) / (double)data->down.geo.h;
	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_ENTER, &minfo);
	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_DOWN, &minfo);
    }
}

static void gbar_move_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Move *move = event_info;
    struct widget_data *data = cbdata;
    Evas_Coord x, y, w, h;
    struct dynamicbox_mouse_event_info minfo;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (!data->is.field.pressed) {
	return;
    }

    evas_object_geometry_get(obj, &x, &y, &w, &h);

    data->x = move->cur.canvas.x;
    data->y = move->cur.canvas.y;

    if (data->is.field.cancel_click != CANCEL_DISABLED || !s_info.conf.field.auto_feed) {
	minfo.x = (double)(move->cur.canvas.x - x) / (double)w;
	minfo.y = (double)(move->cur.canvas.y - y) / (double)h;

	if (data->is.field.cancel_click == CANCEL_USER) {
	    dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_ON_HOLD, &minfo);
	    data->is.field.cancel_click = CANCEL_PROCESSED;
	}

	if (!s_info.conf.field.auto_feed) {
	    dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_MOVE, &minfo);
	}

	if (s_info.conf.field.auto_render_selector) {
	    DbgPrint("Change to direct render\n");
	    s_info.conf.field.render_animator = 0;
	}
    }

}

static void dbox_pixmap_del_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    struct widget_data *data = cbdata;

    if (data->dbox_pixmap) {
	dynamicbox_release_resource_id(data->handle, 0, data->dbox_pixmap);
	data->dbox_pixmap = 0;
    }

    if (data->dbox_extra) {
	int idx;

	for (idx = 0; idx < dynamicbox_option(DBOX_OPTION_EXTRA_BUFFER_CNT); idx++) {
	    if (data->dbox_extra[idx] != 0u) {
		if (dynamicbox_release_resource_id(data->handle, 0, data->dbox_extra[idx]) < 0) {
		    ErrPrint("Failed to release %u\n", data->dbox_extra[idx]);
		}
		data->dbox_extra[idx] = 0u;
	    }
	}

	free(data->dbox_extra);
	data->dbox_extra = NULL;
    }
}

static void gbar_pixmap_del_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    struct widget_data *data = cbdata;

    if (data->gbar_pixmap) {
	dynamicbox_release_resource_id(data->handle, 1, data->gbar_pixmap);
	data->gbar_pixmap = 0;
    }

    if (data->gbar_extra) {
	int idx;

	for (idx = 0; idx < dynamicbox_option(DBOX_OPTION_EXTRA_BUFFER_CNT); idx++) {
	    if (data->gbar_extra[idx] != 0u) {
		if (dynamicbox_release_resource_id(data->handle, 0, data->gbar_extra[idx]) < 0) {
		    ErrPrint("Failed to release %u\n", data->gbar_extra[idx]);
		}
		data->gbar_extra[idx] = 0u;
	    }
	}

        free(data->gbar_extra);
	data->gbar_extra = NULL;
    }
}

static void gbar_up_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Up *up = event_info;
    struct widget_data *data = cbdata;
    Evas_Coord x, y, w, h;
    struct dynamicbox_mouse_event_info minfo;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (!data->is.field.pressed) {
	return;
    }

    evas_object_geometry_get(obj, &x, &y, &w, &h);

    if (s_info.conf.field.auto_feed) {
	minfo.x = (double)x / (double)w;
	minfo.y = (double)y / (double)h;
	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_UNSET, &minfo);
    } else {
	minfo.x = (double)(up->canvas.x - x) / (double)w;
	minfo.y = (double)(up->canvas.y - y) / (double)h;

	if (data->down.geo.x != x || data->down.geo.y != y || data->is.field.cancel_click == CANCEL_USER) {
	    dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_ON_HOLD, &minfo);
	    data->is.field.cancel_click = CANCEL_PROCESSED;
	}

	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_UP, &minfo);
	dynamicbox_feed_mouse_event(data->handle, DBOX_GBAR_MOUSE_LEAVE, &minfo);
    }

    data->is.field.cancel_click = CANCEL_DISABLED;
}

static void dbox_down_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Down *down = event_info;
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (s_info.conf.field.support_gbar && !data->is.field.gbar_created) {
	data->is.field.flick_down = 1;
    }

    data->down.x = data->x = down->canvas.x;
    data->down.y = data->y = down->canvas.y;
    data->is.field.pressed = 1;
    data->is.field.scroll_x = 0;
    data->is.field.scroll_y = 0;
    data->is.field.cancel_scroll_x = 0;
    data->is.field.cancel_scroll_y = 0;

    if (s_info.conf.field.auto_render_selector) {
	DbgPrint("Change to direct render\n");
	s_info.conf.field.render_animator = 0;
    }

    evas_object_geometry_get(obj, &data->down.geo.x, &data->down.geo.y, &data->down.geo.w, &data->down.geo.h);

    if (s_info.conf.field.sensitive_move && (data->down.geo.x != data->view_port.x || data->down.geo.y != data->view_port.y)) {
	data->is.field.pressed = 0;
	if (s_info.conf.field.auto_render_selector) {
	    DbgPrint("Change to render animator\n");
	    s_info.conf.field.render_animator = 1;
	}
	return;
    }

    if (data->handle && !data->is.field.faulted) {
	struct dynamicbox_mouse_event_info minfo;

	if (s_info.conf.field.auto_feed && data->is.field.mouse_event) {
	    minfo.x = (double)data->down.geo.x;
	    minfo.y = (double)data->down.geo.y;
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_SET, &minfo);
	} else {
	    minfo.x = (double)(data->x - data->down.geo.x) / (double)data->down.geo.w;
	    minfo.y = (double)(data->y - data->down.geo.y) / (double)data->down.geo.h;

	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ENTER, &minfo);
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_DOWN, &minfo);
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_MOVE, &minfo);
	}
    }
}

static void smart_callback_call(struct widget_data *data, const char *signal, void *cbdata)
{
    if (data->is.field.deleted || !data->dynamicbox) {
	DbgPrint("Dynamicbox is deleted, ignore smart callback call\n");
	return;
    }

    evas_object_smart_callback_call(data->dynamicbox, signal, cbdata);
}

static void dbox_destroy_gbar_cb(struct dynamicbox *handle, int ret, void *cbdata)
{
    struct widget_data *data = cbdata;
    Evas_Object *gbar_content;
    struct dynamicbox_evas_event_info info;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    data->is.field.gbar_created = 0;

    info.error = ret;
    info.event = DBOX_EVENT_GBAR_DESTROYED;
    info.pkgname = data->dbox_id;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_DESTROYED, &info);

    DbgPrint("ret: %d\n", ret);
    gbar_content = elm_object_part_content_unset(data->gbar_layout, "gbar,content");
    if (gbar_content) {
	Evas_Native_Surface *surface;
	unsigned int pixmap;

	switch (dynamicbox_type(data->handle, 1)) {
	    case DBOX_CONTENT_TYPE_RESOURCE_ID:
		if (!s_info.conf.field.force_to_buffer) {
		    surface = evas_object_image_native_surface_get(gbar_content);
		    if (!surface) {
			ErrPrint("surface is NULL\n");
			evas_object_del(gbar_content);
			break;
		    }

		    pixmap = surface->data.x11.pixmap;
		    evas_object_del(gbar_content);

		    dynamicbox_release_resource_id(data->handle, 1, (int)pixmap);
		    if (pixmap == data->gbar_pixmap) {
			data->gbar_pixmap = 0;
		    }
		    break;
		}
	    case DBOX_CONTENT_TYPE_BUFFER:
		if (data->gbar_fb) {
		    dynamicbox_release_buffer(data->gbar_fb);
		    data->gbar_fb = NULL;
		}
		evas_object_del(gbar_content);
		break;
	    case DBOX_CONTENT_TYPE_TEXT:
		break;
	    case DBOX_CONTENT_TYPE_UIFW:
		break;
	    case DBOX_CONTENT_TYPE_INVALID:
	    default:
		break;
	}
    }

    if (data->gbar_layout) {
	evas_object_del(data->gbar_layout);
	data->gbar_layout = NULL;
    }

    remove_gbar_dirty_object_list(data);
    widget_unref(data);
}

static void gbar_animation_done_cb(void *cbdata, Evas_Object *obj, const char *emission, const char *source)
{
    Evas_Object *rect;
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    rect = elm_object_part_content_unset(obj, "overlay,content");
    if (rect) {
	evas_object_del(rect);
    }
}

static void gbar_create_buffer_object(struct widget_data *data)
{
    Evas_Object *gbar_content;

    gbar_content = evas_object_image_filled_add(data->e);
    if (!gbar_content) {
	ErrPrint("Failed to create an image object\n");
    } else {
	evas_object_image_colorspace_set(gbar_content, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_alpha_set(gbar_content, EINA_TRUE);

	elm_object_part_content_set(data->gbar_layout, "gbar,content", gbar_content);
    }
}

static void gbar_create_text_object(struct widget_data *data)
{
    ErrPrint("Unsupported\n");
    /*!
     * \todo
     */
}

static void gbar_create_pixmap_object(struct widget_data *data)
{
    Evas_Object *gbar_content;

    gbar_content = evas_object_image_filled_add(data->e);
    if (!gbar_content) {
	ErrPrint("Failed to create an image object\n");
	return;
    }

    evas_object_image_colorspace_set(gbar_content, EVAS_COLORSPACE_ARGB8888);
    evas_object_image_alpha_set(gbar_content, EINA_TRUE);
    elm_object_part_content_set(data->gbar_layout, "gbar,content", gbar_content);

    evas_object_event_callback_add(gbar_content, EVAS_CALLBACK_MOUSE_DOWN, gbar_down_cb, data);
    evas_object_event_callback_add(gbar_content, EVAS_CALLBACK_MOUSE_MOVE, gbar_move_cb, data);
    evas_object_event_callback_add(gbar_content, EVAS_CALLBACK_MOUSE_UP, gbar_up_cb, data);
    evas_object_event_callback_add(gbar_content, EVAS_CALLBACK_DEL, gbar_pixmap_del_cb, data);
}

static void dbox_create_gbar_cb(struct dynamicbox *handle, int ret, void *cbdata)
{
    struct widget_data *data = cbdata;
    struct dynamicbox_evas_event_info info;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (ret != DBOX_STATUS_ERROR_NONE) {
	DbgPrint("Create GBAR: 0x%X\n", ret);
	info.error = ret;
	info.event = DBOX_EVENT_GBAR_CREATED;
	info.pkgname = data->dbox_id;
	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_ABORTED, &info);
	widget_unref(data);
	return;
    }

    if (data->is.field.deleted) {
	/**
	 * Evas Object is deleted.
	 * Do not proceed this process anymore and destroy GBAR too
	 */
	dynamicbox_destroy_glance_bar(data->handle, NULL, NULL);
	return;
    }

    DbgPrint("GBAR is created\n");

    if (!data->gbar_layout) {
	data->gbar_layout = elm_layout_add(data->parent);
	if (!data->gbar_layout) {
	    ErrPrint("Failed to add an edje\n");
	    info.error = DBOX_STATUS_ERROR_FAULT;
	    info.event = DBOX_EVENT_GBAR_CREATED;
	    info.pkgname = data->dbox_id;
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_ABORTED, &info);

	    ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
	    if (ret < 0) {
		/*!
		 * \note
		 *       PREVENT detect this. but it is FALSE POSITIVE
		 *
		 * widget_ref will increase the refcnt of data.
		 * and it is called when calling the dynamicbox_destroy_glance_bar function (via the last param)
		 * So this function call will not release the data.
		 */
		widget_unref(data);
	    }

	    widget_unref(data);
	    return;
	}

	if (elm_layout_file_set(data->gbar_layout, DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_GBAR) == EINA_FALSE) {
	    ErrPrint("Failed to load edje object: %s(%s)\n", DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_GBAR);
	    evas_object_del(data->gbar_layout);
	    data->gbar_layout = NULL;

	    info.error = DBOX_STATUS_ERROR_IO_ERROR;
	    info.event = DBOX_EVENT_GBAR_CREATED;
	    info.pkgname = data->dbox_id;
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_ABORTED, &info);

	    ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
	    if (ret < 0) {
		/*!
		 * \note
		 *       PREVENT detect this. but it is FALSE POSITIVE
		 *
		 * widget_ref will increase the refcnt of data.
		 * and it is called when calling the dynamicbox_destroy_glance_bar function (via the last param)
		 * So this function call will not release the data.
		 */
		widget_unref(data);
	    }

	    widget_unref(data);
	    return;
	}

	evas_object_smart_member_add(data->gbar_layout, data->dynamicbox);
	evas_object_clip_set(data->gbar_layout, data->stage);
	elm_object_signal_callback_add(data->gbar_layout, "finished", "animation", gbar_animation_done_cb, data);
	evas_object_show(data->gbar_layout);
    }
    gbar_overlay_loading(data);

    switch (dynamicbox_type(data->handle, 1)) {
	case DBOX_CONTENT_TYPE_RESOURCE_ID:
	    if (!s_info.conf.field.force_to_buffer) {
		gbar_create_pixmap_object(data);
		break;
	    }
	case DBOX_CONTENT_TYPE_BUFFER:
	    gbar_create_buffer_object(data);
	    break;
	case DBOX_CONTENT_TYPE_TEXT:
	    gbar_create_text_object(data);
	    break;
	case DBOX_CONTENT_TYPE_UIFW:
	    break;
	default:
	    info.error = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	    info.event = DBOX_EVENT_GBAR_CREATED;
	    info.pkgname = data->dbox_id;
	    ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
	    if (ret < 0) {
		/*!
		 * \note
		 *       PREVENT detect this. but it is FALSE POSITIVE
		 *
		 * widget_ref will increase the refcnt of data.
		 * and it is called when calling the dynamicbox_destroy_glance_bar function (via the last param)
		 * So this function call will not release the data.
		 */
		widget_unref(data);
	    }
	    ErrPrint("Failed to create a GBAR, unknown type\n");
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_ABORTED, &info);
	    widget_unref(data);
	    return;
    }

    data->is.field.gbar_created = 1;
    info.error = DBOX_STATUS_ERROR_NONE;
    info.event = DBOX_EVENT_GBAR_CREATED;
    info.pkgname = data->dbox_id;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_GBAR_CREATED, &info);
    widget_unref(data);
}

static void update_scroll_flag(struct widget_data *data, int x, int y)
{
    if (s_info.conf.field.is_scroll_x && !s_info.conf.field.is_scroll_y) {
	if (!data->is.field.scroll_x && abs(y - data->down.y) > CLICK_REGION && (abs(x - data->down.x) <= CLICK_REGION)) {
	    data->is.field.cancel_scroll_x = 1;
	}
    }

    if (s_info.conf.field.is_scroll_y && !s_info.conf.field.is_scroll_x) {
	if (!data->is.field.scroll_y && abs(x - data->down.x) > CLICK_REGION && (abs(y - data->down.y) <= CLICK_REGION)) {
	    data->is.field.cancel_scroll_y = 1;
	}
    }

    data->is.field.scroll_x = s_info.conf.field.is_scroll_x && (!!(data->is.field.scroll_x || (abs(x - data->down.x) > CLICK_REGION)));
    data->is.field.scroll_y = s_info.conf.field.is_scroll_y && (!!(data->is.field.scroll_y || (abs(y - data->down.y) > CLICK_REGION)));
    data->is.field.scroll_x = !data->is.field.cancel_scroll_x && data->is.field.scroll_x;
    data->is.field.scroll_y = !data->is.field.cancel_scroll_y && data->is.field.scroll_y;
}

static void dbox_up_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Up *up = event_info;
    struct dynamicbox_evas_event_info info;
    struct widget_data *data = cbdata;
    Evas_Coord x, y, w, h;
    int ret = 0;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (!data->is.field.pressed) {
	return;
    }

    update_scroll_flag(data, up->canvas.x, up->canvas.y);

    data->x = up->canvas.x;
    data->y = up->canvas.y;
    data->is.field.pressed = 0;

    if (s_info.conf.field.auto_render_selector) {
	DbgPrint("Change to render animator\n");
	s_info.conf.field.render_animator = 1;
    }

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_GBAR_CREATED;

    if (s_info.conf.field.support_gbar && data->is.field.flick_down && data->y - data->down.y < CLICK_REGION) {
	DbgPrint("Flick down is canceled\n");
	data->is.field.flick_down = 0;
	info.error = DBOX_STATUS_ERROR_CANCEL;
	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
    }

    evas_object_geometry_get(data->dynamicbox, &x, &y, &w, &h);

    if (data->is.field.flick_down) {
	data->is.field.flick_down = 0;
	if (!data->handle || data->is.field.faulted || !dynamicbox_has_glance_bar(data->handle)) {
	    if (!data->is.field.gbar_created && s_info.conf.field.support_gbar) {
		elm_object_signal_emit(data->dbox_layout, "tilt", "content");
	    }
	    DbgPrint("Flick down is canceled\n");
	    info.error = DBOX_STATUS_ERROR_CANCEL;
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
	} else if (s_info.conf.field.support_gbar && !data->is.field.gbar_created) {
	    double rx;
	    double ry;
	    int gbar_w;
	    int gbar_h;

	    if (dynamicbox_get_glance_bar_size(data->handle, &gbar_w, &gbar_h) != DBOX_STATUS_ERROR_NONE) {
		gbar_w = 0;
		gbar_h = 0;
	    }

	    elm_object_signal_emit(data->dbox_layout, "move,down", "content");

	    rx = ((double)x + (double)w / 2.0f) / (double)s_info.screen_width;
	    DbgPrint("x[%d], w[%d], rx[%lf]\n", x, w, rx);
	    // 0.0    0.125    0.25    0.375   0.5   0.625   0.75    0.875   1.0
	    switch (dynamicbox_size(data->handle)) {
		case DBOX_SIZE_TYPE_1x1:
		    if (rx < 0.25f) {
			rx = 0.125f;
		    } else if (rx < 0.5f) {
			rx = 0.375f;
		    } else if (rx < 0.75f) {
			rx = 0.625f;
		    } else if (rx < 1.0f) {
			rx = 0.875f;
		    }
		    break;
		case DBOX_SIZE_TYPE_2x1:
		case DBOX_SIZE_TYPE_2x2:
		    if (rx < 0.5f) {
			rx = 0.25f;
		    } else if (rx < 0.625f) {
			rx = 0.5f;
		    } else {
			rx = 0.75f;
		    }
		    break;
		default:
		    rx = 0.5f;
		    break;
	    }

	    if (y + h + gbar_h <= s_info.screen_height) {
		ry = 0.0f;
	    } else {
		ry = 1.0f;
	    }

	    DbgPrint("converted rx[%lf], ry[%lf]\n", rx, ry);

	    ret = dynamicbox_create_glance_bar(data->handle, rx, ry, dbox_create_gbar_cb, widget_ref(data));
	    if (ret < 0) {
		widget_unref(data);
		DbgPrint("Flick down is canceled\n");
		info.error = DBOX_STATUS_ERROR_CANCEL;
		smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
	    }
	    DbgPrint("Create GBAR: %d (%lfx%lf)\n", ret, rx, ry);
	}
    }

    if (data->handle && !data->is.field.faulted) {
	struct dynamicbox_mouse_event_info minfo;

	minfo.x = (double)(data->x - x) / (double)w;
	minfo.y = (double)(data->y - y) / (double)h;

	evas_object_geometry_get(obj, &x, &y, NULL, NULL);

	reset_scroller(data);

	if (s_info.conf.field.auto_feed && data->is.field.mouse_event) {
	    struct dynamicbox_mouse_event_info _minfo;

	    if (data->down.geo.x != x || data->down.geo.y != y || data->is.field.scroll_x || data->is.field.scroll_y || data->is.field.cancel_click == CANCEL_USER) {
		dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);
		data->is.field.cancel_click = CANCEL_PROCESSED;
	    }

	    _minfo.x = (double)data->down.geo.x / (double)data->down.geo.w;
	    _minfo.y = (double)data->down.geo.y / (double)data->down.geo.h;
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_UNSET, &_minfo);
	} else {
	    if (!data->is.field.mouse_event) {
		/* We have to keep the first position of touch down */
		minfo.x = (double)(data->down.x - x) / (double)w;
		minfo.y = (double)(data->down.y - y) / (double)h;
		if (data->down.geo.x != x || data->down.geo.y != y || data->is.field.scroll_x || data->is.field.scroll_y || data->is.field.cancel_click == CANCEL_USER || abs(data->x - data->down.x) > CLICK_REGION || abs(data->y - data->down.y) > CLICK_REGION) {
		    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);
		    data->is.field.cancel_click = CANCEL_PROCESSED;
		}
	    } else {
		if (data->down.geo.x != x || data->down.geo.y != y || data->is.field.scroll_x || data->is.field.scroll_y || data->is.field.cancel_click == CANCEL_USER) {
		    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);
		    data->is.field.cancel_click = CANCEL_PROCESSED;
		}
	    }

	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_UP, &minfo);
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_LEAVE, &minfo);
	}

	if (!data->is.field.flick_down) {
	    ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	    if (data->is.field.gbar_created) {
		ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
		if (ret < 0) {
		    widget_unref(data);
		}
	    } else if (data->is.field.cancel_click == CANCEL_DISABLED) {
		ret = dynamicbox_click(data->handle, minfo.x, minfo.y);
	    }
	}

	DbgPrint("Up: %lfx%lf [x:%d/%d/%d] [y:%d/%d/%d], ret: 0x%X, cancel: 0x%x\n",
		minfo.x, minfo.y,
		data->is.field.scroll_x, s_info.conf.field.is_scroll_x, data->is.field.cancel_scroll_x,
		data->is.field.scroll_y, s_info.conf.field.is_scroll_y, data->is.field.cancel_scroll_y,
		ret, data->is.field.cancel_click);
	data->is.field.cancel_click = CANCEL_DISABLED;
    }
}

static void dbox_move_cb(void *cbdata, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Mouse_Move *move = event_info;
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (!data->is.field.pressed) {
	return;
    }

    if (s_info.conf.field.support_gbar && data->is.field.flick_down && data->y - move->cur.canvas.y > 0) {
	struct dynamicbox_evas_event_info info;

	DbgPrint("Flick down is canceled\n");
	data->is.field.flick_down = 0;
	info.pkgname = data->dbox_id;
	info.event = DBOX_EVENT_GBAR_CREATED;
	info.error = DBOX_STATUS_ERROR_CANCEL;

	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
    }

    update_scroll_flag(data, move->cur.canvas.x, move->cur.canvas.y);

    data->x = move->cur.canvas.x;
    data->y = move->cur.canvas.y;

    if (data->handle && !data->is.field.faulted) {
	if (data->is.field.cancel_click == CANCEL_USER) {
	    struct dynamicbox_mouse_event_info minfo;
	    Evas_Coord x, y, w, h;

	    evas_object_geometry_get(obj, &x, &y, &w, &h);

	    minfo.x = (double)(data->x - x) / (double)w;
	    minfo.y = (double)(data->y - y) / (double)h;
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);

	    /*
	     * Reset the clicked event
	     */
	    data->is.field.cancel_click = CANCEL_PROCESSED;

	    if (s_info.conf.field.auto_render_selector) {
		DbgPrint("Change to render animator\n");
		s_info.conf.field.render_animator = 1;
	    }

	}

	if (!s_info.conf.field.auto_feed) {
	    struct dynamicbox_mouse_event_info minfo;
	    Evas_Coord x, y, w, h;

	    evas_object_geometry_get(obj, &x, &y, &w, &h);

	    minfo.x = (double)(data->x - x) / (double)w;
	    minfo.y = (double)(data->y - y) / (double)h;
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_MOVE, &minfo);
	}

	if (s_info.conf.field.support_gbar && data->is.field.flick_down && abs(data->y - data->down.y) > CLICK_REGION) {
	    struct dynamicbox_evas_event_info info;
	    data->is.field.flick_down = 0;
	    info.pkgname = data->dbox_id;
	    info.event = DBOX_EVENT_GBAR_CREATED;
	    info.error = DBOX_STATUS_ERROR_CANCEL;
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
	    DbgPrint("Flick down is canceled\n");
	}

    } else {
	if (s_info.conf.field.auto_render_selector) {
	    s_info.conf.field.render_animator = 1;
	}
    }
}

static char *get_package_icon(struct widget_data *data)
{
    char *icon;
    char *uiapp;

    if (data->size_type == DBOX_SIZE_TYPE_UNKNOWN) {
	icon = dynamicbox_service_i18n_icon(data->dbox_id, NULL);
    } else {
	icon = dynamicbox_service_preview(data->dbox_id, data->size_type);
    }

    if (icon && access(icon, R_OK) == 0) {
	return icon;
    }

    if (icon) {
	ErrPrint("Failed to access an icon file: [%s]\n", icon);
	free(icon);
	icon = NULL;
    }

    uiapp = dynamicbox_service_mainappid(data->dbox_id);
    if (uiapp) {
	int ret;
	ail_appinfo_h ai;

	ret = ail_get_appinfo(uiapp, &ai);
	free(uiapp);
	if (ret == AIL_ERROR_OK) {
	    char *uiapp_icon = NULL;

	    ret = ail_appinfo_get_str(ai, AIL_PROP_ICON_STR, &uiapp_icon);
	    if (ret != AIL_ERROR_OK || !uiapp_icon || access(uiapp_icon, R_OK) != 0) {
		ErrPrint("[%s] - %s\n", uiapp_icon, strerror(errno));
	    } else {
		DbgPrint("UI-App icon: [%s]\n", uiapp_icon);
		icon = strdup(uiapp_icon);
		if (!icon) {
		    ErrPrint("Heap: %s\n", strerror(errno));
		    /*!
		     * \note
		     * icon will be specified to "unknown" icon file. (default)
		     */
		}
	    }

	    ail_destroy_appinfo(ai);
	}
    }

    if (!icon) {
	icon = strdup(DYNAMICBOX_EVAS_UNKNOWN);
	if (!icon) {
	    ErrPrint("Heap: %s\n", strerror(errno));
	}
    }

    return icon;
}

static void activate_ret_cb(struct dynamicbox *handle, int ret, void *cbdata)
{
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    data->overlay_update_counter = 0;
    dbox_overlay_disable(data, 1);

    DbgPrint("Activated (%s): %d\n", data->dbox_id, ret);
    if (!data->is.field.deleted && (ret == DBOX_STATUS_ERROR_NONE || ret == DBOX_STATUS_ERROR_INVALID_PARAMETER)) {
	int type;
	Evas_Coord w, h;
	struct acquire_data acquire_data = {
	    .data = data,
	};

	evas_object_geometry_get(data->dbox_layout, NULL, NULL, &w, &h);

	type = find_size_type(data, w, h);
	if (type == DBOX_SIZE_TYPE_UNKNOWN) {
	    ErrPrint("Failed to find a proper size type: %dx%d\n", w, h);
	    type = DBOX_SIZE_TYPE_1x1;
	}

	data->is.field.faulted = 0;
	data->is.field.created = 0;
	data->is.field.send_delete = 1;
	update_dbox_geometry(&acquire_data);
	data->handle = dynamicbox_add(data->dbox_id, data->content,
		data->cluster, data->category,
		data->period, type,
		dbox_created_cb, widget_ref(data));
	if (!data->handle) {
	    ErrPrint("Failed to send add request\n");
	    widget_unref(data);
	    return;
	}

	DbgPrint("Added Handle: (%p) %p\n", data->handle, data);
	dynamicbox_set_data(data->handle, data->dynamicbox);
	dbox_overlay_loading(data);
    }

    data->is.field.deleted = 0;
    widget_unref(data);
}

static void dbox_animation_done_cb(void *cbdata, Evas_Object *obj, const char *emission, const char *source)
{
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (dynamicbox_has_glance_bar(data->handle)) {
    } else {
	DbgPrint("Animation finished\n");
    }
}

static void dbox_turn_done_cb(void *cbdata, Evas_Object *obj, const char *emission, const char *source)
{
    struct widget_data *data = cbdata;
    Evas_Object *overlay;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    overlay = elm_object_part_content_unset(data->dbox_layout, "overlay,content");
    if (overlay) {
	evas_object_del(overlay);
	data->is.field.dbox_overlay_loaded = 0;
    }
}

static void dbox_overlay_clicked_cb(void *cbdata, Evas_Object *obj, const char *emission, const char *source)
{
    struct widget_data *data = cbdata;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    DbgPrint("Overlay is clicked: (%s) (%s)\n", emission, source);
    if (!data->is.field.faulted) {
	/*!
	 * \todo
	 * Reload
	 */
	DbgPrint("Package [%s] is not faulted one\n", data->dbox_id);
    } else {
	DbgPrint("Activate: [%s]\n", data->dbox_id);
	if (dynamicbox_activate(data->dbox_id, activate_ret_cb, widget_ref(data)) < 0) {
	    widget_unref(data);
	    ErrPrint("Failed to activate %s\n", data->dbox_id);
	}
    }
}

static void widget_data_setup(struct widget_data *data)
{
    data->e = evas_object_evas_get(data->dynamicbox);
    if (!data->e) {
	ErrPrint("Failed to get Evas object\n");
	data->state = WIDGET_DATA_DELETED;
	free(data);
	return;
    }

    data->stage = evas_object_rectangle_add(data->e);
    if (!data->stage) {
	ErrPrint("Failed to add stage object\n");
	data->state = WIDGET_DATA_DELETED;
	free(data);
	return;
    }

    evas_object_color_set(data->stage, 255, 255, 255, 255);

    data->dbox_layout = elm_layout_add(data->parent);
    if (!data->dbox_layout) {
	ErrPrint("Failed to add edje object\n");
	evas_object_del(data->stage);
	data->state = WIDGET_DATA_DELETED;
	free(data);
	return;
    }

    if (elm_layout_file_set(data->dbox_layout, DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_LB) == EINA_FALSE) {
	ErrPrint("Failed to load edje object: %s(%s)\n", DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_LB);
	evas_object_del(data->dbox_layout);
	evas_object_del(data->stage);
	data->state = WIDGET_DATA_DELETED;
	free(data);
	return;
    }

    Evas_Object *scroller;
    scroller = elm_scroller_add(data->parent);
    if (scroller) {
	Evas_Object *box;

	elm_scroller_bounce_set(scroller, EINA_FALSE, EINA_FALSE);
	elm_scroller_policy_set(scroller, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_OFF);
	elm_scroller_single_direction_set(scroller, ELM_SCROLLER_SINGLE_DIRECTION_HARD);
	//elm_object_scroll_lock_x_set(scroller, EINA_TRUE);

	box = evas_object_rectangle_add(data->e);
	if (box) {
	    int height;

	    height = s_info.screen_height << 1;

	    evas_object_color_set(box, 0, 0, 0, 0);
	    evas_object_resize(box, s_info.screen_width, height);
	    evas_object_size_hint_min_set(box, s_info.screen_width, height);
	    evas_object_show(box);
	}

	elm_object_content_set(scroller, box);
	elm_object_part_content_set(data->dbox_layout, "scroller", scroller);
    } else {
	ErrPrint("Failed to create a scroller\n");
    }

    evas_object_show(data->dbox_layout);

    elm_object_signal_callback_add(data->dbox_layout, "mouse,clicked,1", "overlay,content", dbox_overlay_clicked_cb, data);
    elm_object_signal_callback_add(data->dbox_layout, "done", "turn", dbox_turn_done_cb, data);
    elm_object_signal_callback_add(data->dbox_layout, "finished", "animation", dbox_animation_done_cb, data);

    evas_object_event_callback_add(data->dbox_layout, EVAS_CALLBACK_MOUSE_DOWN, dbox_down_cb, data);
    evas_object_event_callback_add(data->dbox_layout, EVAS_CALLBACK_MOUSE_MOVE, dbox_move_cb, data);
    evas_object_event_callback_add(data->dbox_layout, EVAS_CALLBACK_MOUSE_UP, dbox_up_cb, data);

    evas_object_smart_member_add(data->stage, data->dynamicbox);
    evas_object_smart_member_add(data->dbox_layout, data->dynamicbox);
    evas_object_clip_set(data->dbox_layout, data->stage);

}

static Eina_Bool renderer_cb(void *_data)
{
    struct widget_data *data;

    EINA_LIST_FREE(s_info.dbox_dirty_objects, data) {
	dynamicbox_event_dbox_updated(data);
    }

    EINA_LIST_FREE(s_info.gbar_dirty_objects, data) {
	dynamicbox_event_gbar_updated(data);
    }

    s_info.renderer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void remove_dbox_dirty_object_list(struct widget_data *data)
{
    s_info.dbox_dirty_objects = eina_list_remove(s_info.dbox_dirty_objects, data);
}

static void remove_gbar_dirty_object_list(struct widget_data *data)
{
    s_info.gbar_dirty_objects = eina_list_remove(s_info.gbar_dirty_objects, data);
}

static void append_dbox_dirty_object_list(struct widget_data *data, int idx)
{
    data->is.field.dbox_dirty = 1;

    if (idx != DBOX_KEEP_BUFFER) {
	data->dbox_latest_idx = idx;
    }

    if (dynamicbox_visibility(data->handle) != DBOX_SHOW) {
	return;
    }

    if (s_info.conf.field.render_animator) {
	if (eina_list_data_find(s_info.dbox_dirty_objects, data)) {
	    return;
	}

	if (!s_info.renderer) {
	    s_info.renderer = ecore_animator_add(renderer_cb, NULL);
	    if (!s_info.renderer) {
		ErrPrint("Failed to create a renderer\n");
	    }
	}

	s_info.dbox_dirty_objects = eina_list_append(s_info.dbox_dirty_objects, data);
    } else {
	if (s_info.renderer) {
	    ecore_animator_del(s_info.renderer);
	    s_info.renderer = NULL;
	}

	/* Need a choice
	 * Do we have to discard these all changes? or just flush them?
	 struct widget_data *item;
	 EINA_LIST_FREE(s_info.dbox_dirty_objects, item) {
	 dynamicbox_event_dbox_updated(item);
	 }
	 */
	eina_list_free(s_info.dbox_dirty_objects);
	s_info.dbox_dirty_objects = NULL;
	dynamicbox_event_dbox_updated(data);
    }
}

static void append_gbar_dirty_object_list(struct widget_data *data, int idx)
{
    data->is.field.gbar_dirty = 1;

    if (idx != DBOX_KEEP_BUFFER) {
	data->gbar_latest_idx = idx;
    }

    if (dynamicbox_visibility(data->handle) != DBOX_SHOW) {
	return;
    }

    if (s_info.conf.field.render_animator) {
	if (eina_list_data_find(s_info.gbar_dirty_objects, data)) {
	    return;
	}

	if (!s_info.renderer) {
	    s_info.renderer = ecore_animator_add(renderer_cb, NULL);
	    if (!s_info.renderer) {
		ErrPrint("Failed to create a renderer\n");
	    }
	}

	s_info.gbar_dirty_objects = eina_list_append(s_info.gbar_dirty_objects, data);
    } else {
	if (s_info.renderer) {
	    ecore_animator_del(s_info.renderer);
	    s_info.renderer = NULL;
	}

	/* Need a choice
	 * Do we have to discard these all changes? or just flush them?
	 struct widget_data *item;
	 EINA_LIST_FREE(s_info.gbar_dirty_objects, item) {
	 dynamicbox_event_gbar_updated(item);
	 }
	 */
	eina_list_free(s_info.gbar_dirty_objects);
	s_info.gbar_dirty_objects = NULL;
	dynamicbox_event_gbar_updated(data);
    }
}

static void widget_add(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = calloc(1, sizeof(*data));
    if (!data) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return;
    }

    data->state = WIDGET_DATA_CREATED;
    data->dynamicbox = dynamicbox;
    data->is.field.permanent_delete = 0;
    data->dbox_latest_idx = DBOX_PRIMARY_BUFFER;
    data->gbar_latest_idx = DBOX_PRIMARY_BUFFER;
    evas_object_smart_data_set(data->dynamicbox, data);
    widget_ref(data);

    s_info.list = eina_list_append(s_info.list, dynamicbox);
    return;
}

static Evas_Object *create_image_object(struct widget_data *data)
{
    Evas_Object *img;

    img = evas_object_image_filled_add(data->e);
    if (!img) {
	ErrPrint("Failed to create an image object\n");
    } else {
	evas_object_image_colorspace_set(img, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_alpha_set(img, EINA_TRUE);
    }

    return img;
}

static void replace_dbox_pixmap_with_image(struct widget_data *data)
{
    Evas_Object *img;
    Evas_Object *dbox_content;

    dbox_content = elm_object_part_content_unset(data->dbox_layout, "dynamicbox,content");
    if (!dbox_content) {
	ErrPrint("Failed to get content object\n");
	return;
    }

    img = create_image_object(data);
    if (img) {
	Evas_Coord w;
	Evas_Coord h;
	void *content;

	evas_object_image_size_get(dbox_content, &w, &h);
	evas_object_image_size_set(img, w, h);

	content = evas_object_image_data_get(dbox_content, 0);
	if (content) {
	    evas_object_image_data_copy_set(img, content);
	}

	evas_object_image_fill_set(img, 0, 0, w, h);
	evas_object_image_pixels_dirty_set(img, EINA_TRUE);
	evas_object_image_data_update_add(img, 0, 0, w, h);

	elm_object_part_content_set(data->dbox_layout, "dynamicbox,content", img);
    } else {
	ErrPrint("Failed to create an image object\n");
    }

    evas_object_del(dbox_content);
}

static void replace_gbar_pixmap_with_image(struct widget_data *data)
{
    Evas_Object *img;
    Evas_Object *gbar_content;

    gbar_content = elm_object_part_content_unset(data->dbox_layout, "gbar,content");
    if (!gbar_content) {
	ErrPrint("Failed to get content object\n");
	return;
    }

    img = create_image_object(data);
    if (img) {
	Evas_Coord w;
	Evas_Coord h;
	void *content;

	evas_object_image_size_get(gbar_content, &w, &h);
	evas_object_image_size_set(img, w, h);

	content = evas_object_image_data_get(gbar_content, 0);
	if (content) {
	    evas_object_image_data_copy_set(img, content);
	}

	evas_object_image_fill_set(img, 0, 0, w, h);
	evas_object_image_pixels_dirty_set(img, EINA_TRUE);
	evas_object_image_data_update_add(img, 0, 0, w, h);

	elm_object_part_content_set(data->dbox_layout, "gbar,content", img);
    } else {
	ErrPrint("Failed to create an image object\n");
    }

    evas_object_del(gbar_content);
}

static void dbox_destroy_dbox_cb(struct dynamicbox *handle, int ret, void *_data)
{
    struct widget_data *data = _data;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (data->dbox_pixmap) {
	replace_dbox_pixmap_with_image(data);
    }

    if (data->gbar_pixmap) {
	replace_gbar_pixmap_with_image(data);
    }

    data->is.field.send_delete = 0;
    DbgPrint("Invoke raw delete %s\n", data->dbox_id);
    invoke_raw_event_callback(DYNAMICBOX_EVAS_RAW_DELETE, data->dbox_id, NULL, ret);
    remove_dbox_dirty_object_list(data);
    remove_gbar_dirty_object_list(data); /* for the safety */
    widget_unref(data);
}

static void widget_del(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (data->is.field.deleted == 1) {
	DbgPrint("Already deleted: %s\n", data->dbox_id);
	return;
    }

    data->is.field.deleted = 1;

    s_info.list = eina_list_remove(s_info.list, dynamicbox);

    if (data->handle) {
	dynamicbox_set_data(data->handle, NULL);

	if (data->is.field.send_delete) {
	    int delete_type;

	    if (data->is.field.permanent_delete) {
		delete_type = DBOX_DELETE_PERMANENTLY;
	    } else {
		delete_type = DBOX_DELETE_TEMPORARY;
	    }
	    DbgPrint("Send delete request (0x%X)\n", delete_type);

	    if (data->is.field.created) {
		if (dynamicbox_del(data->handle, delete_type, dbox_destroy_dbox_cb, widget_ref(data)) < 0) {
		    widget_unref(data);
		}
	    } else {
		DbgPrint("Not created yet. this will be canceld by created callback, ignore delete callback\n");
		if (dynamicbox_del(data->handle, delete_type, NULL, NULL) < 0) {
		    DbgPrint("Unref %p\n", data);
		}
	    }
	} else {
	    DbgPrint("Skip delete request\n");
	}
    } else {
	DbgPrint("Handle is not created: %s\n", data->dbox_id);
    }

    /**
     * From now, the dynamicbox object is not valid
     */
    data->dynamicbox = NULL;
    widget_unref(data);
}

static void update_visibility(struct widget_data *data)
{
    int is_visible = 0;

    if (!data->handle || !data->is.field.created) {
	return;
    }

    if (data->is.field.freeze_visibility) {
	DbgPrint("Freezed visibility: %X (%s)\n", data->freezed_visibility, dynamicbox_pkgname(data->handle));
	(void)dynamicbox_set_visibility(data->handle, data->freezed_visibility);
	return;
    }

    is_visible = evas_object_visible_get(data->stage);

    if (is_visible) {
	Evas_Coord x, y, w, h;

	evas_object_geometry_get(data->dbox_layout, &x, &y, &w, &h);

	if (!s_info.conf.field.user_view_port) {
	    Ecore_Evas *ee;

	    ee = ecore_evas_ecore_evas_get(data->e);
	    if (ee) {
		ecore_evas_geometry_get(ee, &data->view_port.x, &data->view_port.y, &data->view_port.w, &data->view_port.h);
	    } else {
		data->view_port.x = 0;
		data->view_port.y = 0;
		ecore_x_window_size_get(0, &data->view_port.w, &data->view_port.h);
		ErrPrint("Failed to get view port info (Fallback: %dx%d - %dx%d\n",
			data->view_port.x, data->view_port.y, data->view_port.w, data->view_port.h);
	    }
	}

	if (x + w <= data->view_port.x || x >= data->view_port.x + data->view_port.w || y + h <= data->view_port.y || y >= data->view_port.y + data->view_port.h) {
	    is_visible = 0;
	} else {
	    is_visible = 1;
	}
    }

    if (is_visible) {
	(void)dynamicbox_set_visibility(data->handle, DBOX_SHOW);

	if (data->is.field.dbox_dirty) {
	    /**
	     * If the object has dirty flag, pumping it up again
	     * To updates its content
	     */
	    append_dbox_dirty_object_list(data, DBOX_KEEP_BUFFER);
	}
    } else {
	(void)dynamicbox_set_visibility(data->handle, DBOX_HIDE_WITH_PAUSE);
    }
}

static int do_force_mouse_up(struct widget_data *data)
{
    struct dynamicbox_mouse_event_info minfo;
    Evas_Coord x, y, w, h;
    struct dynamicbox_evas_event_info info;

    if (s_info.conf.field.auto_render_selector && s_info.conf.field.render_animator == 0) {
	DbgPrint("Change to render animator\n");
	s_info.conf.field.render_animator = 1;
    }

    if (!data->is.field.pressed) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    evas_object_geometry_get(data->dynamicbox, &x, &y, &w, &h);

    minfo.x = (double)(data->x - x) / (double)w;
    minfo.y = (double)(data->y - y) / (double)h;

    data->is.field.pressed = 0;

    reset_scroller(data);

    if (s_info.conf.field.auto_feed && data->is.field.mouse_event) {
	DbgPrint("%x\n", data->is.field.cancel_click);
	if (data->is.field.cancel_click != CANCEL_PROCESSED) {
	    DbgPrint("ON_HOLD send\n");
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);
	    data->is.field.cancel_click = CANCEL_PROCESSED;
	}

	minfo.x = (double)data->down.geo.x / (double)data->down.geo.w;
	minfo.y = (double)data->down.geo.y / (double)data->down.geo.h;

	dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_UNSET, &minfo);
    } else {
	if (!data->is.field.mouse_event) {
	    /* We have to keep the first position of touch down */
	    minfo.x = (double)(data->down.x - x) / (double)w;
	    minfo.y = (double)(data->down.y - y) / (double)h;
	}

	DbgPrint("%x\n", data->is.field.cancel_click);
	if (data->is.field.cancel_click != CANCEL_PROCESSED) {
	    DbgPrint("ON_HOLD send\n");
	    dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_ON_HOLD, &minfo);
	    data->is.field.cancel_click = CANCEL_PROCESSED;
	}

	dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_UP, &minfo);
	dynamicbox_feed_mouse_event(data->handle, DBOX_MOUSE_LEAVE, &minfo);
    }

    data->is.field.cancel_click = CANCEL_DISABLED;
    data->is.field.flick_down = 0;
    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_GBAR_CREATED;
    info.error = DBOX_STATUS_ERROR_CANCEL;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED, &info);
    DbgPrint("Flick down is canceled\n");
    return DBOX_STATUS_ERROR_NONE;
}

static void widget_move(Evas_Object *dynamicbox, Evas_Coord x, Evas_Coord y)
{
    struct widget_data *data;
    Evas_Coord w, h;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (data->gbar_layout) {
	Evas_Coord gbar_x, gbar_y, gbar_h;
	Evas_Coord prev_x, prev_y;
	Evas_Coord dbox_w, dbox_h;
	double rx;
	double ry;

	evas_object_geometry_get(data->dbox_layout, &prev_x, &prev_y, &dbox_w, &dbox_h);
	evas_object_geometry_get(data->gbar_layout, &gbar_x, &gbar_y, NULL, &gbar_h);

	gbar_x += (x - prev_x);
	gbar_y += (y - prev_y);

	evas_object_move(data->gbar_layout, gbar_x, gbar_y);

	rx = ((double)x + (double)dbox_w / 2.0f) / s_info.screen_width;
	switch (find_size_type(data, dbox_w, dbox_h)) {
	    case DBOX_SIZE_TYPE_1x1:
		if (rx < 0.25f) {
		    rx = 0.125f;
		} else if (rx < 0.5f) {
		    rx = 0.375f;
		} else if (rx < 0.75f) {
		    rx = 0.625f;
		} else if (rx < 1.0f) {
		    rx = 0.875f;
		}
		break;
	    case DBOX_SIZE_TYPE_2x1:
	    case DBOX_SIZE_TYPE_2x2:
		if (rx < 0.5f) {
		    rx = 0.25f;
		} else if (rx < 0.75f) {
		    rx = 0.5f;
		} else {
		    rx = 0.75f;
		}
		break;
	    default:
		rx = 0.5f;
		break;
	}
	if (prev_y + dbox_h + gbar_h > s_info.screen_height) {
	    ry = 1.0f;
	} else {
	    ry = 0.0f;
	}

	if (data->is.field.gbar_created) {
	    dynamicbox_move_glance_bar(data->handle, rx, ry);
	}
    }

    evas_object_move(data->stage, x, y);
    evas_object_move(data->dbox_layout, x, y);
    evas_object_geometry_get(data->dbox_layout, NULL, NULL, &w, &h);

    if (!s_info.conf.field.manual_pause_resume) {
	update_visibility(data);
    }

    if (s_info.conf.field.sensitive_move) {
	do_force_mouse_up(data);
    }
}

static int dbox_create_plug_object(struct widget_data *data)
{
    struct acquire_data acquire_data = {
	.w = 0,
	.h = 0,
	.content = NULL,
	.data = data,
    };

    DbgPrint("Plug type created\n");

    acquire_data.content = elm_object_part_content_unset(data->dbox_layout, "dynamicbox,content");
    if (acquire_data.content) {
	DbgPrint("Dynamicbox Content is already prepared: %s\n", dynamicbox_filename(data->handle));
	evas_object_del(acquire_data.content);
    }

    acquire_data.content = elm_plug_add(s_info.win);
    if (!acquire_data.content) {
	ErrPrint("Failed to add a plug object\n");
	return DBOX_STATUS_ERROR_FAULT;
    }

    DbgPrint("Try to connect to %s\n", dynamicbox_filename(data->handle));
    if (!elm_plug_connect(acquire_data.content, dynamicbox_filename(data->handle), 0, EINA_TRUE)) {
	ErrPrint("Cannot connect plug[%s]", dynamicbox_filename(data->handle));
	evas_object_del(acquire_data.content);
	return DBOX_STATUS_ERROR_FAULT;
    }

    elm_object_part_content_set(data->dbox_layout, "dynamicbox,content", acquire_data.content);

    acquire_data.w = data->dbox_width;
    acquire_data.h = data->dbox_height;
    update_dbox_geometry(&acquire_data);
    return DBOX_STATUS_ERROR_NONE;
}

static int dbox_create_image_object(struct widget_data *data)
{
    Evas_Object *front_image;
    struct acquire_data acquire_data = {
	.w = 0,
	.h = 0,
	.content = NULL,
	.data = data,
    };

    DbgPrint("Image type created\n");

    acquire_data.content = elm_object_part_content_get(data->dbox_layout, "dynamicbox,content");
    if (!acquire_data.content) {
	acquire_data.content = elm_layout_add(data->parent);
	if (!acquire_data.content) {
	    ErrPrint("Failed to create an edje object\n");
	    return DBOX_STATUS_ERROR_FAULT;
	}

	if (elm_layout_file_set(acquire_data.content, DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_IMG) == EINA_FALSE) {
	    ErrPrint("Failed to load edje object: %s(%s)\n", DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_IMG);
	    evas_object_del(acquire_data.content);
	    return DBOX_STATUS_ERROR_IO_ERROR;
	}

	front_image = elm_image_add(acquire_data.content);
	if (!front_image) {
	    ErrPrint("Failed to add front_image object\n");
	    evas_object_del(acquire_data.content);
	    return DBOX_STATUS_ERROR_FAULT;
	}

	DbgPrint("Default size %dx%d\n", data->dbox_width, data->dbox_height);

	elm_object_part_content_set(acquire_data.content, "front,content", front_image);
	elm_object_part_content_set(data->dbox_layout, "dynamicbox,content", acquire_data.content);
    } else {
	front_image = elm_object_part_content_get(acquire_data.content, "front,content");
	if (!front_image) {
	    ErrPrint("Unable to get front,content object\n");
	    front_image = elm_image_add(acquire_data.content);
	    if (!front_image) {
		ErrPrint("Failed to add front_image object\n");
		return DBOX_STATUS_ERROR_FAULT;
	    }

	    elm_object_part_content_set(acquire_data.content, "front,content", front_image);
	}
    }

    /*
       evas_object_geometry_get(data->dynamicbox, NULL, NULL, &acquire_data.w, &acquire_data.h);
       DbgPrint("Default size %dx%d\n", acquire_data.w, acquire_data.h);
       DbgPrint("Image size: %dx%d\n", acquire_data.w, acquire_data.h);
     */
    acquire_data.w = data->dbox_width;
    acquire_data.h = data->dbox_height;
    update_dbox_geometry(&acquire_data);
    return DBOX_STATUS_ERROR_NONE;
}

static int dbox_create_buffer_object(struct widget_data *data)
{
    Evas_Object *dbox_content;

    dbox_content = elm_object_part_content_get(data->dbox_layout, "dynamicbox,content");
    if (!dbox_content) {
	dbox_content = evas_object_image_filled_add(data->e);
	if (!dbox_content) {
	    ErrPrint("Failed to create an image object\n");
	    return DBOX_STATUS_ERROR_FAULT;
	}

	evas_object_image_colorspace_set(dbox_content, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_alpha_set(dbox_content, EINA_TRUE);
	elm_object_part_content_set(data->dbox_layout, "dynamicbox,content", dbox_content);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static int dbox_create_text_object(struct widget_data *data)
{
    ErrPrint("Unsupported\n");

    /*!
     * \todo
     */

    return DBOX_STATUS_ERROR_NOT_IMPLEMENTED;
}

static int dbox_create_pixmap_object(struct widget_data *data)
{
    Evas_Object *dbox_content;

    dbox_content = elm_object_part_content_get(data->dbox_layout, "dynamicbox,content");
    if (!dbox_content) {
	dbox_content = evas_object_image_filled_add(data->e);
	if (!dbox_content) {
	    ErrPrint("Failed to create an image object\n");
	    return DBOX_STATUS_ERROR_FAULT;
	}

	evas_object_image_colorspace_set(dbox_content, EVAS_COLORSPACE_ARGB8888);
	evas_object_image_alpha_set(dbox_content, EINA_TRUE);
	evas_object_event_callback_add(dbox_content, EVAS_CALLBACK_DEL, dbox_pixmap_del_cb, data);

	elm_object_part_content_set(data->dbox_layout, "dynamicbox,content", dbox_content);
    }

    return DBOX_STATUS_ERROR_NONE;
}

static void dbox_resize_pixmap_object(struct widget_data *data)
{
    DbgPrint("Dynamicbox resize request is succssfully sent\n");
}

static void update_dbox_pixmap(Evas_Object *content, int w, int h)
{
    evas_object_image_pixels_dirty_set(content, EINA_TRUE);
    evas_object_image_data_update_add(content, 0, 0, w, h);
    evas_object_show(content);
}

static void acquire_dbox_extra_resource_cb(struct dynamicbox *handle, int pixmap, void *cbdata)
{
    DbgPrint("Acquired: %u\n", (unsigned int)pixmap);
}

static void acquire_gbar_extra_resource_cb(struct dynamicbox *handle, int pixmap, void *cbdata)
{
    DbgPrint("Acquired: %u\n", (unsigned int)pixmap);
}

static void replace_pixmap(struct dynamicbox *handle, int gbar, Evas_Object *content, unsigned int pixmap)
{
    Evas_Native_Surface *old_surface;
    Evas_Native_Surface surface;

    surface.version = EVAS_NATIVE_SURFACE_VERSION;
    surface.type = EVAS_NATIVE_SURFACE_X11;
    surface.data.x11.pixmap = pixmap;

    old_surface = evas_object_image_native_surface_get(content);
    if (!old_surface) {
	surface.data.x11.visual = ecore_x_default_visual_get(ecore_x_display_get(), ecore_x_default_screen_get());

	evas_object_image_native_surface_set(content, &surface);

	DbgPrint("Created: %u\n", surface.data.x11.pixmap);
    } else {
	unsigned int old_pixmap;

	old_pixmap = old_surface->data.x11.pixmap;

	surface.data.x11.visual = old_surface->data.x11.visual;
	evas_object_image_native_surface_set(content, &surface);

	if (old_pixmap && handle) {
	    dynamicbox_release_resource_id(handle, gbar, old_pixmap);
	}
    }
}

static void acquire_dbox_pixmap_cb(struct dynamicbox *handle, int pixmap, void *cbdata)
{
    struct acquire_data *acquire_data = cbdata;
    struct widget_data *data = acquire_data->data;

    data->is.field.dbox_pixmap_acquire_requested = 0;

    if (pixmap == 0) {
	DbgPrint("Pixmap gotten (0)\n");
	free(acquire_data);
	widget_unref(data);
	return;
    }

    evas_object_image_size_set(acquire_data->content, acquire_data->w, acquire_data->h);
    evas_object_image_fill_set(acquire_data->content, 0, 0, acquire_data->w, acquire_data->h);
    DbgPrint("fillset: %dx%d\n", acquire_data->w, acquire_data->h);

    replace_pixmap(handle, 0, acquire_data->content, (unsigned int)pixmap);

    data->dbox_pixmap = pixmap;

    append_dbox_dirty_object_list(data, DBOX_KEEP_BUFFER);
    update_dbox_geometry(acquire_data);

    widget_unref(data);
    free(acquire_data);
}

static void dbox_update_pixmap_object(struct widget_data *data, Evas_Object *dbox_content, int w, int h)
{
    int ret;
    struct acquire_data *acquire_data;

    if (data->dbox_latest_idx == DBOX_PRIMARY_BUFFER) {
	if (data->dbox_pixmap == dynamicbox_resource_id(data->handle, 0)) {
	    if (data->dbox_extra) {
		/* Just replace the pixmap in this case, do not release old pixmap */
		replace_pixmap(NULL, 0, dbox_content, data->dbox_pixmap);
	    }

	    update_dbox_pixmap(dbox_content, w, h);
	    return;
	}

	if (data->is.field.dbox_pixmap_acquire_requested) {
	    return;
	}

	acquire_data = malloc(sizeof(*acquire_data));
	if (!acquire_data) {
	    ErrPrint("malloc: %s\n", strerror(errno));
	    return;
	}

	acquire_data->data = widget_ref(data);
	acquire_data->content = dbox_content;
	acquire_data->w = w;
	acquire_data->h = h;

	ret = dynamicbox_acquire_resource_id(data->handle, 0, acquire_dbox_pixmap_cb, acquire_data);
	if (ret != DBOX_STATUS_ERROR_NONE) {
	    widget_unref(data);
	    free(acquire_data);
	} else {
	    data->is.field.dbox_pixmap_acquire_requested = 1;
	}
    } else {
	if (!data->dbox_extra) {
	    ErrPrint("Extra buffer is not prepared yet\n");
	    return;
	}

	replace_pixmap(NULL, 0, dbox_content, data->dbox_extra[data->dbox_latest_idx]);
	update_dbox_pixmap(dbox_content, w, h);
    }
}

static void dbox_created_cb(struct dynamicbox *handle, int ret, void *cbdata)
{
    struct widget_data *data = cbdata;
    struct dynamicbox_evas_event_info info;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p (%d), %s\n", data, ret, dynamicbox_pkgname(handle));
	return;
    }

    if (ret != DBOX_STATUS_ERROR_NONE) {
	DbgPrint("Failed to create: %X\n", ret);
	data->handle = NULL;

	if (!data->is.field.deleted) {
	    struct dynamicbox_evas_event_info fault_event;

	    fault_event.error = ret;
	    fault_event.pkgname = data->dbox_id;
	    fault_event.event = DBOX_EVENT_CREATED;

	    if (!data->is.field.faulted) {
		data->is.field.faulted = 1;
		dbox_overlay_faulted(data);
	    }

	    DbgPrint("Display tap to load (%p) [%s]\n", data, data->dbox_id);
	    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_CREATE_ABORTED, &fault_event);

	    ret = DBOX_STATUS_ERROR_FAULT;
	} else {
	    ret = DBOX_STATUS_ERROR_CANCEL;
	}

	data->is.field.send_delete = 0;
	DbgPrint("Invoke raw delete %s\n", data->dbox_id);
	invoke_raw_event_callback(DYNAMICBOX_EVAS_RAW_DELETE, data->dbox_id, data->dynamicbox, ret);
	widget_unref(data);
	return;
    }

    switch (dynamicbox_type(handle, 0)) {
	case DBOX_CONTENT_TYPE_IMAGE:
	    ret = dbox_create_image_object(data);
	    break;
	case DBOX_CONTENT_TYPE_RESOURCE_ID:
	    if (!s_info.conf.field.force_to_buffer) {
		ret = dbox_create_pixmap_object(data);
		break;
	    }
	case DBOX_CONTENT_TYPE_BUFFER:
	    ret = dbox_create_buffer_object(data);
	    break;
	case DBOX_CONTENT_TYPE_TEXT:
	    ret = dbox_create_text_object(data);
	    break;
	case DBOX_CONTENT_TYPE_UIFW:
	    ret = dbox_create_plug_object(data);
	    break;
	case DBOX_CONTENT_TYPE_INVALID:
	default:
	    ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	    break;
    }

    if (ret == DBOX_STATUS_ERROR_NONE) {
	info.error = DBOX_STATUS_ERROR_NONE;
	info.pkgname = data->dbox_id;
	info.event = DBOX_EVENT_CREATED;

	data->is.field.created = 1;

	update_visibility(data);
	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_CREATED, &info);
	DbgPrint("Invoke raw create %s\n", data->dbox_id);
	invoke_raw_event_callback(DYNAMICBOX_EVAS_RAW_CREATE, data->dbox_id, data->dynamicbox, ret);

	/**
	 * In case of using the direct update path,
	 * sometimes the provider can send the updated event faster than created event.
	 * In that case, the viewer cannot recognize the updated content of a dbox.
	 * So for the safety, I added this to forcely update the dbox at the first time
	 * Right after creating its instance.
	 */
	append_dbox_dirty_object_list(data, DBOX_KEEP_BUFFER);
    } else {
	info.error = ret;
	info.pkgname = data->dbox_id;
	info.event = DBOX_EVENT_CREATED;
	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_CREATE_ABORTED, &info);
	data->is.field.send_delete = 0;
	DbgPrint("Invoke raw delete %s\n", data->dbox_id);
	invoke_raw_event_callback(DYNAMICBOX_EVAS_RAW_DELETE, data->dbox_id, data->dynamicbox, ret);
    }

    widget_unref(data);
}

static void dbox_resize_image_object(struct widget_data *data)
{
    DbgPrint("Dynamicbox resize request is succssfully sent\n");
}

static void dbox_resize_buffer_object(struct widget_data *data)
{
    DbgPrint("Dynamicbox resize request is succssfully sent\n");
}

static void dbox_resize_text_object(struct widget_data *data)
{
    DbgPrint("Dynamicbox resize request is succssfully sent\n");
}

static void dbox_resize_cb(struct dynamicbox *handle, int ret, void *cbdata)
{
    struct widget_data *data = cbdata;
    struct dynamicbox_evas_event_info info;

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    if (ret != DBOX_STATUS_ERROR_NONE) {
	info.error = ret;
	info.event = DBOX_EVENT_DBOX_SIZE_CHANGED;
	info.pkgname = data->dbox_id;
	smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_RESIZE_ABORTED, &info);
	widget_unref(data);
	return;
    }

    switch (dynamicbox_type(handle, 0)) {
	case DBOX_CONTENT_TYPE_IMAGE:
	    dbox_resize_image_object(data);
	    break;
	case DBOX_CONTENT_TYPE_RESOURCE_ID:
	    if (!s_info.conf.field.force_to_buffer) {
		dbox_resize_pixmap_object(data);
		break;
	    }
	case DBOX_CONTENT_TYPE_BUFFER:
	    dbox_resize_buffer_object(data);
	    break;
	case DBOX_CONTENT_TYPE_TEXT:
	    dbox_resize_text_object(data);
	    break;
	case DBOX_CONTENT_TYPE_UIFW:
	    break;
	case DBOX_CONTENT_TYPE_INVALID:
	    break;
	default:
	    break;
    }

    info.error = ret;
    info.event = DBOX_EVENT_DBOX_SIZE_CHANGED;
    info.pkgname = data->dbox_id;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_RESIZED, &info);
    widget_unref(data);
}

static void gbar_overlay_disable(struct widget_data *data)
{
    if (!data->gbar_layout) {
	return;
    }

    if (!data->is.field.gbar_overlay_loaded) {
	return;
    }

    elm_object_signal_emit(data->gbar_layout, "disable", "overlay");
    data->is.field.gbar_overlay_loaded = 0;
}

static void gbar_overlay_loading(struct widget_data *data)
{
    Evas_Object *rect;

    if (data->is.field.gbar_overlay_loaded) {
	ErrPrint("Already loaded");
	return;
    }

    rect = elm_object_part_content_unset(data->gbar_layout, "overlay,content");
    if (rect) {
	evas_object_del(rect);
    }

    rect = evas_object_rectangle_add(data->e);
    evas_object_color_set(rect, 0, 0, 0, 0);
    evas_object_show(rect);
    /*!
     * \todo
     * Overlay for loading a GBAR
     */

    elm_object_part_content_set(data->gbar_layout, "overlay,content", rect);
    elm_object_signal_emit(data->gbar_layout, "enable", "overlay");

    data->is.field.gbar_overlay_loaded = 1;
}

static Evas_Object *dbox_load_overlay_edje(struct widget_data *data)
{
    Evas_Object *overlay;

    overlay = elm_layout_add(data->parent);
    if (!overlay) {
	ErrPrint("Failed to create a overlay\n");
	return NULL;
    }

    if (elm_layout_file_set(overlay, DYNAMICBOX_EVAS_RESOURCE_EDJ, DYNAMICBOX_EVAS_RESOURCE_OVERLAY_LOADING) == EINA_FALSE) {
	ErrPrint("Failed to load overlay file\n");
	evas_object_del(overlay);
	return NULL;
    }

    elm_object_part_content_set(data->dbox_layout, "overlay,content", overlay);
    return overlay;
}

static Eina_Bool delayed_overlay_disable_cb(void *_data)
{
    struct widget_data *data = _data;

    elm_object_signal_emit(data->dbox_layout, "disable", "overlay");

    data->is.field.dbox_overlay_loaded = 0;
    data->overlay_update_counter = DEFAULT_OVERLAY_COUNTER;
    data->overlay_timer = NULL;
    return ECORE_CALLBACK_CANCEL;
}

static void dbox_overlay_disable(struct widget_data *data, int no_timer)
{
    if (!data->dbox_layout) {
	return;
    }

    if (!data->is.field.dbox_overlay_loaded) {
	return;
    }

    data->overlay_update_counter--;
    if (data->overlay_update_counter <= 0) {
	if (no_timer) {
	    if (data->overlay_timer) {
		ecore_timer_del(data->overlay_timer);
		data->overlay_timer = NULL;
	    }
	    delayed_overlay_disable_cb(data);
	} else {
	    if (data->overlay_timer) {
		ecore_timer_del(data->overlay_timer);
		data->overlay_timer = NULL;
		delayed_overlay_disable_cb(data);
	    } else {
		return;
	    }
	}
    }

    if (!no_timer && !data->overlay_timer) {
	data->overlay_timer = ecore_timer_add(DEFAULT_OVERLAY_WAIT_TIME, delayed_overlay_disable_cb, data);
	if (!data->overlay_timer) {
	    ErrPrint("Failed to create a timer\n");
	    delayed_overlay_disable_cb(data);
	}
    }
}

static void dbox_overlay_loading(struct widget_data *data)
{
    struct acquire_data acquire_data;
    Evas_Object *overlay;

    if (data->is.field.disable_loading == 1) {
	DbgPrint("Loading overlay is disabled");
	return;
    }

    if (data->is.field.dbox_overlay_loaded == 1) {
	DbgPrint("Overlay is already loaded");
	return;
    }

    overlay = elm_object_part_content_get(data->dbox_layout, "overlay,content");
    if (!overlay) {
	overlay = dbox_load_overlay_edje(data);
	if (!overlay) {
	    return;
	}
    }

    if (!data->is.field.disable_preview) {
	char *icon;

	icon = get_package_icon(data);
	if (icon) {
	    Evas_Object *preview;

	    preview = elm_object_part_content_get(overlay, "preview");
	    if (!preview) {
		preview = elm_image_add(overlay);
	    }

	    if (preview) {
		elm_image_file_set(preview, icon, NULL);
		elm_object_part_content_set(overlay, "preview", preview);
	    }

	    free(icon);
	}

	DbgPrint("Set overlay loading (%p) %s\n", data, data->dbox_id);
    } else {
	DbgPrint("Overlay is disabled (%s)\n", data->dbox_id);
    }

    elm_object_part_text_set(overlay, "text", _("IDS_IDLE_POP_LOADING_ING"));
    if (data->is.field.disable_text) {
	elm_object_signal_emit(overlay, "disable", "text");
    }

    elm_object_signal_emit(data->dbox_layout, "reset", "overlay");
    elm_object_signal_emit(data->dbox_layout, "enable", "overlay");

    evas_object_geometry_get(data->dynamicbox, NULL, NULL, &acquire_data.w, &acquire_data.h);
    acquire_data.content = NULL;
    acquire_data.data = data;
    update_dbox_geometry(&acquire_data);

    data->is.field.dbox_overlay_loaded = 1;
    data->overlay_update_counter = DEFAULT_OVERLAY_COUNTER;
}

static void dbox_overlay_faulted(struct widget_data *data)
{
    struct acquire_data acquire_data;
    Evas_Object *overlay;

    if (data->is.field.dbox_overlay_loaded) {
	data->overlay_update_counter = 0;
	dbox_overlay_disable(data, 1);
    }

    overlay = elm_object_part_content_get(data->dbox_layout, "overlay,content");
    if (!overlay) {
	overlay = dbox_load_overlay_edje(data);
	if (!overlay) {
	    return;
	}
    }

    if (dynamicbox_type(data->handle, 0) != DBOX_CONTENT_TYPE_IMAGE) {
	Evas_Object *preview;

	preview = elm_object_part_content_get(overlay, "preview");
	if (!preview) {
	    char *icon;

	    icon = dynamicbox_service_preview(data->dbox_id, data->size_type);
	    if (icon) {
		preview = elm_image_add(data->dbox_layout);
		if (preview) {
		    elm_image_file_set(preview, icon, NULL);
		    elm_object_part_content_set(overlay, "preview", preview);
		}

		free(icon);
	    }
	}
    }

    DbgPrint("Set overlay fault (%p) %s\n", data, data->dbox_id);
    elm_object_part_text_set(overlay, "text", _("IDS_HS_BODY_UNABLE_TO_LOAD_DATA_TAP_TO_RETRY"));
    elm_object_signal_emit(overlay, "enable", "text");
    elm_object_signal_emit(data->dbox_layout, "reset", "overlay");
    elm_object_signal_emit(data->dbox_layout, "enable", "overlay");

    evas_object_geometry_get(data->dynamicbox, NULL, NULL, &acquire_data.w, &acquire_data.h);
    acquire_data.content = NULL;
    acquire_data.data = data;
    update_dbox_geometry(&acquire_data);
    data->is.field.dbox_overlay_loaded = 1;
}

static void widget_resize(Evas_Object *dynamicbox, Evas_Coord w, Evas_Coord h)
{
    struct widget_data *data;
    int type;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    type = find_size_type(data, w, h);
    if (type == DBOX_SIZE_TYPE_UNKNOWN) {
	ErrPrint("Invalid size: %dx%d\n", w, h);
	//return;
    } else if (s_info.conf.field.use_fixed_size) {
	if (dynamicbox_service_get_size(type, &w, &h) < 0) {
	    ErrPrint("Failed to get box size\n");
	}
    }

    data->dbox_width = w;
    data->dbox_height = h;
    data->size_type = type;

    if (data->is.field.faulted) {
	evas_object_resize(data->dbox_layout, data->dbox_width, data->dbox_height);
	ErrPrint("Faulted DBox, skip resizing (%s)\n", data->dbox_id);
	return;
    }

    if (!data->handle) {
	struct acquire_data acquire_data = {
	    .data = data,
	};
	DbgPrint("Create new handle: %dx%d, (%s, %s), %s/%s\n", data->dbox_width, data->dbox_height,
		data->dbox_id, data->content,
		data->cluster, data->category);
	if (dynamicbox_activate(data->dbox_id, NULL, NULL) < 0) {
	    ErrPrint("Activate: %s\n", data->dbox_id);
	}
	data->is.field.created = 0;
	data->is.field.send_delete = 1;
	update_dbox_geometry(&acquire_data);

	data->handle = dynamicbox_add(data->dbox_id, data->content,
		data->cluster, data->category,
		data->period, type,
		dbox_created_cb, widget_ref(data));
	if (!data->handle) {
	    ErrPrint("Failed to send add request\n");
	    DbgPrint("Unref %p %s\n", data, data->dbox_id);
	    widget_unref(data);
	    return;
	}

	DbgPrint("Added handle: %p (%p)\n", data->handle, data);
	dynamicbox_set_data(data->handle, dynamicbox);
	dbox_overlay_loading(data);
	data->is.field.touch_effect = dynamicbox_service_touch_effect(data->dbox_id, type);
	data->is.field.mouse_event = dynamicbox_service_mouse_event(data->dbox_id, type);
    } else {
	int ret;

	DbgPrint("Resize to %dx%d\n", w, h);

	if (type > 0 && type != DBOX_SIZE_TYPE_UNKNOWN) {
	    ret = dynamicbox_resize(data->handle, type, dbox_resize_cb, widget_ref(data));
	} else {
	    ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	    /* This will be decreased soon ... */
	    widget_ref(data);
	}

	evas_object_resize(data->dbox_layout, data->dbox_width, data->dbox_height);
	if (ret == DBOX_STATUS_ERROR_ALREADY) {
	    DbgPrint("Same size\n");
	    widget_unref(data);
	} else if (ret == DBOX_STATUS_ERROR_NONE) {
	    DbgPrint("Resize request is successfully sent\n");
	    data->is.field.touch_effect = dynamicbox_service_touch_effect(data->dbox_id, type);
	    data->is.field.mouse_event = dynamicbox_service_mouse_event(data->dbox_id, type);
	} else {
	    widget_unref(data);
	}
    }
}

static void widget_show(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_show(data->stage);
    evas_object_show(data->dbox_layout);

    update_visibility(data);
}

static void widget_hide(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_hide(data->stage);
    evas_object_hide(data->dbox_layout);

    update_visibility(data);
}

static void widget_color_set(Evas_Object *dynamicbox, int r, int g, int b, int a)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_color_set(data->stage, r, g, b, a);
}

static void widget_clip_set(Evas_Object *dynamicbox, Evas_Object *clip)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_clip_set(data->stage, clip);
}

static void widget_clip_unset(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = evas_object_smart_data_get(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->state != WIDGET_DATA_CREATED) {
	ErrPrint("Invalid widget data: %p\n", data);
	return;
    }

    evas_object_clip_unset(data->stage);
}

/*!
 * This must be called before update_gbar_geometry
 */
static void update_stage_geometry(struct acquire_data *acquire_data)
{
    Evas_Coord dbox_x, dbox_y, dbox_w, dbox_h;
    Evas_Coord stage_w, stage_h;

    evas_object_geometry_get(acquire_data->data->dbox_layout, &dbox_x, &dbox_y, &dbox_w, &dbox_h);

    const int delta_y_top    = (acquire_data->h - dbox_y);
    const int delta_y_bottom = (acquire_data->h - (s_info.screen_height - dbox_y - dbox_h));

    stage_w = dbox_w > acquire_data->w ? dbox_w : acquire_data->w;
    stage_h = dbox_h + acquire_data->h;// - delta_y_top;

    if(delta_y_top >= delta_y_bottom)
    {
	evas_object_move(acquire_data->data->stage, 0, dbox_y);
    }
    else
    {
	evas_object_move(acquire_data->data->stage, 0, dbox_y - acquire_data->h);
    }

    evas_object_resize(acquire_data->data->stage, stage_w, stage_h);
}

static void update_gbar_geometry(struct acquire_data *acquire_data)
{
    Evas_Coord dbox_x, dbox_y, dbox_w, dbox_h;

    evas_object_geometry_get(acquire_data->data->dbox_layout, &dbox_x, &dbox_y, &dbox_w, &dbox_h);

    //How much of the GBAR is outside the screen
    const int delta_y_top    = (acquire_data->h - dbox_y) < 0 ? 0 : acquire_data->h - dbox_y;
    const int delta_y_bottom = (acquire_data->h - (s_info.screen_height - dbox_y - dbox_h)) < 0 ? 0 : (acquire_data->h - (s_info.screen_height - dbox_y - dbox_h));

    //If more of the GBAR is outside the top side draw at the bottom, otherwise draw at the top
    if(delta_y_top >= delta_y_bottom)
    {
	evas_object_move(acquire_data->data->gbar_layout, 0, dbox_y + dbox_h - delta_y_bottom);
	effect_resize(acquire_data->data->gbar_layout, acquire_data->w, acquire_data->h, EFFECT_HEIGHT);
    }
    else
    {
	evas_object_move(acquire_data->data->gbar_layout, 0, dbox_y + delta_y_top);
	effect_resize(acquire_data->data->gbar_layout, acquire_data->w, acquire_data->h, EFFECT_HEIGHT|EFFECT_MOVE);
    }
}

static void update_dbox_geometry(struct acquire_data *acquire_data)
{
    Evas_Coord dbox_x, dbox_y, dbox_w, dbox_h;
    Evas_Coord stage_w, stage_h;
    struct widget_data *data = acquire_data->data;

    evas_object_resize(data->dbox_layout, data->dbox_width, data->dbox_height);
    evas_object_geometry_get(data->dbox_layout, &dbox_x, &dbox_y, &dbox_w, &dbox_h);

    if (data->gbar_layout) {
	Evas_Coord gbar_x, gbar_y, gbar_w, gbar_h;

	evas_object_geometry_get(data->gbar_layout, &gbar_x, &gbar_y, &gbar_w, &gbar_h);
	if (dbox_y + dbox_h + gbar_h > s_info.screen_height) {
	    evas_object_move(data->gbar_layout, 0, dbox_y - gbar_h);
	    evas_object_move(data->stage, 0, dbox_y - gbar_h);
	} else {
	    evas_object_move(data->gbar_layout, 0, dbox_y + dbox_h);
	    evas_object_move(data->stage, 0, dbox_y);
	}

	stage_w = gbar_w > dbox_w ? gbar_w : dbox_w;
	stage_h = dbox_h + gbar_h;
    } else {
	stage_w = dbox_w;
	if (s_info.conf.field.support_gbar) {
	    stage_h = dbox_h + 100; /* Reserve 100 px for effect */
	} else {
	    stage_h = dbox_h;
	}

	evas_object_move(data->stage, dbox_x, dbox_y);
    }

    evas_object_resize(data->stage, stage_w, stage_h);
}

static void dbox_update_image_object(struct widget_data *data, Evas_Object *dbox_content, int w, int h)
{
    Evas_Object *front_image;

    front_image = elm_object_part_content_get(dbox_content, "front,content");
    if (front_image) {
	elm_image_file_set(front_image, dynamicbox_filename(data->handle), NULL);
    } else {
	ErrPrint("Image object not found\n");
    }
}

static void dbox_update_buffer_object(struct widget_data *data, Evas_Object *dbox_content, int w, int h)
{
    struct acquire_data acquire_data = {
	.w = w,
	.h = h,
	.content = dbox_content,
	.data = data,
    };

    if (data->dbox_fb) {
	dynamicbox_release_buffer(data->dbox_fb);
	data->dbox_fb = NULL;
    }

    data->dbox_fb = dynamicbox_acquire_buffer(data->handle, 0);
    if (!data->dbox_fb) {
	ErrPrint("Failed to get fb\n");
	return;
    }

    evas_object_image_size_set(dbox_content, w, h);

    if (dynamicbox_acquire_buffer_lock(data->handle, 0) < 0) {
	ErrPrint("Failed to acquire lock\n");
    }
    evas_object_image_data_copy_set(dbox_content, data->dbox_fb);
    if (dynamicbox_release_buffer_lock(data->handle, 0) < 0) {
	ErrPrint("Failed to release lock\n");
    }

    evas_object_image_fill_set(dbox_content, 0, 0, w, h);
    evas_object_image_pixels_dirty_set(dbox_content, EINA_TRUE);
    evas_object_image_data_update_add(dbox_content, 0, 0, w, h);
    update_dbox_geometry(&acquire_data);
}

static void dbox_update_text_object(struct widget_data *data, Evas_Object *dbox_content, int w, int h)
{
    struct acquire_data acquire_data = {
	.w = w,
	.h = h,
	.content = dbox_content,
	.data = data,
    };

    update_dbox_geometry(&acquire_data);
}

static void dynamicbox_event_extra_info_updated(struct widget_data *data)
{
    struct dynamicbox_evas_event_info info;
    const char *content_info;
    char *tmp;

    if (data->is.field.deleted) {
	DbgPrint("Box is %s, ignore update\n", data->is.field.deleted ? "deleted" : "faulted");
	return;
    }

    content_info = dynamicbox_content(data->handle);
    if (content_info && data->content) {
	if (!strcmp(content_info, data->content)) {
	    /* Nothing chnaged */
	} else {
	    tmp = strdup(content_info);
	    if (!tmp) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return;
	    }

	    free(data->content);
	    data->content = tmp;
	}
    } else if (content_info) {
	tmp = strdup(content_info);
	if (!tmp) {
	    ErrPrint("Heap: %s\n", strerror(errno));
	    return;
	}
	data->content = tmp;
    } else if (data->content) {
	free(data->content);
	data->content = NULL;
    } else {
	/* Nothing changed */
    }

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_EXTRA_INFO_UPDATED;
    info.error = DBOX_STATUS_ERROR_NONE;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_EXTRA_INFO_UPDATED, &info);
}

/*!
 * Event handlers
 */
static void dynamicbox_event_dbox_updated(struct widget_data *data)
{
    Evas_Object *dbox_content;
    int type;
    int w, h;
    struct dynamicbox_evas_event_info info;

    data->is.field.dbox_dirty = 0;

    if (data->is.field.deleted) {
	DbgPrint("Box is %s, ignore update\n", data->is.field.deleted ? "deleted" : "faulted");
	return;
    }

    dbox_content = elm_object_part_content_get(data->dbox_layout, "dynamicbox,content");
    if (!dbox_content) {
	ErrPrint("Failed to get content object\n");
	return;
    }

    type = dynamicbox_size(data->handle);
    if (type < 0 || type == DBOX_SIZE_TYPE_UNKNOWN) {
	ErrPrint("Size is not valid %X\n", type);
	return;
    }

    w = data->dbox_width;
    h = data->dbox_height;

    switch (dynamicbox_type(data->handle, 0)) {
	case DBOX_CONTENT_TYPE_IMAGE:
	    dbox_update_image_object(data, dbox_content, w, h);
	    break;
	case DBOX_CONTENT_TYPE_RESOURCE_ID:
	    if (!s_info.conf.field.force_to_buffer) {
		dbox_update_pixmap_object(data, dbox_content, w, h);
		break;
	    }
	case DBOX_CONTENT_TYPE_BUFFER:
	    dbox_update_buffer_object(data, dbox_content, w, h);
	    break;
	case DBOX_CONTENT_TYPE_TEXT:
	    dbox_update_text_object(data, dbox_content, w, h);
	    break;
	case DBOX_CONTENT_TYPE_UIFW:
	    break;
	case DBOX_CONTENT_TYPE_INVALID:
	default:
	    break;
    }

    dbox_overlay_disable(data, 0);

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_DBOX_UPDATED;
    info.error = DBOX_STATUS_ERROR_NONE;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_UPDATED, &info);
}

static void gbar_update_buffer_object(struct widget_data *data, Evas_Object *gbar_content, int w, int h)
{
    struct acquire_data acquire_data = {
	.data = data,
	.content = gbar_content,
	.w = w,
	.h = h,
    };

    if (data->gbar_fb) {
	dynamicbox_release_buffer(data->gbar_fb);
	data->gbar_fb = NULL;
    } else {
	// This is first time
	gbar_overlay_disable(data);
    }

    data->gbar_fb = dynamicbox_acquire_buffer(data->handle, 1);
    if (!data->gbar_fb) {
	ErrPrint("Failed to get fb\n");
	return;
    }

    evas_object_image_size_set(gbar_content, w, h);

    if (dynamicbox_acquire_buffer_lock(data->handle, 1) < 0) {
	ErrPrint("Failed to acquire lock\n");
    }
    evas_object_image_data_copy_set(gbar_content, data->gbar_fb);
    if (dynamicbox_release_buffer_lock(data->handle, 1) < 0) {
	ErrPrint("Failed to release lock\n");
    }

    evas_object_image_fill_set(gbar_content, 0, 0, w, h);
    evas_object_image_pixels_dirty_set(gbar_content, EINA_TRUE);
    evas_object_image_data_update_add(gbar_content, 0, 0, w, h);

    update_stage_geometry(&acquire_data);
    update_gbar_geometry(&acquire_data);
}

static void gbar_update_text_object(struct widget_data *data, Evas_Object *gbar_content, int w, int h)
{
    struct acquire_data acquire_data = {
	.data = data,
	.content = gbar_content,
	.w = w,
	.h = h,
    };

    ErrPrint("Text type is updated\n");
    gbar_overlay_disable(data);

    update_stage_geometry(&acquire_data);
    update_gbar_geometry(&acquire_data);
}

static void update_gbar_pixmap(Evas_Object *content, int w, int h)
{
    evas_object_image_pixels_dirty_set(content, EINA_TRUE);
    evas_object_image_data_update_add(content, 0, 0, w, h);
    evas_object_show(content);
}

static void acquire_gbar_pixmap_cb(struct dynamicbox *handle, int pixmap, void *cbdata)
{
    struct acquire_data *acquire_data = cbdata;
    struct widget_data *data = acquire_data->data;
    Evas_Native_Surface *old_surface;
    Evas_Native_Surface surface;

    data->is.field.gbar_pixmap_acquire_requested = 0;

    if (pixmap == 0) {
	ErrPrint("Failed to acquire pixmap\n");
	DbgPrint("Unref %p %s\n", data, data->dbox_id);
	widget_unref(data);
	free(acquire_data);
	return;
    }

    evas_object_image_size_set(acquire_data->content, acquire_data->w, acquire_data->h);
    evas_object_image_fill_set(acquire_data->content, 0, 0, acquire_data->w, acquire_data->h);

    surface.version = EVAS_NATIVE_SURFACE_VERSION;
    surface.type = EVAS_NATIVE_SURFACE_X11;
    surface.data.x11.pixmap = (unsigned int)pixmap;

    old_surface = evas_object_image_native_surface_get(acquire_data->content);
    if (!old_surface) {
	gbar_overlay_disable(data);
	surface.data.x11.visual = ecore_x_default_visual_get(ecore_x_display_get(), ecore_x_default_screen_get());
	evas_object_image_native_surface_set(acquire_data->content, &surface);
    } else {
	unsigned int old_pixmap = 0u;
	old_pixmap = old_surface->data.x11.pixmap;
	surface.data.x11.visual = old_surface->data.x11.visual;
	evas_object_image_native_surface_set(acquire_data->content, &surface);

	if (old_pixmap) {
	    dynamicbox_release_resource_id(data->handle, 1, old_pixmap);
	}
    }

    data->gbar_pixmap = (unsigned int)pixmap;

    append_gbar_dirty_object_list(data, DBOX_KEEP_BUFFER);
    update_stage_geometry(acquire_data);
    update_gbar_geometry(acquire_data);

    free(acquire_data);
    DbgPrint("Unref %p %s\n", data, data->dbox_id);
    widget_unref(data);
}

static void gbar_update_pixmap_object(struct widget_data *data, Evas_Object *gbar_content, int w, int h)
{
    struct acquire_data *acquire_data;
    int ret;

    if (data->gbar_latest_idx == DBOX_PRIMARY_BUFFER) {
	if (data->gbar_pixmap == dynamicbox_resource_id(data->handle, 1)) {
	    int ow;
	    int oh;

	    effect_size_get(gbar_content, &ow, &oh);

	    if (data->gbar_extra) {
		replace_pixmap(NULL, 1, gbar_content, data->gbar_pixmap);
	    }
	    update_gbar_pixmap(gbar_content, w, h);

	    if (ow != w || oh != h) {
		struct acquire_data adata = {
		    .data = data,
		    .content = gbar_content,
		    .w = w,
		    .h = h,
		};

		update_stage_geometry(&adata);
	    }
	    return;
	}

	if (data->is.field.gbar_pixmap_acquire_requested) {
	    return;
	}

	acquire_data = malloc(sizeof(*acquire_data));
	if (!acquire_data) {
	    ErrPrint("Heap: %s\n", strerror(errno));
	    return;
	}

	acquire_data->content = gbar_content;
	acquire_data->w = w;
	acquire_data->h = h;
	acquire_data->data = widget_ref(data);

	ret = dynamicbox_acquire_resource_id(data->handle, 1, acquire_gbar_pixmap_cb, acquire_data);
	if (ret != DBOX_STATUS_ERROR_NONE) {
	    ErrPrint("Failed to acquire gbar resource id\n");
	    free(acquire_data);
	    DbgPrint("Unref %p %s\n", data, data->dbox_id);
	    widget_unref(data);
	} else {
	    data->is.field.gbar_pixmap_acquire_requested = 1;
	}
    } else {
	int ow;
	int oh;

	if (!data->gbar_extra) {
	    DbgPrint("Extra GBar is not prepared yet\n");
	    return;
	}

	effect_size_get(gbar_content, &ow, &oh);

	replace_pixmap(NULL, 1, gbar_content, data->gbar_extra[data->gbar_latest_idx]);
	update_gbar_pixmap(gbar_content, w, h);

	if (ow != w || oh != h) {
	    struct acquire_data adata = {
		.data = data,
		.content = gbar_content,
		.w = w,
		.h = h,
	    };

	    update_stage_geometry(&adata);
	}
    }
}

static void dynamicbox_event_gbar_updated(struct widget_data *data)
{
    Evas_Object *gbar_content;
    int w, h;

    data->is.field.gbar_dirty = 0;

    if (data->is.field.deleted) {
	DbgPrint("Box is deleted, ignore update\n");
	return;
    }

    gbar_content = elm_object_part_content_get(data->gbar_layout, "gbar,content");
    if (!gbar_content) {
	ErrPrint("Failed to get content object\n");
	return;
    }

    if (dynamicbox_get_glance_bar_size(data->handle, &w, &h) != DBOX_STATUS_ERROR_NONE) {
	ErrPrint("Failed to get gbar_size\n");
	w = 0;
	h = 0;
    }

    switch (dynamicbox_type(data->handle, 1)) {
	case DBOX_CONTENT_TYPE_RESOURCE_ID:
	    if (!s_info.conf.field.force_to_buffer) {
		gbar_update_pixmap_object(data, gbar_content, w, h);
		break;
	    }
	case DBOX_CONTENT_TYPE_BUFFER:
	    gbar_update_buffer_object(data, gbar_content, w, h);
	    break;
	case DBOX_CONTENT_TYPE_TEXT:
	    gbar_update_text_object(data, gbar_content, w, h);
	    break;
	case DBOX_CONTENT_TYPE_UIFW:
	    break;
	case DBOX_CONTENT_TYPE_INVALID:
	default:
	    ErrPrint("Invalid pd type\n");
	    break;
    }
}

static void dynamicbox_event_deleted(struct widget_data *data)
{
    struct dynamicbox_evas_event_info info;

    if (data->dbox_fb) {
	dynamicbox_release_buffer(data->dbox_fb);
	data->dbox_fb = NULL;
    }

    if (data->gbar_fb) {
	dynamicbox_release_buffer(data->gbar_fb);
	data->gbar_fb = NULL;
    }

    if (data->dbox_pixmap) {
	replace_dbox_pixmap_with_image(data);
    }

    if (data->gbar_pixmap) {
	replace_gbar_pixmap_with_image(data);
    }

    DbgPrint("Dynamicbox is deleted: %p (emit signal)\n", data);
    data->is.field.send_delete = 0;
    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_DELETED;
    info.error = data->is.field.faulted ? DBOX_STATUS_ERROR_FAULT : DBOX_STATUS_ERROR_NONE;

    /**
     * Even if the dynamicbox object tries to be deleted from DBOX_DELETED event callback,
     * widget data should not be released while processing RAW_DELETE event handling
     */
    widget_ref(data);

    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_DELETED, &info);
    DbgPrint("Invoke raw delete %s\n", data->dbox_id);
    invoke_raw_event_callback(DYNAMICBOX_EVAS_RAW_DELETE, data->dbox_id, data->dynamicbox, info.error);

    remove_dbox_dirty_object_list(data);
    remove_gbar_dirty_object_list(data); /* For the safety */

    data->handle = NULL;

    /**
     * All event handler is handled correctly,
     * Then decrease the refcnt of it.
     */
    widget_unref(data);
}

static void dynamicbox_event_request_close_gbar(struct widget_data *data)
{
    int ret;

    ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
    if (ret < 0) {
	ErrPrint("Failed to close a GBAR: %x\n", ret);
	DbgPrint("Unref %p %s\n", data, data->dbox_id);
	widget_unref(data);
    }
}

static void dynamicbox_event_group_changed(struct widget_data *data)
{
    DbgPrint("Group is changed\n");
}

static void dynamicbox_event_pinup_changed(struct widget_data *data)
{
    DbgPrint("Pinup is changed\n");
}

static void dynamicbox_event_period_changed(struct widget_data *data)
{
    struct dynamicbox_evas_event_info info;

    data->period = dynamicbox_period(data->handle);
    DbgPrint("Update period is changed to (%lf)\n", data->period);

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_PERIOD_CHANGED;
    info.error = DBOX_STATUS_ERROR_NONE;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_PERIOD_CHANGED, &info);
}

static void dynamicbox_event_dbox_size_changed(struct widget_data *data)
{
    DbgPrint("Dynamicbox LB size is changed\n");
}

static void dynamicbox_event_gbar_size_changed(struct widget_data *data)
{
    DbgPrint("Dynamicbox GBAR size is changed\n");
}

static void dynamicbox_event_gbar_created(struct widget_data *data)
{
    DbgPrint("Dynamicbox GBAR is created\n");
}

static void dynamicbox_event_gbar_destroyed(struct widget_data *data)
{
    DbgPrint("Dynamicbox GBAR is destroyed\n");
    remove_gbar_dirty_object_list(data);
}

static void dynamicbox_event_hold_scroll(struct widget_data *data)
{
    struct dynamicbox_evas_event_info info;
    DbgPrint("Dynamicbox hold scroll\n");

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_HOLD_SCROLL;
    info.error = DBOX_STATUS_ERROR_NONE;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_CONTROL_SCROLLER, &info);
}

static void dynamicbox_event_release_scroll(struct widget_data *data)
{
    struct dynamicbox_evas_event_info info;
    DbgPrint("Dynamicbox release scroll\n");

    info.pkgname = data->dbox_id;
    info.event = DBOX_EVENT_RELEASE_SCROLL;
    info.error = DBOX_STATUS_ERROR_NONE;
    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_CONTROL_SCROLLER, &info);
}

static void dynamicbox_event_dbox_update_begin(struct widget_data *data)
{
    DbgPrint("DBOX Update Begin\n");
}

static void dynamicbox_event_dbox_update_end(struct widget_data *data)
{
    DbgPrint("DBOX Update End\n");
}

static void dynamicbox_event_gbar_update_begin(struct widget_data *data)
{
    DbgPrint("GBAR Update Begin\n");
}

static void dynamicbox_event_gbar_update_end(struct widget_data *data)
{
    DbgPrint("GBAR Update End\n");
}

static void dynamicbox_event_update_mode_changed(struct widget_data *data)
{
    DbgPrint("Update mode changed\n");
}

static void dynamicbox_event_ignored(struct widget_data *data)
{
    DbgPrint("Request is ignored\n");
}

static int dynamicbox_event_handler(struct dynamicbox *handle, enum dynamicbox_event_type event, void *cbdata)
{
    Evas_Object *dynamicbox;
    struct widget_data *data;
    int idx;
    unsigned int resource_id;
    int status;

    dynamicbox = dynamicbox_data(handle);
    if (!dynamicbox) {
	ErrPrint("dynamicbox object is not exists\n");
	return 0;
    }

    data = get_smart_data(dynamicbox);
    if (!data || data->is.field.deleted) {
	ErrPrint("Failed to get smart data\n");
	dynamicbox_set_data(handle, NULL);
	if (event == DBOX_EVENT_CREATED) {
	    DbgPrint("System created dynamicbox is not supported\n");
	    (void)dynamicbox_del(handle, DBOX_DELETE_PERMANENTLY, NULL, NULL);
	}
	return 0;
    }

    switch (event) {
	case DBOX_EVENT_DBOX_EXTRA_BUFFER_CREATED:
	    dynamicbox_get_affected_extra_buffer(handle, 0, &idx, &resource_id);
	    DbgPrint("Extra buffer created for DBOX: %d (%u)\n", idx, resource_id);

	    status = dynamicbox_acquire_extra_resource_id(handle, 0, idx, acquire_dbox_extra_resource_cb, data);
	    if (status < 0) {
		ErrPrint("Failed to acquire resource: %u (0x%X)\n", resource_id, status);
		break;
	    }

	    if (!data->dbox_extra) {
		data->dbox_extra = calloc(dynamicbox_option(DBOX_OPTION_EXTRA_BUFFER_CNT), sizeof(*data->dbox_extra));
		if (!data->dbox_extra) {
		    ErrPrint("calloc: %s\n", strerror(errno));
		}
	    }

	    data->dbox_extra[idx] = resource_id;
	    data->dbox_extra_cnt++;
	    break;
	case DBOX_EVENT_DBOX_EXTRA_BUFFER_DESTROYED:
	    dynamicbox_get_affected_extra_buffer(handle, 0, &idx, &resource_id);
	    DbgPrint("Extra buffer destroyed for DBOX: %d (%u)\n", idx, resource_id);
	    if (data->dbox_extra[idx] != resource_id) {
		DbgPrint("Resource Id mismatched\n");
		if (data->dbox_extra[idx] == 0u) {
		    DbgPrint("Not acquired resourced\n");
		    break;
		}
	    }

	    data->dbox_extra[idx] = 0u;
	    data->dbox_extra_cnt--;
	    if (!data->dbox_extra_cnt) {
		DbgPrint("Release dbox array\n");
		free(data->dbox_extra);
		data->dbox_extra = NULL;
	    }

	    if (dynamicbox_release_resource_id(handle, 0, resource_id) < 0) {
		ErrPrint("Failed to release resource: %u\n", resource_id);
	    }
	    break;
	case DBOX_EVENT_GBAR_EXTRA_BUFFER_CREATED:
	    dynamicbox_get_affected_extra_buffer(handle, 1, &idx, &resource_id);
	    DbgPrint("Extra buffer destroyed for GBAR: %d (%u)\n", idx, resource_id);
	    if (!data->gbar_extra) {
		data->gbar_extra = calloc(dynamicbox_option(DBOX_OPTION_EXTRA_BUFFER_CNT), sizeof(*data->gbar_extra));
		if (!data->gbar_extra) {
		    ErrPrint("calloc: %s\n", strerror(errno));
		    break;
		}
	    }

	    data->gbar_extra[idx] = resource_id;
	    data->gbar_extra_cnt++;

	    if (dynamicbox_acquire_extra_resource_id(handle, 1, idx, acquire_gbar_extra_resource_cb, data) < 0) {
		ErrPrint("Failed to acquire resource: %u\n", resource_id);
	    }
	    break;
	case DBOX_EVENT_GBAR_EXTRA_BUFFER_DESTROYED:
	    dynamicbox_get_affected_extra_buffer(handle, 1, &idx, &resource_id);
	    DbgPrint("Extra buffer destroyed for GBAR: %d (%u)\n", idx, resource_id);
	    if (data->gbar_extra[idx] != resource_id) {
		DbgPrint("Resource Id mismatched\n");
	    }
	    data->gbar_extra[idx] = 0u;
	    data->gbar_extra_cnt--;
	    if (!data->gbar_extra_cnt) {
		DbgPrint("Release gbar array\n");
		free(data->gbar_extra);
		data->gbar_extra = NULL;
	    }

	    if (dynamicbox_release_resource_id(handle, 1, resource_id) < 0) {
		ErrPrint("Failed to release resource: %u\n", resource_id);
	    }
	    break;
	case DBOX_EVENT_DBOX_EXTRA_UPDATED:
	    dynamicbox_get_affected_extra_buffer(handle, 0, &idx, &resource_id);
	    if (!data->dbox_extra) {
		ErrPrint("Extra buffer is not prepared yet\n");
	    } else {
		if (data->dbox_extra[idx] != resource_id) {
		    ErrPrint("Resource ID mismatched\n");
		}
		append_dbox_dirty_object_list(data, idx);
	    }
	    break;
	case DBOX_EVENT_GBAR_EXTRA_UPDATED:
	    dynamicbox_get_affected_extra_buffer(handle, 1, &idx, &resource_id);
	    if (!data->gbar_extra) {
		ErrPrint("Extra buffer is not prepared yet\n");
	    } else {
		if (data->gbar_extra[idx] != resource_id) {
		    ErrPrint("Resource ID mismatched\n");
		}
		append_gbar_dirty_object_list(data, idx);
	    }
	    break;
	case DBOX_EVENT_DBOX_UPDATED:
	    append_dbox_dirty_object_list(data, DBOX_PRIMARY_BUFFER);
	    break;
	case DBOX_EVENT_GBAR_UPDATED:
	    append_gbar_dirty_object_list(data, DBOX_PRIMARY_BUFFER);
	    break;
	case DBOX_EVENT_EXTRA_INFO_UPDATED:
	    dynamicbox_event_extra_info_updated(data);
	    break;

	case DBOX_EVENT_DELETED:
	    dynamicbox_event_deleted(data);
	    break;

	case DBOX_EVENT_GROUP_CHANGED:
	    dynamicbox_event_group_changed(data);
	    break;
	case DBOX_EVENT_PINUP_CHANGED:
	    dynamicbox_event_pinup_changed(data);
	    break;
	case DBOX_EVENT_PERIOD_CHANGED:
	    dynamicbox_event_period_changed(data);
	    break;

	case DBOX_EVENT_DBOX_SIZE_CHANGED:
	    dynamicbox_event_dbox_size_changed(data);
	    break;
	case DBOX_EVENT_GBAR_SIZE_CHANGED:
	    dynamicbox_event_gbar_size_changed(data);
	    break;

	case DBOX_EVENT_GBAR_CREATED:
	    dynamicbox_event_gbar_created(data);
	    break;
	case DBOX_EVENT_GBAR_DESTROYED:
	    dynamicbox_event_gbar_destroyed(data);
	    break;

	case DBOX_EVENT_HOLD_SCROLL:
	    dynamicbox_event_hold_scroll(data);
	    break;
	case DBOX_EVENT_RELEASE_SCROLL:
	    dynamicbox_event_release_scroll(data);
	    break;

	case DBOX_EVENT_DBOX_UPDATE_BEGIN:
	    dynamicbox_event_dbox_update_begin(data);
	    break;
	case DBOX_EVENT_DBOX_UPDATE_END:
	    dynamicbox_event_dbox_update_end(data);
	    break;

	case DBOX_EVENT_GBAR_UPDATE_BEGIN:
	    dynamicbox_event_gbar_update_begin(data);
	    break;
	case DBOX_EVENT_GBAR_UPDATE_END:
	    dynamicbox_event_gbar_update_end(data);
	    break;

	case DBOX_EVENT_UPDATE_MODE_CHANGED:
	    dynamicbox_event_update_mode_changed(data);
	    break;

	case DBOX_EVENT_REQUEST_CLOSE_GBAR:
	    dynamicbox_event_request_close_gbar(data);
	    break;

	case DBOX_EVENT_IGNORED:
	    dynamicbox_event_ignored(data);
	    break;
	default:
	    break;
    }

    return 0;
}

static int dynamicbox_fault_handler(enum dynamicbox_fault_type fault, const char *pkgname, const char *filename, const char *funcname, void *cbdata)
{
    Eina_List *l = NULL;
    Evas_Object *dynamicbox;
    struct widget_data *data;
    struct dynamicbox_evas_event_info info;

    switch (fault) {
	case DBOX_FAULT_DEACTIVATED:
	    EINA_LIST_FOREACH(s_info.list, l, dynamicbox) {
		data = get_smart_data(dynamicbox);
		if (!data) {
		    continue;
		}

		if (!strcmp(data->dbox_id, pkgname)) {
		    DbgPrint("Faulted: %s (%p)\n", pkgname, data);
		    data->is.field.faulted = 1;
		    dbox_overlay_faulted(data);
		    info.error = DBOX_STATUS_ERROR_FAULT;
		    info.pkgname = data->dbox_id;
		    info.event = DBOX_FAULT_DEACTIVATED;
		    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_DBOX_FAULTED, &info);
		}
	    }
	    break;
	case DBOX_FAULT_PROVIDER_DISCONNECTED:
	    EINA_LIST_FOREACH(s_info.list, l, dynamicbox) {
		data = get_smart_data(dynamicbox);
		if (!data) {
		    continue;
		}

		if (!strcmp(data->dbox_id, pkgname)) {
		    DbgPrint("Disconnected: %s (%p)\n", pkgname, data);
		    data->is.field.faulted = 1;
		    dbox_overlay_faulted(data);
		    info.error = DBOX_STATUS_ERROR_FAULT;
		    info.pkgname = data->dbox_id;
		    info.event = DBOX_FAULT_PROVIDER_DISCONNECTED;
		    smart_callback_call(data, DYNAMICBOX_SMART_SIGNAL_PROVIDER_DISCONNECTED, &info);
		}
	    }
	    break;
	default:
	    break;
    }
    return 0;
}

EAPI int evas_object_dynamicbox_init(Evas_Object *win, int force_to_buffer)
{
    int ret;

    ecore_x_window_size_get(0, &s_info.screen_width, &s_info.screen_height);

    s_info.conf.field.render_animator = 0;	// By default, use render animator for updating

    ret = dynamicbox_init(ecore_x_display_get(), 1, 0.001f, 1);
    if (ret < 0) {
	return ret;
    }

    ret = dynamicbox_add_event_handler(dynamicbox_event_handler, NULL);
    if (ret != DBOX_STATUS_ERROR_NONE) {
	ErrPrint("Failed to set handler\n");
	dynamicbox_fini();
    } else {
	DbgPrint("Event handler registered\n");
	ret = dynamicbox_add_fault_handler(dynamicbox_fault_handler, NULL);
	if (ret != DBOX_STATUS_ERROR_NONE) {
	    ErrPrint("Failed to set fault handler\n");
	    dynamicbox_remove_event_handler(dynamicbox_event_handler);
	    dynamicbox_fini();
	} else {
	    DbgPrint("Fault handler is registered\n");
	}
    }

    s_info.conf.field.force_to_buffer = force_to_buffer;
    s_info.win = win;

    return ret;
}

EAPI int evas_object_dynamicbox_fini(void)
{
    dynamicbox_remove_event_handler(dynamicbox_event_handler);
    dynamicbox_remove_fault_handler(dynamicbox_fault_handler);
    dynamicbox_fini();
    return 0;
}

EAPI int evas_object_dynamicbox_resumed(void)
{
    return dynamicbox_viewer_set_resumed();
}

EAPI int evas_object_dynamicbox_paused(void)
{
    return dynamicbox_viewer_set_paused();
}

EAPI Evas_Object *evas_object_dynamicbox_add(Evas_Object *parent, const char *dbox_id, const char *content_info, const char *cluster, const char *category, double period)
{
    struct widget_data *data;
    Evas_Object *dynamicbox;
    char *_dbox_id;
    char *_content_info;
    char *_cluster;
    char *_category;

    if (!parent || !dbox_id) {
	return NULL;
    }

    if (!cluster) {
	cluster = DEFAULT_CLUSTER;
    }

    if (!category) {
	category = DEFAULT_CATEGORY;
    }

    _cluster = strdup(cluster);
    if (!_cluster) {
	ErrPrint("Heap: %s\n", strerror(errno));
	return NULL;
    }

    _category = strdup(category);
    if (!_category) {
	ErrPrint("Heap: %s\n", strerror(errno));
	free(_cluster);
	return NULL;
    }

    _dbox_id = strdup(dbox_id);
    if (!_dbox_id) {
	ErrPrint("Heap: %s\n", strerror(errno));
	free(_category);
	free(_cluster);
	return NULL;
    }

    if (content_info) {
	_content_info = strdup(content_info);
	if (!_content_info) {
	    ErrPrint("Heap: %s\n", strerror(errno));
	    free(_dbox_id);
	    free(_category);
	    free(_cluster);
	    return NULL;
	}
    } else {
	_content_info = NULL;
    }

    if (!s_info.smart) {
	s_info.sc.add = widget_add;
	s_info.sc.del = widget_del;
	s_info.sc.move = widget_move;
	s_info.sc.resize = widget_resize;
	s_info.sc.show = widget_show;
	s_info.sc.hide = widget_hide;
	s_info.sc.color_set = widget_color_set;
	s_info.sc.clip_set = widget_clip_set;
	s_info.sc.clip_unset = widget_clip_unset;

	s_info.smart = evas_smart_class_new(&s_info.sc);
    }

    dynamicbox = evas_object_smart_add(evas_object_evas_get(parent), s_info.smart);

    data = evas_object_smart_data_get(dynamicbox);
    if (data) {
	data->parent = parent;
	data->dbox_id = _dbox_id;
	data->content = _content_info;
	data->cluster = _cluster;
	data->category = _category;
	data->is.field.mouse_event = 0;
	data->period = period;

	widget_data_setup(data);
    } else {
	ErrPrint("Failed to get smart data\n");
	free(_dbox_id);
	free(_content_info);
	free(_cluster);
	free(_category);
    }

    return dynamicbox;
}

EAPI int evas_object_dynamicbox_view_port_set(Evas_Object *dynamicbox, int x, int y, int w, int h)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    data->view_port.x = x;
    data->view_port.y = y;
    data->view_port.w = w;
    data->view_port.h = h;
    s_info.conf.field.user_view_port = 1;
    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_view_port_get(Evas_Object *dynamicbox, int *x, int *y, int *w, int *h)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    if (x) {
	*x = data->view_port.x;
    }

    if (y) {
	*y = data->view_port.y;
    }

    if (w) {
	*w = data->view_port.w;
    }

    if (h) {
	*h = data->view_port.h;
    }

    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_conf_set(enum dynamicbox_evas_conf type, int value)
{
    switch (type) {
    case DYNAMICBOX_EVAS_SENSITIVE_MOVE:
	s_info.conf.field.sensitive_move = value;
	break;
    case DYNAMICBOX_EVAS_EVENT_AUTO_FEED:
	s_info.conf.field.auto_feed = value;
	break;
    case DYNAMICBOX_EVAS_EASY_MODE:
	s_info.conf.field.easy_mode = value;
	break;
    case DYNAMICBOX_EVAS_USE_FIXED_SIZE:
	s_info.conf.field.use_fixed_size = value;
	break;
    case DYNAMICBOX_EVAS_MANUAL_PAUSE_RESUME:
	s_info.conf.field.manual_pause_resume = value;
	break;
    case DYNAMICBOX_EVAS_SHARED_CONTENT:
	(void)dynamicbox_set_option(DBOX_OPTION_SHARED_CONTENT, value);
	break;
    case DYNAMICBOX_EVAS_SUPPORT_GBAR:
	s_info.conf.field.support_gbar = value;
	break;
    case DYNAMICBOX_EVAS_SCROLL_X:
	s_info.conf.field.is_scroll_x = value;
	break;
    case DYNAMICBOX_EVAS_SCROLL_Y:
	s_info.conf.field.is_scroll_y = value;
	break;
    case DYNAMICBOX_EVAS_DELAYED_PAUSE_RESUME:
	s_info.conf.field.delayed_pause_resume = value;
	break;
    case DYNAMICBOX_EVAS_AUTO_RENDER_SELECTION:
	s_info.conf.field.auto_render_selector = value;
	break;
    case DYNAMICBOX_EVAS_DIRECT_UPDATE:
	(void)dynamicbox_set_option(DBOX_OPTION_DIRECT_UPDATE, !!value);
	break;
    case DYNAMICBOX_EVAS_USE_RENDER_ANIMATOR:
	if (s_info.conf.field.auto_render_selector) {
	    DbgPrint("Auto selector enabled, this render_animator option will be changed automatically\n");
	}

	s_info.conf.field.render_animator = !!value;
	DbgPrint("Turn %s render animator\n", s_info.conf.field.render_animator ? "on" : "off");
	break;
    default:
	break;
    }

    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_pause(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    return dynamicbox_set_visibility(data->handle, DBOX_HIDE_WITH_PAUSE);
}

EAPI int evas_object_dynamicbox_resume(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    return dynamicbox_set_visibility(data->handle, DBOX_SHOW);
}

EAPI int evas_object_dynamicbox_destroy_gbar(Evas_Object *dynamicbox)
{
    struct widget_data *data;
    int ret;

    data = get_smart_data(dynamicbox);
    if (!data || data->state != WIDGET_DATA_CREATED || !data->is.field.created || !data->handle || !data->is.field.gbar_created) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    ret = dynamicbox_destroy_glance_bar(data->handle, dbox_destroy_gbar_cb, widget_ref(data));
    if (ret < 0) {
	widget_unref(data);
    }

    return ret;
}

EAPI const char *evas_object_dynamicbox_content(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return NULL;
    }

    return dynamicbox_content(data->handle);
}

EAPI const char *evas_object_dynamicbox_title(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return NULL;
    }

    return dynamicbox_title(data->handle);
}

EAPI const char *evas_object_dynamicbox_dbox_id(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || data->state != WIDGET_DATA_CREATED) {
	return NULL;
    }

    return data->dbox_id;
}

EAPI double evas_object_dynamicbox_period(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return 0.0f;
    }

    return data->period;
}

EAPI void evas_object_dynamicbox_cancel_click(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return;
    }

    if (data->is.field.cancel_click == CANCEL_DISABLED) {
	data->is.field.cancel_click = CANCEL_USER;
    }
}

static void access_ret_cb(struct dynamicbox *handle, int ret, void *data)
{
    struct access_ret_cb_data *cb_data = data;

    switch (ret) {
	case DBOX_ACCESS_STATUS_ERROR:
	    ret = DYNAMICBOX_ACCESS_ERROR;
	    break;
	case DBOX_ACCESS_STATUS_DONE:
	    ret = DYNAMICBOX_ACCESS_DONE;
	    break;
	case DBOX_ACCESS_STATUS_FIRST:
	    ret = DYNAMICBOX_ACCESS_FIRST;
	    break;
	case DBOX_ACCESS_STATUS_LAST:
	    ret = DYNAMICBOX_ACCESS_LAST;
	    break;
	case DBOX_ACCESS_STATUS_READ:
	    ret = DYNAMICBOX_ACCESS_READ;
	    break;
	default:
	    ret = DYNAMICBOX_ACCESS_UNKNOWN;
	    break;
    }

    if (cb_data->ret_cb) {
	cb_data->ret_cb(cb_data->obj, ret, cb_data->data);
    }

    free(cb_data);
}

EAPI int evas_object_dynamicbox_force_mouse_up(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    return do_force_mouse_up(data);
}

EAPI int evas_object_dynamicbox_access_action(Evas_Object *dynamicbox, int type, void *_info, void (*ret_cb)(Evas_Object *obj, int ret, void *data), void *cbdata)
{
    struct widget_data *data;
    Elm_Access_Action_Info *info = _info;
    int w;
    int h;
    struct access_ret_cb_data *cb_data;
    int ret;
    struct dynamicbox_access_event_info ainfo;

    data = get_smart_data(dynamicbox);
    if (!data || !data->is.field.created || !data->handle) {
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    evas_object_geometry_get(data->dbox_layout, NULL, NULL, &w, &h);
    ainfo.x = (double)info->x / (double)w;
    ainfo.y = (double)info->y / (double)h;
    ainfo.info = 0;

    switch (type) {
	case ELM_ACCESS_ACTION_HIGHLIGHT: /* highlight an object */
	    DbgPrint("Highlight %dx%d ignored\n", info->x, info->y);
	    break;
	case ELM_ACCESS_ACTION_READ:
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_HIGHLIGHT;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_HIGHLIGHT, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_UNHIGHLIGHT: /* unhighlight an object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_UNHIGHLIGHT;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_HIGHLIGHT, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT: /* set highlight to next object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_HIGHLIGHT_NEXT;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_HIGHLIGHT, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_HIGHLIGHT_PREV: /* set highlight to previous object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_HIGHLIGHT_NEXT;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_HIGHLIGHT, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_ACTIVATE: /* activate a highlight object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_NONE; /* meaningless */
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_ACTIVATE, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_SCROLL: /* scroll if one of highlight object parents * is scrollable */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    switch (info->mouse_type) {
		case 0:
		    ainfo.type = DBOX_ACCESS_TYPE_DOWN;
		    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_SCROLL, &ainfo, access_ret_cb, cb_data);
		    if (ret != DBOX_STATUS_ERROR_NONE) {
			free(cb_data);
		    }
		    break;
		case 1:
		    ainfo.type = DBOX_ACCESS_TYPE_MOVE;
		    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_SCROLL, &ainfo, access_ret_cb, cb_data);
		    if (ret != DBOX_STATUS_ERROR_NONE) {
			free(cb_data);
		    }
		    break;
		case 2:
		    ainfo.type = DBOX_ACCESS_TYPE_UP;
		    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_SCROLL, &ainfo, access_ret_cb, cb_data);
		    if (ret != DBOX_STATUS_ERROR_NONE) {
			free(cb_data);
		    }
		    break;
		default:
		    ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
		    free(cb_data);
		    break;
	    }
	    break;
	case ELM_ACCESS_ACTION_MOUSE: /* give mouse event to highlight object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;
	    ainfo.type = DBOX_ACCESS_TYPE_NONE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_MOUSE, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_UP: /* change value up of highlight object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;
	    ainfo.type = DBOX_ACCESS_TYPE_UP;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_ACTION, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_DOWN: /* change value down of highlight object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_DOWN;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_ACTION, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_VALUE_CHANGE: /* TODO: deprecate this */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_NONE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_VALUE_CHANGE, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_BACK: /* go back to a previous view ex: pop naviframe item */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_NONE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_BACK, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_OVER: /* mouse over an object */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_NONE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_OVER, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_ENABLE: /* enable highlight and read ability */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_ENABLE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_ENABLE, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	case ELM_ACCESS_ACTION_DISABLE: /* disable highlight and read ability */
	    cb_data = calloc(1, sizeof(*cb_data));
	    if (!cb_data) {
		ErrPrint("Heap: %s\n", strerror(errno));
		return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
	    }

	    cb_data->ret_cb = ret_cb;
	    cb_data->data = cbdata;
	    cb_data->obj = dynamicbox;

	    ainfo.type = DBOX_ACCESS_TYPE_DISABLE;
	    ret = dynamicbox_feed_access_event(data->handle, DBOX_ACCESS_ENABLE, &ainfo, access_ret_cb, cb_data);
	    if (ret != DBOX_STATUS_ERROR_NONE) {
		free(cb_data);
	    }
	    break;
	default:
	    ret = DBOX_STATUS_ERROR_INVALID_PARAMETER;
	    break;
    }

    return ret;
}

EAPI void evas_object_dynamicbox_disable_preview(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    data->is.field.disable_preview = 1;
}

EAPI void evas_object_dynamicbox_disable_overlay_text(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    data->is.field.disable_text = 1;
}

EAPI void evas_object_dynamicbox_disable_loading(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    data->is.field.disable_loading = 1;
}

EAPI void evas_object_dynamicbox_activate(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    if (data->is.field.faulted) {
	elm_object_signal_emit(data->dbox_layout, "mouse,clicked,1", "overlay,content");
    } else {
	DbgPrint("Dynamicbox is not faulted\n");
    }
}

EAPI int evas_object_dynamicbox_is_faulted(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return 0;
    }

    return data->is.field.faulted;
}

EAPI int evas_object_dynamicbox_set_raw_event_callback(enum dynamicbox_evas_raw_event_type type, void (*cb)(struct dynamicbox_evas_raw_event_info *info, void *data), void *data)
{
    struct raw_event_cbdata *cbdata;

    cbdata = calloc(1, sizeof(*cbdata));
    if (!cbdata) {
	ErrPrint("calloc: %s\n", strerror(errno));
	return DBOX_STATUS_ERROR_OUT_OF_MEMORY;
    }

    cbdata->cb = cb;
    cbdata->data = data;

    switch (type) {
	case DYNAMICBOX_EVAS_RAW_DELETE:
	    s_info.raw_event.delete_list = eina_list_append(s_info.raw_event.delete_list, cbdata);
	    break;
	case DYNAMICBOX_EVAS_RAW_CREATE:
	    s_info.raw_event.create_list = eina_list_append(s_info.raw_event.create_list, cbdata);
	    break;
	default:
	    free(cbdata);
	    return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_unset_raw_event_callback(enum dynamicbox_evas_raw_event_type type, void (*cb)(struct dynamicbox_evas_raw_event_info *info, void *data), void *data)
{
    Eina_List *l;
    Eina_List *n;
    struct raw_event_cbdata *cbdata;

    switch (type) {
	case DYNAMICBOX_EVAS_RAW_DELETE:
	    EINA_LIST_FOREACH_SAFE(s_info.raw_event.delete_list, l, n, cbdata) {
		if (cbdata->cb == cb && cbdata->data == data) {
		    s_info.raw_event.delete_list = eina_list_remove(s_info.raw_event.delete_list, cbdata);
		    break;
		}
	    }
	    break;
	case DYNAMICBOX_EVAS_RAW_CREATE:
	    EINA_LIST_FOREACH_SAFE(s_info.raw_event.create_list, l, n, cbdata) {
		if (cbdata->cb == cb && cbdata->data == data) {
		    s_info.raw_event.create_list = eina_list_remove(s_info.raw_event.create_list, cbdata);
		    break;
		}
	    }
	    break;
	default:
	    break;
    }

    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_freeze_visibility(Evas_Object *dynamicbox, int status)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    data->is.field.freeze_visibility = 1;
    data->freezed_visibility = status;
    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_thaw_visibility(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    data->is.field.freeze_visibility = 0;

    return DBOX_STATUS_ERROR_NONE;
}

EAPI int evas_object_dynamicbox_visibility_is_freezed(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return 0;
    }

    return data->is.field.freeze_visibility;
}

EAPI int evas_object_dynamicbox_dump(Evas_Object *dynamicbox, const char *filename)
{
    struct widget_data *data;
    FILE *fp;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return DBOX_STATUS_ERROR_INVALID_PARAMETER;
    }

    fp = fopen(filename, "w+");
    if (fp) {
	Evas_Object *image;
	image = elm_object_part_content_get(data->dbox_layout, "dynamicbox,content");
	if (image) {
	    void *data;
	    Evas_Coord w, h;
	    evas_object_geometry_get(image, NULL, NULL, &w, &h);

	    data = evas_object_image_data_get(image, 0);
	    if (data) {
		fwrite(data, w * h, sizeof(int), fp);
	    }
	}
	fclose(fp);
    }

    return DBOX_STATUS_ERROR_NONE;
}

int evas_object_dynamicbox_is_dynamicbox(Evas_Object *dynamicbox)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return 0;
    }

    return 1;
}

void evas_object_dynamicbox_use_render_animator(int flag)
{
}

void evas_object_dynamicbox_set_permanent_delete(Evas_Object *dynamicbox, int flag)
{
    struct widget_data *data;

    data = get_smart_data(dynamicbox);
    if (!data) {
	ErrPrint("Invalid object\n");
	return;
    }

    data->is.field.permanent_delete = !!flag;
}

/* End of a file */
