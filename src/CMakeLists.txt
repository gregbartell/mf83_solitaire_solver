# Add this directory as an include path, so source files can include files
# relative to this directory
set_property(DIRECTORY APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mf83_main
    main.x.cpp)

install(TARGETS mf83_main
    RUNTIME DESTINATION bin)

add_executable(mf83_tests
    Searcher.t.cpp
    State.t.cpp
    TranspoTable.t.cpp)
target_link_libraries(mf83_tests
    gtest
    gtest_main)
gtest_discover_tests(mf83_tests)
