add_executable(client
    client.cpp
    main.cpp
)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../usrsctp/usrsctplib
)

target_link_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../usrsctp/usrsctplib/.libs
)

target_link_libraries(client
    usrsctp
)
