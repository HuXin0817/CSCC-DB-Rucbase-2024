include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(analyze)
add_subdirectory(record)
add_subdirectory(index)
add_subdirectory(system)
add_subdirectory(execution)
add_subdirectory(parser)
add_subdirectory(optimizer)
add_subdirectory(common)
add_subdirectory(transaction)
add_subdirectory(test)


target_link_libraries(parser execution pthread)

add_executable(rmdb rmdb_finals.cpp)
target_link_libraries(rmdb parser execution readline pthread planner analyze)
