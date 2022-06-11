cmake_minimum_required(VERSION 3.1)

project(sphone LANGUAGES C CXX VERSION 0.6)

set(CMAKE_INSTALL_PREFIX "/usr")

set(SPHONE_SYSCONF_DIR /usr/share/sphone)
set(SPHONE_SYSCONF_OVR_DIR .sphone/)
set(SPHONE_MODULE_DIR /usr/lib/sphone/modules)

add_definitions(-D_GNU_SOURCE)
add_definitions(-DSPHONE_MODULE_DIR=${SPHONE_MODULE_DIR})
add_definitions(-DSPHONE_SYSCONF_DIR=${SPHONE_SYSCONF_DIR})
add_definitions(-DSPHONE_SYSCONF_OVR_DIR=${SPHONE_SYSCONF_OVR_DIR})
add_definitions(-DSPHONE_SYSCONF_INI=sphone.ini)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)
pkg_search_module(GTK REQUIRED gtk+-2.0)
pkg_search_module(GMODULE REQUIRED gmodule-2.0)
pkg_search_module(DBUS REQUIRED dbus-glib-1)
pkg_search_module(SQL REQUIRED sqlite3)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets)
pkg_search_module(GSTREAMER gstreamer-1.0)
pkg_search_module(PULSE libpulse)
pkg_search_module(LIBHILDON hildon-1)
pkg_search_module(LIBPROFILE profile)
pkg_search_module(EBOOK libebook-1.2)
pkg_search_module(EBOOKC libebook-contacts-1.2)
pkg_search_module(RTCOM rtcom-eventlogger)
pkg_search_module(ABOOK libosso-abook-1.0)

if(DEFINED LIBHILDON_LIBRARIES)
	set(SPHONE_DESKTOP_DIR /usr/share/applications/hildon)
else()
	set(SPHONE_DESKTOP_DIR /usr/share/applications/)
endif(DEFINED LIBHILDON_LIBRARIES)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")

add_compile_options( 
	-Wextra
	-Wall
	-Wno-switch-default
	-Wno-implicit-fallthrough
	-Wpointer-arith
	-Wundef
	-Wshadow
	-Wbad-function-cast
	-Wwrite-strings
	-Wsign-compare
	-Wmissing-noreturn
	-Wnested-externs
	-Wchar-subscripts
	-Wmissing-prototypes
	-Wformat-security
	-Wformat=2
	-Wformat-nonliteral
	-Winit-self
	-Wstrict-prototypes
	-Wold-style-definition
	-Wmissing-declarations
	-Wmissing-include-dirs
	-Wstrict-aliasing=2
	-Wunsafe-loop-optimizations
	-Winvalid-pch
	-Waddress
	-Wvolatile-register-var
	-Wstrict-overflow=5
	-Wunreachable-code
	-Wstack-protector )

set(COMMON_INCLUDE_DIRS 
	${GLIB_INCLUDE_DIRS} 
	${GIO_INCLUDE_DIRS} 
	${GMODULE_INCLUDE_DIRS} 
	${DBUS_INCLUDE_DIRS} 
	${GDBUS_INCLUDE_DIRS}
	${SQL_INCLUDE_DIRS}
	${GTK_INCLUDE_DIRS}
	${LIBHILDON_INCLUDE_DIRS})

set(COMMON_LIBRARIES 
	${GLIB_LIBRARIES} 
	${GIO_LIBRARIES} 
	${GMODULE_LIBRARIES} 
	${DBUS_LIBRARIES} 
	${GDBUS_LIBRARIES}
	${SQL_LIBRARIES}
	${GTK_LIBRARIES}
	${LIBHILDON_LIBRARIES})

if(DEFINED LIBHILDON_LIBRARIES)
	message("Hildon support enabled")
	add_definitions(-DENABLE_LIBHILDON=1 -DMAEMO_GTK=1)
else()
	message("Hildon support disabled")
endif(DEFINED LIBHILDON_LIBRARIES)

if(DEFINED LIBPROFILE_LIBRARIES)
	message("Profiled support enabled")
else()
	message("Profiled support disabled")
endif(DEFINED LIBPROFILE_LIBRARIES)

if(DEFINED GSTREAMER_LIBRARIES)
	message("GStreamer support enabled")
else()
	message("GStreamer support disabled")
endif(DEFINED GSTREAMER_LIBRARIES)

if(DEFINED PULSE_LIBRARIES)
	message("Pulseaudio support enabled")
else()
	message("Pulseaudio support disabled")
endif(DEFINED PULSE_LIBRARIES)

if(DEFINED EBOOK_LIBRARIES AND DEFINED EBOOKC_LIBRARIES)
	message("Evolution address book support enabled")
else()
	message("Evolution address book support disabled")
endif(DEFINED EBOOK_LIBRARIES AND DEFINED EBOOKC_LIBRARIES)

if(DEFINED RTCOM_LIBRARIES)
	message("rtcom-eventlogger support enabled")
else()
	message("rtcom-eventlogger support disabled")
endif(DEFINED RTCOM_LIBRARIES)

if(DEFINED Qt${QT_VERSION_MAJOR}_FOUND)
	message("QT support enabled")
else()
	message("QT support disabled")
endif(DEFINED Qt${QT_VERSION_MAJOR}_FOUND)

if(DEFINED ABOOK_LIBRARIES)
	message("osso-abook support enabled")
else()
	message("osso-abook support disabled")
endif(DEFINED ABOOK_LIBRARIES)

add_subdirectory(src)
add_subdirectory(src/modules)
add_subdirectory(desktop)
add_subdirectory(config)
