cmake_minimum_required(VERSION 2.8.3)

project(RE216)

#add here source files you need to compile and link to the client app
SET(SRC_CLIENT
	common.c
	client.c
	file_trans.c
	)


add_executable(RE216_CLIENT ${SRC_CLIENT})


#add here source files you need to compile and link to the server app
SET(SRC_SERVER
				list.c
				group.c
				common.c
        server.c)


add_executable(RE216_SERVER  ${SRC_SERVER})
