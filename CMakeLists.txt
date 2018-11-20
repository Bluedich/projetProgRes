cmake_minimum_required(VERSION 2.8.3)

project(RE216_JALON02)

find_package (Threads)
#add here source files you need to compile and link to the client app
SET(SRC_CLIENT
	common.c
	client.c
	)


add_executable(RE216_JALON02_CLIENT ${SRC_CLIENT})
target_link_libraries(RE216_JALON02_CLIENT $(CMAKE_THREAD_LIBS_INIT))

#add here source files you need to compile and link to the server app
SET(SRC_SERVER
				common.c
        server.c)


add_executable(RE216_JALON02_SERVER  ${SRC_SERVER})
target_link_libraries(RE216_JALON02_SERVER $(CMAKE_THREAD_LIBS_INIT))
