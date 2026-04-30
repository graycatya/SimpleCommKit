

project(SimpleCommKitBle)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS  ON)
endif()

set(PRIVATE_SOURCES
    src/SimpleCommKitBleCentral.cpp
    src/SimpleCommKitBleCentral_p.cpp
)
set(PRIVATE_HEADER
    src/SimpleCommKitBleCentral_p.h
)


set(HEADERS
    SimpleCommKitBleCentral.h
)


add_library(${PROJECT_NAME}
    ${PRIVATE_HEADER}
    ${PRIVATE_SOURCES}
    ${HEADERS}
)
add_library(SimpleCommKit::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_SHARED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SIMPLECOMMKIT_EXPORTS)
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    simpleble::simpleble SimpleCommKitUtil
)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)


if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()

# # 安装规则
# install(TARGETS ${PROJECT_NAME}
#     LIBRARY DESTINATION lib
#     ARCHIVE DESTINATION lib
#     RUNTIME DESTINATION bin
# )

install(FILES ${HEADERS}
    DESTINATION include/${PROJECT_NAME}
)
