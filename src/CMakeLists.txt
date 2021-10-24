set(SPHONE_SRC_FILES 	sphone.cpp
	utils/book-import.c
	utils/rtconf.c
	utils/dbus-marshalers.c 
	utils/sphone-log.c
	utils/sphone-modules.c 
	utils/sphone-conf.c 
	utils/datapipe.c 
	utils/datapipes.c
	utils/sphone-store-tree-model.c 
	utils/store.c 
	utils/utils.c
	utils/types.c
	utils/comm.c
	gui/gui-calls-manager.c
	gui/gui-contact-view.c
	gui/gui-dialer.c
	gui/gui-options.c
	gui/gui-sms.c
	gui/keypad.c
	gui/gui-history-calls.c
	)

add_executable(sphone ${SPHONE_SRC_FILES})
target_link_libraries(sphone ${COMMON_LIBRARIES} Qt${QT_VERSION_MAJOR}::Widgets)
target_include_directories(sphone SYSTEM PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(sphone PRIVATE . utils gui modapi)
install(TARGETS sphone DESTINATION bin)

