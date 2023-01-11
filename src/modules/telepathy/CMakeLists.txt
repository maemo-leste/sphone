find_package(PkgConfig)
set(MODULE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../modapi)

if(DEFINED Qt${QT_VERSION_MAJOR}_FOUND)
    pkg_check_modules(QTVOICECALL REQUIRED qtvoicecall)

	add_library(comm-telepathy SHARED
        comm-telepathy.cpp
        comm-telepathy-maemomanager.h
        comm-telepathy-maemoprovider.h
        comm-telepathy-maemomanager.cpp
        comm-telepathy-maemoprovider.cpp
        comm-telepathy-maemocallhandler.h
        comm-telepathy-maemocallhandler.cpp
        )
	target_link_libraries(comm-telepathy ${COMMON_LIBRARIES} Qt${QT_VERSION_MAJOR}::Widgets ${TELEPATHY_QT5_LIBRARIES})

    target_link_libraries(comm-telepathy ${QTVOICECALL_LIBRARIES})
    target_include_directories(comm-telepathy PUBLIC ${QTVOICECALL_INCLUDE_DIRS})
    target_compile_options(comm-telepathy PUBLIC ${QTVOICECALL_CFLAGS_OTHER})

	target_compile_definitions(comm-telepathy PUBLIC ${Qt5DBus_DEFINITIONS})
	target_include_directories(comm-telepathy SYSTEM PRIVATE ${COMMON_INCLUDE_DIRS})
	target_include_directories(comm-telepathy PRIVATE ${MODULE_INCLUDE_DIRS})
	target_include_directories(comm-telepathy PRIVATE ${TELEPATHY_QT5_INCLUDE_DIR})
	target_include_directories(comm-telepathy PRIVATE ${Qt5DBus_INCLUDE_DIRS})

	set_property(TARGET comm-telepathy PROPERTY AUTOMOC ON)

	install(TARGETS comm-telepathy DESTINATION ${SPHONE_MODULE_DIR})
endif(DEFINED Qt${QT_VERSION_MAJOR}_FOUND)
