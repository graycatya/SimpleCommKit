# SimpleCommKitUtil CMakeLists.txt
# Compiled utility library for common error handling and utilities

set(MODULE_NAME SimpleCommKitUtil)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS  ON)
endif()

# Create compiled library (static or shared, controlled by BUILD_SHARED_LIBS)
add_library(${MODULE_NAME}
    SimpleCommKitErrorMap.hpp
    SimpleCommKitErrorMap.cpp
    SimpleCommKitExport.h
    ${CMAKE_BINARY_DIR}/generated/SimpleCommKitVersion.cpp
)

# Set public include directories so other targets can find headers
target_include_directories(${MODULE_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/include
)

# Require C++17 or later
target_compile_features(${MODULE_NAME} PUBLIC cxx_std_17)

# When building as shared library, define SIMPLECOMMKIT_EXPORTS to enable dllexport
if(BUILD_SHARED_LIBS)
    target_compile_definitions(${MODULE_NAME} PRIVATE SIMPLECOMMKIT_EXPORTS)
endif()

# Installation rules - install headers to include directory
install(FILES SimpleCommKitErrorMap.hpp SimpleCommKitExport.h
    DESTINATION include/${MODULE_NAME}
)
# Install the include version header (built by root CMakeLists.txt)
install(FILES ${CMAKE_BINARY_DIR}/include/SimpleCommKitVersion.h
    DESTINATION include/${MODULE_NAME}
)

# Export the library for use by other projects
install(TARGETS ${MODULE_NAME}
    EXPORT SimpleCommKitUtilTargets
    INCLUDES DESTINATION include
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install export targets
# install(EXPORT SimpleCommKitUtilTargets
#     FILE SimpleCommKitUtilTargets.cmake
#     NAMESPACE SimpleCommKit::
#     DESTINATION lib/cmake/SimpleCommKitUtil
# )
