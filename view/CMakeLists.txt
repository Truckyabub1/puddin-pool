add_library(puddin_view STATIC
    src/cli_view.cpp
)

target_include_directories(puddin_view
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(puddin_view PUBLIC cxx_std_20)

target_link_libraries(puddin_view PUBLIC puddin_core)
