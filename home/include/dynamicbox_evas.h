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

#ifndef __DYNAMICBOX_EVAS_H
#define __DYNAMICBOX_EVAS_H

#ifdef __cplusplus
extern "C" {
#endif

#define DYNAMICBOX_EVAS_DEFAULT_PERIOD                -1.0f                   /**< Default Update Period */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_CREATE_ABORTED   "dbox,create,aborted"   /**< Dynamicbox creation is aborted */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_CREATED          "dbox,created"          /**< Dynamicbox is created */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_RESIZE_ABORTED   "dbox,resize,aborted"   /**< Resizing dynamicbox is aborted */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_RESIZED          "dbox,resized"          /**< Dynamicbox is resized */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_FAULTED          "dbox,faulted"          /**< Dynamicbox has faulted */
#define DYNAMICBOX_SMART_SIGNAL_UPDATED               "updated"               /**< Dynamicbox content is updated */
#define DYNAMICBOX_SMART_SIGNAL_EXTRA_INFO_UPDATED    "info,updated"          /**< Dynamicbox extra info is updated */
#define DYNAMICBOX_SMART_SIGNAL_PROVIDER_DISCONNECTED "provider,disconnected" /**< Provider is disconnected */
#define DYNAMICBOX_SMART_SIGNAL_GBAR_DESTROYED        "gbar,destroyed"        /**< GBAR is destroyed */
#define DYNAMICBOX_SMART_SIGNAL_GBAR_ABORTED          "gbar,aborted"          /**< GBAR creation is aborted */
#define DYNAMICBOX_SMART_SIGNAL_GBAR_CREATED          "gbar,created"          /**< GBAR is created */
#define DYNAMICBOX_SMART_SIGNAL_FLICKDOWN_CANCELLED   "flickdown,cancelled"   /**< Flick down is canceld */
#define DYNAMICBOX_SMART_SIGNAL_CONTROL_SCROLLER      "control,scroller"      /**< Control Scroller */
#define DYNAMICBOX_SMART_SIGNAL_DBOX_DELETED          "dbox,deleted"          /**< DynamicBox is deleted */
#define DYNAMICBOX_SMART_SIGNAL_PERIOD_CHANGED        "dbox,period,changed"   /**< Period is changed */

/**
 * \brief
 * Data structure for Smart Callback Event
 */
struct dynamicbox_evas_event_info {
    const char *pkgname;    /**< Dynamicbox Application Id */
    int event;		    /**< Event type - DBOX_EVENT_XXX, refer the dynamicbox.h */
    int error;		    /**< Error type - DBOX_STATUS_XXX, refer the dynamicbox.h */
};

enum dynamicbox_evas_raw_event_type {
    DYNAMICBOX_EVAS_RAW_DELETE = 0x00,
    DYNAMICBOX_EVAS_RAW_CREATE = 0x02,
    DYNAMICBOX_EVAS_RAW_MAX = 0xff,
};

struct dynamicbox_evas_raw_event_info {
    const char *pkgname;
    enum dynamicbox_evas_raw_event_type type;
    int error;
    Evas_Object *dynamicbox;
};

/**
 * \brief
 * Configuration keys
 */
enum dynamicbox_evas_conf {
    DYNAMICBOX_EVAS_MANUAL_PAUSE_RESUME = 0x0001,   /**< Visibility will be changed manually */
    DYNAMICBOX_EVAS_SHARED_CONTENT = 0x0002,	    /**< Multiple instances will share the content of one real instance */
    DYNAMICBOX_EVAS_SUPPORT_GBAR = 0x0004,	    /**< GBAR will be used */
    DYNAMICBOX_EVAS_USE_FIXED_SIZE = 0x0008,	    /**< Dynamicbox will be resized to specific size only */
    DYNAMICBOX_EVAS_EASY_MODE = 0x0010,		    /**< Easy mode on/off */
    DYNAMICBOX_EVAS_SCROLL_X = 0x0020,		    /**< Box will be scrolled from left to right vice versa */
    DYNAMICBOX_EVAS_SCROLL_Y = 0x0040,		    /**< Box will be scrolled from top to bottom vice versa */
    DYNAMICBOX_EVAS_EVENT_AUTO_FEED = 0x0080,	    /**< Feeds event automatically from the master provider */
    DYNAMICBOX_EVAS_DELAYED_PAUSE_RESUME = 0x0100,  /**< Delaying the pause/resume when it is automatically changed */
    DYNAMICBOX_EVAS_SENSITIVE_MOVE = 0x0200,	    /**< Force feeds mouse up event if the box is moved */
    DYNAMICBOX_EVAS_AUTO_RENDER_SELECTION = 0x0400, /**< Select render automatically, if a box moved, do not sync using animator, or use the animator */
    DYNAMICBOX_EVAS_DIRECT_UPDATE = 0x0800,	    /**< Enable direct update path */
    DYNAMICBOX_EVAS_USE_RENDER_ANIMATOR = 0x1000,   /**< Use the render animator or not */
    DYNAMICBOX_EVAS_UNKNOWN = 0xFFFF
};

enum dynamicbox_access_result {
    DYNAMICBOX_ACCESS_DONE = 0x00,
    DYNAMICBOX_ACCESS_FIRST = 0x01,
    DYNAMICBOX_ACCESS_LAST = 0x02,
    DYNAMICBOX_ACCESS_READ = 0x04,
    DYNAMICBOX_ACCESS_ERROR = 0x80,
    DYNAMICBOX_ACCESS_UNKNOWN = 0xFF
};

/**
 * @brief Initialize the dynamicbox system
 * @param[in] win Window object
 * @param[in] force_to_buffer if you want use the naive buffer directly (instead of resource id), use 1 or 0.
 * @return int
 * @retval
 * @see evas_object_dynamicbox_fini()
 */
extern int evas_object_dynamicbox_init(Evas_Object *win, int force_to_buffer);

/**
 * @brief Finalize the dynamicbox system
 * @return int
 * @retval
 * @see evas_object_dynamicbox_init()
 */
extern int evas_object_dynamicbox_fini(void);

/**
 * @brief Create a new dynamicbox object
 * @param[in] parent
 * @param[in] dbox_id
 * @param[in] content_info
 * @param[in] cluster
 * @param[in] category
 * @param[in] period update period
 * @return Evas_Object*
 * @retval NULL if it fails to create a new dynamicbox object
 */
extern Evas_Object *evas_object_dynamicbox_add(Evas_Object *parent, const char *dbox_id, const char *content_info, const char *cluster, const char *category, double period);

/**
 * @brief Close the Glance Bar if it is opened
 * @param[in] dynamicbox Dynamicbox object
 * @return int
 */
extern int evas_object_dynamicbox_destroy_gbar(Evas_Object *dynamicbox);

/**
 * @brief if a viewer is resumed, use this function to notify it to the providers.
 * @details if you call this, all providers will gets resumed event.
 * @return int
 * @see evas_object_dynamicbox_paused()
 */
extern int evas_object_dynamicbox_resumed(void);

/**
 * @brief If a viewer is paused, use this function to notify it to the providers
 * @detail if you call this, all providers will gets paused event.
 * @return int
 */
extern int evas_object_dynamicbox_paused(void);

/**
 * @brief Change the state of each dynamicbox. if you want made a box should be paused, call this.
 * @param[in] dynamicbox Dynamicbox object
 * @return int
 */
extern int evas_object_dynamicbox_pause(Evas_Object *dynamicbox);

/**
 * @brief Change the state of each dynamicbox. If you want made a box should be resumed, call this.
 * @param[in] dynamicbox Dynamicbox object
 * @return int
 */
extern int evas_object_dynamicbox_resume(Evas_Object *dynamicbox);

/**
 * @brief Set the viewe port of given dynamicbox
 * @param[in] dynamicbox
 * @param[in] x
 * @param[in] y
 * @param[in] w
 * @param[in] h
 * @return int
 */
extern int evas_object_dynamicbox_view_port_set(Evas_Object *dynamicbox, int x, int y, int w, int h);

/**
 * @brief Get the current view port of given dynamicbox
 * @param[in] dynamicbox
 * @param[out] x
 * @param[out] y
 * @param[out] w
 * @param[out] h
 * @return int
 */
extern int evas_object_dynamicbox_view_port_get(Evas_Object *dynamicbox, int *x, int *y, int *w, int *h);

/**
 * @brief Change the configurable values of dynamicbox system
 * @param[in] type Configuration item
 * @param[in] value Its value
 * @return int
 */
extern int evas_object_dynamicbox_conf_set(enum dynamicbox_evas_conf type, int value);

/**
 * @brief Content string of dynamicbox
 * @details This string should be used for creating dynamicbox again after reboot device or recovered from crash(abnormal status)
 * @param[in] dynamicbox Dynamicbox Object
 * @return const char * String of content
 * @retval NULL if there is no specific content string.
 */
extern const char *evas_object_dynamicbox_content(Evas_Object *dynamicbox);

/**
 * @brief Summarized string of dynamicbox content.
 * @details If the accessibility feature is turned on, the homescreen can read this text to describe the dynamicbox.
 * @param[in] dynamicbox Dynamicbox Object
 * @return const char * Text should be read
 * @retval NULL if there is no summarized text for content of given dynamicbox
 */
extern const char *evas_object_dynamicbox_title(Evas_Object *dynamicbox);

/**
 * @brief Get the dynamicbox Id
 * @param[in] dynamicbox Dynamicbox Object
 * @return const char * Dynamic Box Id
 * @retval NULL if an error occurred
 */
extern const char *evas_object_dynamicbox_dbox_id(Evas_Object *dynamicbox);

/**
 * @brief Current period of updates
 * @param[in] dynamicbox Dynamicbox Object
 * @return double
 * @retval Update period
 */
extern double evas_object_dynamicbox_period(Evas_Object *dynamicbox);

/**
 * @brief Cancelate click event procedure.
 * @details If you call this after feed the mouse_down(or mouse_set) event, the box will get ON_HOLD events.\n
 *          If a box gets ON_HOLD event, it will not do anything even if you feed mouse_up(or mouse_unset) event.\n
 * @param[in] dynamicbox Dynamicbox Object
 * @return void
 */
extern void evas_object_dynamicbox_cancel_click(Evas_Object *dynamicbox);

/**
 * @brief This function should be called right after create the dynamicbox object. before resizing it.
 * @param[in] dynamicbox
 * @return void
 */
extern void evas_object_dynamicbox_disable_preview(Evas_Object *dynamicbox);

/**
 * @brief While loading a box, hide the help text
 * @param[in] dynamicbox
 * @return void
 */
extern void evas_object_dynamicbox_disable_overlay_text(Evas_Object *dynamicbox);

/**
 * @brief Do not display the overlay layer while loading a new box.
 * @details if you disable it, there is no preview & help text while creating a dynamicbox object
 * @return void
 */
extern void evas_object_dynamicbox_disable_loading(Evas_Object *dynamicbox);

/**
 * @brief Feeds the mouse_up event forcely.
 * @details This is very similiar with evas_object_dynamicbox_cancel_click(), but this will sends mouse_up event explicitly.\n
 *          Also feed the ON_HOLD event before feeds mouse_up event.
 * @param[in] dynamicbox Dynamic Box
 * @return int
 */
extern int evas_object_dynamicbox_force_mouse_up(Evas_Object *dynamicbox);

/**
 * @brief Feeds accessibility events
 * @param[in] dynamicbox
 * @param[in] type
 * @param[in] info
 * @param[in] ret_cb
 * @param[in] dta
 * @return int
 */
extern int evas_object_dynamicbox_access_action(Evas_Object *dynamicbox, int type, void *info, void (*ret_cb)(Evas_Object *obj, int ret, void *data), void *data);

/**
 * @brief Activate
 * @param[in] dynamicbox
 */
extern void evas_object_dynamicbox_activate(Evas_Object *dynamicbox);

/**
 * @brief
 * @param[in] dynamicbox
 */
extern int evas_object_dynamicbox_is_faulted(Evas_Object *dynamicbox);

/**
 * @brief
 * @param[in] type
 * @param[in] cb
 * @param[in] data
 * @return int
 */
extern int evas_object_dynamicbox_unset_raw_event_callback(enum dynamicbox_evas_raw_event_type type, void (*cb)(struct dynamicbox_evas_raw_event_info *info, void *data), void *data);

/**
 * @brief
 * @param[in] type
 * @param[in] cb
 * @param[in] data
 */
extern int evas_object_dynamicbox_set_raw_event_callback(enum dynamicbox_evas_raw_event_type type, void (*cb)(struct dynamicbox_evas_raw_event_info *info, void *data), void *data);

/**
 * @brief If you don't want change the visibility automatically, freeze it.\n
 *        The visibility will not be changed even though a box disappeared(hidden)/displayed(shown) from/on the screen.
 * @param[in] dynamicbox
 * @param[in] status
 * @return int
 */
extern int evas_object_dynamicbox_freeze_visibility(Evas_Object *dynamicbox, int status);

/**
 * @brief
 * @param[in] dynamicbox
 * @return int
 */
extern int evas_object_dynamicbox_thaw_visibility(Evas_Object *dynamicbox);

/**
 * @brief Get the state of visibility option.
 * @param[in] dynamicbox
 * @return int
 */
extern int evas_object_dynamicbox_visibility_is_freezed(Evas_Object *dynamicbox);

/**
 * @brief Dump a contents of dynamicbox to a given filename.
 * @param[in] dynamicbox Dynamicbox object
 * @param[in] filename Filename will be used for saving content of a dynamicbox
 * @return int
 */
extern int evas_object_dynamicbox_dump(Evas_Object *dynamicbox, const char *filename);

/**
 * @brief Validate the object, whether it is a dynamicbox object or not
 * @param[in] dynamicbox
 * @return int
 */
extern int evas_object_dynamicbox_is_dynamicbox(Evas_Object *dynamicbox);

/**
 * @brief Before delete a box, set the deletion mode using this.
 * @param[in] dynamicbox Dynamicbox Object which will be deleted soon
 * @param[in] flag 1 if you delete this dynamicbox instance permanently, of 0 if you want keep it and it will be re-created soon.
 * @return void
 */
extern void evas_object_dynamicbox_set_permanent_delete(Evas_Object *dynamicbox, int flag);

#ifdef __cplusplus
}
#endif

#endif

/* End of a file */
