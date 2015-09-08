PROJECT(w-home C)

IF("${ARCH}" STREQUAL "arm")
	OPTION(RUN_ON_DEVICE "Run on device" ON)
	ADD_DEFINITIONS("-DRUN_ON_DEVICE")
ELSE()
	OPTION(RUN_ON_EMULATOR "Run on emulator" ON)
	ADD_DEFINITIONS("-DRUN_ON_EMULATOR")
ENDIF()
MESSAGE("ARCH: ${ARCH}")

INCLUDE(FindPkgConfig)
pkg_check_modules(PKGS REQUIRED
	ail
	appcore-efl
	aul
	badge
	bundle
	capi-appfw-application
	capi-base-utils-i18n
	capi-media-player
	capi-media-sound-manager
	capi-appfw-preference
	capi-system-runtime-info
	capi-system-device
	deviced
	dlog
	ecore
	ecore-x
	ecore-evas
	ecore-file
	ecore-imf
	ecore-input
	edje
	eet
	efl-assist
	efl-extension
	elementary
	eina
	evas
	feedback
	libxml-2.0
	widget_viewer_evas
	widget_service
	minicontrol-viewer
	minicontrol-monitor
	pkgmgr
	pkgmgr-info
	syspopup-caller
	rua
	utilX
	vconf
	appsvc
	capi-system-system-settings
	capi-media-image-util
	notification
	capi-message-port
)

IF(RUN_ON_DEVICE)
pkg_check_modules(PKGS_ARM REQUIRED
	dbus-1
	dbus-glib-1
	glib-2.0
	json
	journal
	tapi
)
ENDIF()

IF("${CIRCLE}" STREQUAL "circle")
	ADD_DEFINITIONS("-DCIRCLE_TYPE=1")
	SET(CIRCLE_TYPE "1")
ELSE()
	ADD_DEFINITIONS("-DCIRCLE_TYPE=0")
	SET(CIRCLE_TYPE "0")
ENDIF()

MESSAGE("circle_type : ${CIRCLE}")

SET(PACKAGE_NAME "org.tizen.${PROJECT_NAME}")
SET(PREFIX "/usr/apps/${PACKAGE_NAME}")
SET(BINDIR "${PREFIX}/bin")
SET(RESDIR "${PREFIX}/res")
SET(DATADIR "/opt${PREFIX}/data")
SET(LOCALEDIR "${RESDIR}/locale")
SET(IMAGEDIR "${RESDIR}/images")
SET(EDJEDIR "${RESDIR}/edje/${CIRCLE}")
SET(ICONDIR "/usr/share/icons/default/small")

REMOVE_DEFINITIONS("-DLOG_TAG")
ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DPROJECT=\"${PROJECT_NAME}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PACKAGE_NAME}\"")
ADD_DEFINITIONS("-DVERSION=\"${VERSION}\"")
ADD_DEFINITIONS("-DDATADIR=\"${DATADIR}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DIMAGEDIR=\"${IMAGEDIR}\"")
ADD_DEFINITIONS("-DEDJEDIR=\"${EDJEDIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DWIDGET_EVAS_RESOURCE_EDJ=\"${EDJEDIR}/widget.edj\"")
ADD_DEFINITIONS("-DWIDGET_EVAS_RESOURCE_PD=\"gbar\"")
ADD_DEFINITIONS("-DWIDGET_EVAS_RESOURCE_LB=\"widget\"")
ADD_DEFINITIONS("-DWIDGET_EVAS_RESOURCE_IMG=\"widget,image\"")
ADD_DEFINITIONS("-DENABLE_INDICATOR_BRIEFING_VIEW")

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

FOREACH(flag ${PKGS_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
FOREACH(flag ${PKGS_ARM_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(PRIVATE_C_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/include")
SET(CMAKE_C_FLAGS " ${PRIVATE_C_FLAGS} ${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE ${ADD_VIEWER_CFLAGS}")
SET(CMAKE_C_FLAGS_PROFILING    "-O0 -g -pg -fPIE")
SET(CMAKE_CXX_FLAGS_PROFILING  "-O0 -std=c++0x -g -pg -fPIE")
SET(CMAKE_C_FLAGS_DEBUG        "-O0 -g -fPIE")
SET(CMAKE_CXX_FLAGS_DEBUG      "-O0 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE      "-O2 -g -fPIE")
SET(CMAKE_CXX_FLAGS_RELEASE    "-O2 -std=c++0x -g -fPIE")
SET(CMAKE_C_FLAGS_CCOV         "-O2 -g --coverage -fPIE")
SET(CMAKE_CXX_FLAGS_CCOV       "-O2 -std=c++0x -g --coverage -fPIE")

SET(EXTRA_LDFLAGS "-ldl")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${PREFIX}/lib -pie")

FIND_PROGRAM(UNAME NAMES uname)
EXEC_PROGRAM("${UNAME}" ARGS "-m" OUTPUT_VARIABLE "ARCH")

#INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

ADD_EXECUTABLE(${PROJECT_NAME}
	src/bg.c
	src/clock_shortcut.c
	src/clock_service.c
	src/clock_event.c
	src/clock_manager.c
	src/clock_inf_widget.c
	src/clock_inf_minictrl.c
	src/clock_manager.c
	src/clock_util.c
	src/clock_view.c
	src/clock_view_cue.c
	src/clock_view_hidden.c
	src/clock_view_indicator.c
	src/db.c
	src/dbus.c
	src/dbus_util.c
	src/edit.c
	src/effect.c
	src/power_mode.c
	src/gesture.c
	src/index.c
	src/item_info.c
	src/key.c
	src/lang.c
	src/layout.c
	src/widget.c
	src/main.c
	src/mapbuf.c
	src/noti_broker.c
	src/minictrl.c
	src/minictrl_util.c
	src/minictrl_manager.c
	src/page.c
	src/page_info.c
	src/popup.c
	src/scroller.c
	src/tutorial.c
	src/util.c
	src/virtual_canvas.c
	src/win.c
	src/xml.c
	src/pkgmgr.c
	src/add-viewer.c
	src/add-viewer_pkgmgr.c
	src/add-viewer_ucol.c
	src/add-viewer_util.c
	src/add-viewer_package.c
	src/wms.c
	src/apps/bg.c
	src/apps/db.c
	src/apps/effect.c
	src/apps/item.c
	src/apps/item_badge.c
	src/apps/item_info.c
	src/apps/lang.c
	src/apps/layout.c
	src/apps/list.c
	src/apps/apps_main.c
	src/apps/page.c
	src/apps/pkgmgr.c
	src/apps/scroller.c
	src/apps/xml.c
	src/critical_log.c
	src/notification/detail.c
	src/notification/notification.c
	src/notification/simple.c
	src/notification/summary.c
	src/notification/time.c
)

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_LDFLAGS} ${EXTRA_LDFLAGS} -lm  -Wl,--hash-style=both -Wl,--as-needed -pie )
IF(RUN_ON_DEVICE)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PKGS_ARM_LDFLAGS} -lm)
ENDIF()

# Install
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/LICENSE.flora DESTINATION /usr/share/license RENAME ${PACKAGE_NAME})

ADD_SUBDIRECTORY(data)
ADD_SUBDIRECTORY(res)

# End of a file
