set(SPHONE_SRC_FILES 	sphone.c 
	utils/rtconf.c
	utils/sphone-log.c
	utils/sphone-modules.c 
	utils/sphone-conf.c 
	utils/datapipe.c 
	utils/datapipes.c
	utils/types.c
	utils/comm.c
	utils/gui.c
	utils/storage.c
	)

add_executable(sphone ${SPHONE_SRC_FILES})
set_property(TARGET sphone PROPERTY ENABLE_EXPORTS 1)
target_link_libraries(sphone ${COMMON_LIBRARIES})
target_include_directories(sphone SYSTEM PRIVATE ${COMMON_INCLUDE_DIRS})
target_include_directories(sphone PRIVATE . utils modapi)
install(TARGETS sphone DESTINATION bin)

