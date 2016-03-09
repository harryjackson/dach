cmake_minimum_required(VERSION 3.0)
project(dach C)
message("CMAKE_CURRENT_SOURCE_DIR == ${CMAKE_CURRENT_SOURCE_DIR}")
#----------------------------------------------
################## dach      ##################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/dach/)
#set(dach_hdrs 
#  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_stream.h
#)
set(dach_srcs 
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_ar64.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_block.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_class.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_cmdline.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_cmdline.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_command.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_command.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_copy.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_error.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_gperf.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_header.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_io.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_lock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_mem.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_options.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_options.h
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_ppm0.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_stream.c
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_string.c
)
add_executable(dach_main
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/dach_main.c
  ${dach_srcs}
)
add_executable(dach_block_test
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/tests/dach_block_test.c
)
add_executable(dach_lock_test
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/tests/dach_lock_test.c
)
add_executable(dach_mem_test
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/tests/dach_mem_test.c
)
add_executable(dach_stream_test
  ${CMAKE_CURRENT_SOURCE_DIR}/dach/tests/dach_stream_test.c
)
add_test(t_d_lock_test   dach_lock_test)
add_test(t_d_block_test  dach_block_test)
add_test(t_d_mem_test    dach_mem_test)
add_test(t_d_stream_test dach_stream_test)

add_library(libdach SHARED
  ${dach_srcs}
)
target_link_libraries(libdach "-lapr-1 -laprutil-1")
target_link_libraries(dach_main        libdach) 
target_link_libraries(dach_block_test  libdach) 
target_link_libraries(dach_lock_test   libdach) 
target_link_libraries(dach_mem_test    libdach) 
target_link_libraries(dach_stream_test libdach) 

################## sqlite3  ######################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/)
set(sqlite3_srcs 
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.c
  ${CMAKE_CURRENT_SOURCE_DIR}/sqlite3/sqlite3.h
)
add_library(dachsql SHARED
  ${sqlite3_srcs}
)
#-------------------------------------------------
################## fdb ###########################
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fdb/)
set(fdb_srcs 
  ${CMAKE_CURRENT_SOURCE_DIR}/fdb/fdb.h
  ${CMAKE_CURRENT_SOURCE_DIR}/fdb/fdb_impl.c
)
add_executable(fdb
  ${CMAKE_CURRENT_SOURCE_DIR}/fdb/fdb.c
  ${fdb_srcs}
)
target_link_libraries(fdb "-lapr-1 -laprutil-1")
target_link_libraries(fdb  dachsql)   
