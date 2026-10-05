cmake_minimum_required(VERSION 3.14)
project(my_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(TESTS ${CMAKE_CURRENT_SOURCE_DIR}/tests)

include(FetchContent)


FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
)

FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.0
)

FetchContent_Declare(
    libtins
    GIT_REPOSITORY https://github.com/mfontanini/libtins.git
    GIT_TAG        v4.5  
)


FetchContent_MakeAvailable(json catch2 libtins)

add_executable(server_app ${SRC}/applications/server.cpp)
target_link_libraries(server_app PRIVATE Catch2::Catch2WithMain)
target_link_libraries(server_app PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(server_app PRIVATE ${libtins_SOURCE_DIR}/include)
target_link_libraries(server_app PRIVATE tins)
target_include_directories(server_app PRIVATE ${SRC}/sys_classes)
target_include_directories(server_app PRIVATE ${SRC}/net_classes)

add_executable(client_app ${SRC}/applications/client.cpp)
target_link_libraries(client_app PRIVATE Catch2::Catch2WithMain)
target_link_libraries(client_app PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(client_app PRIVATE ${libtins_SOURCE_DIR}/include)
target_link_libraries(client_app PRIVATE tins)
target_include_directories(client_app PRIVATE ${SRC}/sys_classes)
target_include_directories(client_app PRIVATE ${SRC}/net_classes)


add_executable(traffic_gen ${SRC}/applications/traffic_gen.cpp)
target_link_libraries(traffic_gen PRIVATE Catch2::Catch2WithMain)
target_link_libraries(traffic_gen PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(traffic_gen PRIVATE ${libtins_SOURCE_DIR}/include)
target_link_libraries(traffic_gen PRIVATE tins)
target_include_directories(traffic_gen PRIVATE ${SRC}/sys_classes)
target_include_directories(traffic_gen PRIVATE ${SRC}/net_classes)

add_executable(admin_app ${SRC}/applications/admin.cpp)
target_link_libraries(admin_app PRIVATE Catch2::Catch2WithMain)
target_link_libraries(admin_app PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(admin_app PRIVATE ${SRC}/sys_classes)

add_executable(sockets_test ${TESTS}/sockets_test.cpp)
target_link_libraries(sockets_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(sockets_test PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(sockets_test PRIVATE ${SRC}/sys_classes)


add_executable(fifo_test ${TESTS}/fifo_test.cpp)
target_link_libraries(fifo_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(fifo_test PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(fifo_test PRIVATE ${SRC}/sys_classes)


add_executable(shared_test ${TESTS}/shared_test.cpp)
target_link_libraries(shared_test PRIVATE Catch2::Catch2WithMain)
target_link_libraries(shared_test PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(shared_test PRIVATE rt pthread)
target_include_directories(shared_test PRIVATE ${SRC}/sys_classes)


