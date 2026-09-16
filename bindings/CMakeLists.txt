add_library(puddinpool SHARED
    puddin_pool_ffi.cpp
    src/puddin_c_abi.cpp
)

target_include_directories(puddinpool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(puddinpool PUBLIC cxx_std_20)
target_link_libraries(puddinpool PUBLIC puddin_core)

set_target_properties(puddinpool PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN NO
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "puddinpool"
)

# Static library target for iOS / embedded static link
add_library(puddinpool_static STATIC
    puddin_pool_ffi.cpp
    src/puddin_c_abi.cpp
)

target_include_directories(puddinpool_static
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(puddinpool_static PUBLIC cxx_std_20)
target_link_libraries(puddinpool_static PUBLIC puddin_core)
set_target_properties(puddinpool_static PROPERTIES
    OUTPUT_NAME "puddinpool"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

if(APPLE)
    set_target_properties(puddinpool PROPERTIES
        MACOSX_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
    )
    if(IOS)
        set_target_properties(puddinpool PROPERTIES
            FRAMEWORK TRUE
            MACOSX_FRAMEWORK_IDENTIFIER "com.puddin.pool"
        )
    endif()
endif()

# Backward compatibility alias
add_library(puddin_bindings ALIAS puddinpool)
