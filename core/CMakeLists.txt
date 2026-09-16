add_library(puddin_core SHARED
    src/engine.cpp
    src/collision.cpp
    src/trajectory.cpp
    src/PuddinWisdomEngine.cpp
    src/GameRules.cpp
    src/DiamondMath.cpp
    src/PuddinWisdomAI.cpp
    src/NineBallRules.cpp
    src/OfficialRules.cpp
    src/PhysicsEngine.cpp
    src/ValleyTableGeometry.cpp
)

target_include_directories(puddin_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(puddin_core PUBLIC cxx_std_20)

set_target_properties(puddin_core PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN NO
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME "puddin_core"
)

# Apple macOS dylib & iOS framework configurations
if(APPLE)
    set_target_properties(puddin_core PROPERTIES
        MACOSX_RPATH TRUE
        INSTALL_NAME_DIR "@rpath"
    )
    if(IOS)
        set_target_properties(puddin_core PROPERTIES
            FRAMEWORK TRUE
            MACOSX_FRAMEWORK_IDENTIFIER "com.puddin.core"
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        )
    endif()
endif()

# Android NDK configuration
if(ANDROID)
    target_link_libraries(puddin_core PRIVATE log)
endif()
