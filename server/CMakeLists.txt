add_executable(server
    server.cpp
    main.cpp
)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../usrsctp/usrsctplib
)

target_link_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../usrsctp/usrsctplib/.libs
)

target_link_libraries(server
    usrsctp
)
