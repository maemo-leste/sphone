set(SPHONE_SRC_FILES 	sphone.c 
	utils/book-import.c
	utils/rtconf.c
	utils/dbus-marshalers.c 
	utils/sphone-call.c
	utils/sphone-log.c 
	utils/sphone-manager.c 
	utils/sphone-modules.c 
	utils/sphone-conf.c 
	utils/datapipe.c 
	utils/datapipes.c
	utils/sphone-store-tree-model.c 
	utils/store.c 
	utils/utils.c
	comms/ofono.c
	gui/gui-calls-manager.c
	gui/gui-contact-view.c
	gui/gui-dialer.c
	gui/gui-options.c
	gui/gui-sms.c
	gui/keypad.c
	)

add_executable(sphone ${SPHONE_SRC_FILES})
target_link_libraries(sphone ${COMMON_LIBRARIES})
target_include_directories(sphone SYSTEM PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(sphone PRIVATE . utils gui comms modapi)
install(TARGETS sphone DESTINATION bin)
