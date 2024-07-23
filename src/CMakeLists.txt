cmake_minimum_required(VERSION 3.18)
INCLUDE (CheckIncludeFiles)
INCLUDE(FindPkgConfig)

PROJECT(reporter C)

SET(SOURCES
	log.c
	reporter.c
	ubus.c
	udp.c
)

ADD_DEFINITIONS(-Os -Wall -Werror --std=gnu99 -g3 -Wmissing-declarations)

FIND_LIBRARY(libjson NAMES json-c json)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

ADD_EXECUTABLE(reporter ${SOURCES})

TARGET_LINK_LIBRARIES(reporter ubox ubus blobmsg_json ${LIBS_EXTRA} ${libjson})

SET(CMAKE_INSTALL_PREFIX /usr)

INSTALL(TARGETS reporter
	RUNTIME DESTINATION bin
)
