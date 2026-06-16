# SimpleCommKitUtil CMakeLists.txt
# Header-only utility library for common error handling and utilities

set(MODULE_NAME SimpleCommKitUtil)

if(SIMPLECOMMKIT_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS  ON)
endif()


# Create header-only interface library
add_library(${MODULE_NAME} INTERFACE  SimpleCommKitErrorMap.hpp
    SimpleCommKitExport.h   
)

# Set public include directories so other targets can find headers
target_include_directories(${MODULE_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Require C++17 or later (matching project standard)
target_compile_features(${MODULE_NAME} INTERFACE cxx_std_17)

# Optional: Add compile definitions if needed
# target_compile_definitions(${MODULE_NAME} INTERFACE SimpleCommKit_UTIL_VERSION="1.0.0")

# Installation rules - install headers to include directory
install(FILES SimpleCommKitErrorMap.hpp
    DESTINATION include/${MODULE_NAME}
)
# Install the generated version header (built by root CMakeLists.txt)
install(FILES ${CMAKE_BINARY_DIR}/generated/SimpleCommKitVersion.h
    DESTINATION include/${MODULE_NAME}
)

# Export the library for use by other projects
install(TARGETS ${MODULE_NAME}
    EXPORT SimpleCommKitUtilTargets
    INCLUDES DESTINATION include
)

# Install export targets
# install(EXPORT SimpleCommKitUtilTargets
#     FILE SimpleCommKitUtilTargets.cmake
#     NAMESPACE SimpleCommKit::
#     DESTINATION lib/cmake/SimpleCommKitUtil
# )
