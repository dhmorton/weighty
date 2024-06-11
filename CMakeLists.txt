cmake_minimum_required(VERSION 3.28.0)
project(weighty VERSION 0.1.0 LANGUAGES C)

file(GLOB SOURCES
	src/*.h
	src/*.c
)

#use pgkconfig to find gtk+ headers
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(weighty ${SOURCES})
target_link_libraries(weighty PRIVATE ${GTK3_LIBRARIES} m)
target_link_libraries(weighty PUBLIC -ltag_c -lsndfile -lvorbisfile -lmpg123 -lao -lmtp -lpthread -lbluetooth -lmp4v2 -lgobject-2.0 -lcurl -lm -lvorbisfile -logg -lvorbis -lFLAC -lid3tag -lgtk-x11-2.0 -lsqlite3 -lxine -lX11)

target_include_directories(weighty PRIVATE ${GTK3_INCLUDE_DIRS} /usr/include)
target_link_directories(weighty PRIVATE ${GTK3_LIBRARY_DIRS})
target_compile_options(weighty PRIVATE ${GTK3_CFLAGS_OTHER})
#target_link_libraries(-ltag_c -lsndfile -lvorbisfile -lmpg123 -lao -lmtp -lpthread -lbluetooth -lmp4v2 -lgobject-2.0 -lcurl -lm -lvorbisfile -logg -lvorbis -lFLAC -lid3tag -lgtk-x11-2.0 -lsqlite3 -lxine -lX11)