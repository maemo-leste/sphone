find_package(PkgConfig)

pkg_check_modules(QTVOICECALL qtvoicecall)

if(DEFINED QTVOICECALL_FOUND)
	add_library(comm-voicecallmanager SHARED
		comm-voicecallmanager.cpp
		comm-voicecallmanager.h
		comm-voicecallmanager-maemomanager.h
		comm-voicecallmanager-maemoprovider.h
		comm-voicecallmanager-maemomanager.cpp
		comm-voicecallmanager-maemoprovider.cpp
		comm-voicecallmanager-maemocallhandler.h
		comm-voicecallmanager-maemocallhandler.cpp
		)
	target_link_libraries(comm-voicecallmanager ${COMMON_LIBRARIES} Qt${QT_VERSION_MAJOR}::Widgets ${TELEPATHY_QT5_LIBRARIES})

	target_link_libraries(comm-voicecallmanager ${QTVOICECALL_LIBRARIES})
	target_include_directories(comm-voicecallmanager PUBLIC ${QTVOICECALL_INCLUDE_DIRS})
	target_compile_options(comm-voicecallmanager PUBLIC ${QTVOICECALL_CFLAGS_OTHER})

	target_compile_definitions(comm-voicecallmanager PUBLIC ${Qt5DBus_DEFINITIONS})

	target_include_directories(comm-voicecallmanager SYSTEM PRIVATE ${COMMON_INCLUDE_DIRS})
	target_include_directories(comm-voicecallmanager PRIVATE ${MODULE_INCLUDE_DIRS})
	target_include_directories(comm-voicecallmanager PRIVATE ${TELEPATHY_QT5_INCLUDE_DIR})
	target_include_directories(comm-voicecallmanager PRIVATE ${Qt5DBus_INCLUDE_DIRS})

	set_property(TARGET comm-voicecallmanager PROPERTY AUTOMOC ON)

	install(TARGETS comm-voicecallmanager DESTINATION ${SPHONE_MODULE_DIR})
endif(DEFINED QTVOICECALL_FOUND)
