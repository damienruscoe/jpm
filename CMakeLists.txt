cmake_minimum_required(VERSION 3.16)
project(matching_engine CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

option(BUILD_GUI "Build the ImGui/ImPlot dashboard sample (Bloomberg terminal clone)" OFF)
option(ENABLE_SANITIZERS "Build non-Debug configs with ASan/UBSan (matches old default)" ON)
option(WASM "Configure this build for WebAssembly via Emscripten" OFF)

if(WASM AND NOT EMSCRIPTEN)
    message(FATAL_ERROR "-DWASM=ON requires the Emscripten toolchain:\n"
                         "`emcmake cmake -S . -B build-wasm -DWASM=ON`")
endif()

# ---------------------------------------------------------------------------
# Common warning / sanitizer flags
# Debug config: -ggdb -O2, no sanitizers (matches old DEBUG_CXXFLAGS)
# Everything else: -O3, sanitizers optional (matches old CXXFLAGS default)
# ---------------------------------------------------------------------------
add_library(project_warnings INTERFACE)

if (WASM)
    target_compile_options(project_warnings INTERFACE
        #-Wall -Wextra -Wpedantic -Werror
        -Wno-error=pedantic -Wno-error=deprecated-literal-operator
        $<$<CONFIG:Debug>:-ggdb -O0>
        $<$<NOT:$<CONFIG:Debug>>:-O3>
    )
else()
    target_compile_options(project_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Werror -Wno-error=pedantic
        $<$<CONFIG:Debug>:-ggdb -O0>
        $<$<NOT:$<CONFIG:Debug>>:-O3>
    )
endif()

add_library(project_sanitizers INTERFACE)
if(ENABLE_SANITIZERS)
    target_compile_options(project_sanitizers INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:-fsanitize=address,undefined>
    )
    target_link_options(project_sanitizers INTERFACE
        $<$<NOT:$<CONFIG:Debug>>:-fsanitize=address,undefined>
    )
endif()

# ---------------------------------------------------------------------------
# Core library — everything under src/ and src/parser/, excluding the two
# app entry points (src/apps/main.cpp, src/apps/sample_dashboard.cpp) and
# the UI-only sources under src/ui/. CONFIGURE_DEPENDS re-globs on rebuild
# so new files are picked up without a manual reconfigure (same convenience
# the old wildcard-based Makefile gave you, without its full-rebuild cost).
# ---------------------------------------------------------------------------
file(GLOB CORE_SRCS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

add_library(core STATIC ${CORE_SRCS})
target_include_directories(core PUBLIC
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
)
target_include_directories(core SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/deps/cnl/include
)
target_link_libraries(core PUBLIC project_warnings project_sanitizers)


include(FetchContent)
FetchContent_Declare(
  boost_headers
  URL https://github.com/emscripten-ports/boost/releases/download/boost-1.83.0/boost-headers-1.83.0.zip
  URL_HASH SHA512=82e3a64e55caac0254f9e6b179437ad9421943b8f73957285978dad16c24dcae6372c464909fdc085b9790662b6a3af5163140b1e456705e80fda51c5fe3c243
)
FetchContent_MakeAvailable(boost_headers)



if (WASM)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    #add_compile_options(-s USE_BOOST_HEADERS=1)
    #add_link_options(-s USE_BOOST_HEADERS=1)

    add_link_options(
        "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/pages/shell.html"
        -sMIN_WEBGL_VERSION=1
        -sMAX_WEBGL_VERSION=2
        -sUSE_WEBGL2=1
        -sOFFSCREENCANVAS_SUPPORT=0
        -sALLOW_MEMORY_GROWTH=1
        $<$<CONFIG:Debug>:-sGL_DEBUG=1>
        $<$<CONFIG:Debug>:-sASSERTIONS>
        $<$<CONFIG:Debug>:-fexceptions>
        $<$<CONFIG:Debug>:-g>
    )
endif()

if (NOT WASM)
    find_package(databento REQUIRED)
    find_package(OpenSSL REQUIRED)
    find_package(nlohmann_json REQUIRED)


    file(GLOB PARSER_SRCS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/parser/*.cpp")
    add_library(parsers STATIC ${PARSER_SRCS})
    target_include_directories(parsers PUBLIC
        ${CMAKE_SOURCE_DIR}/src
    )
    target_include_directories(parsers SYSTEM PUBLIC
        ${CMAKE_SOURCE_DIR}/deps/cnl/include
    )
    target_link_libraries(parsers PUBLIC project_warnings project_sanitizers)
endif()


if (NOT WASM)
    add_executable(matching_engine src/apps/main.cpp)
    target_link_libraries(matching_engine PRIVATE
        core
        parsers
        databento::databento
    )


    add_executable(gemini src/apps/gemini.cpp)
    target_include_directories(gemini PUBLIC
        ../queueco/deps/websocketpp
    )
    target_link_libraries(gemini PRIVATE
        core
        OpenSSL::SSL
        OpenSSL::Crypto
        nlohmann_json::nlohmann_json
    )
endif()


# ---------------------------------------------------------------------------
# matching_engine_tests
# find_package first (system/vendored gtest); FetchContent fallback so a
# machine without libgtest-dev installed still builds.
# ---------------------------------------------------------------------------
if (NOT WASM)

    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG v1.15.2
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        add_library(GTest::gtest ALIAS gtest)
        add_library(GTest::gtest_main ALIAS gtest_main)
    endif()

    file(GLOB TEST_SRCS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/test/*.cpp")
    add_executable(matching_engine_tests ${TEST_SRCS})
    target_link_libraries(matching_engine_tests PRIVATE
        core
        parsers
        GTest::gtest GTest::gtest_main
    )

endif()

# ---------------------------------------------------------------------------
# bloomberg — optional, off by default (-DBUILD_GUI=ON to enable).
# ImGui and ImPlot ship without a top-level CMakeLists, so — same as the
# old Makefile did by hand — we compile their sources directly into a
# library target ourselves. GLFW *does* ship a full CMake project, so
# FetchContent gives us a ready-made `glfw` target for free, which is
# what removes the old hardcoded chronovis build-tree paths and -lglfw.
# ---------------------------------------------------------------------------
if(BUILD_GUI)
    if(WASM)
        include(FetchContent)
        FetchContent_Declare(imgui  GIT_REPOSITORY https://github.com/ocornut/imgui.git  GIT_TAG v1.91.5)
        FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot.git GIT_TAG v0.16)
        FetchContent_MakeAvailable(imgui implot)

        add_library(imgui_lib STATIC
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        target_include_directories(imgui_lib PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        # PUBLIC here means every executable that links imgui_lib gets these
        # link flags for free — no per-executable repetition needed.
        target_link_options(imgui_lib PUBLIC -sUSE_GLFW=3 -sFULL_ES3=1 -sUSE_WEBGL2=1 -sALLOW_MEMORY_GROWTH=1)
    else()
        include(FetchContent)

        FetchContent_Declare(imgui  GIT_REPOSITORY https://github.com/ocornut/imgui.git  GIT_TAG v1.91.5)
        FetchContent_Declare(implot GIT_REPOSITORY https://github.com/epezent/implot.git GIT_TAG v0.16)
        FetchContent_MakeAvailable(imgui implot)
            FetchContent_Declare(readerwriterqueue GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue.git GIT_TAG v1.0.7)
            FetchContent_MakeAvailable(readerwriterqueue)

        # GLFW: prefer whatever's already on the machine (a CMake config
        # package, or failing that pkg-config) before building it from
        # source — building GLFW itself pulls in X11/Wayland dev headers
        # that a from-source fetch shouldn't force on someone who already
        # has libglfw3-dev installed.
        find_package(glfw3 3.3 QUIET)
        if(NOT TARGET glfw)
            find_package(PkgConfig QUIET)
            if(PkgConfig_FOUND)
                pkg_check_modules(GLFW3_PC QUIET glfw3)
            endif()
            if(GLFW3_PC_FOUND)
                add_library(glfw INTERFACE)
                target_include_directories(glfw INTERFACE ${GLFW3_PC_INCLUDE_DIRS})
                target_link_directories(glfw INTERFACE ${GLFW3_PC_LIBRARY_DIRS})
                target_link_libraries(glfw INTERFACE ${GLFW3_PC_LIBRARIES})
            else()
                message(STATUS "System GLFW not found — fetching and building from source "
                                "(needs X11 or Wayland dev headers, e.g. xorg-dev)")
                FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
                FetchContent_MakeAvailable(glfw)
            endif()
        endif()

        find_package(OpenGL REQUIRED)

        set(IMGUI_CORE_SRCS
            ${imgui_SOURCE_DIR}/imgui.cpp
            ${imgui_SOURCE_DIR}/imgui_draw.cpp
            ${imgui_SOURCE_DIR}/imgui_tables.cpp
            ${imgui_SOURCE_DIR}/imgui_widgets.cpp
            ${imgui_SOURCE_DIR}/imgui_demo.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
            ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
        )
        add_library(imgui_lib STATIC ${IMGUI_CORE_SRCS})
        target_include_directories(imgui_lib PUBLIC
            ${imgui_SOURCE_DIR}
            ${imgui_SOURCE_DIR}/backends
        )
        target_link_libraries(imgui_lib PUBLIC glfw OpenGL::GL)

    endif()



    add_library(implot_lib STATIC
        ${implot_SOURCE_DIR}/implot.cpp
        ${implot_SOURCE_DIR}/implot_items.cpp
        ${implot_SOURCE_DIR}/implot_demo.cpp
    )
    target_include_directories(implot_lib PUBLIC
        ${implot_SOURCE_DIR}
    )
    target_link_libraries(implot_lib PUBLIC
        imgui_lib
    )




    file(GLOB UI_SRCS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/ui/*.cpp")


    add_executable(fake_dashboard src/apps/sample_dashboard.cpp ${UI_SRCS})
    target_include_directories(fake_dashboard SYSTEM PRIVATE ${boost_headers_SOURCE_DIR})
    target_link_libraries(fake_dashboard PRIVATE
        core
        implot_lib
    )
    #target_link_options(fake_dashboard PRIVATE "-s TEXTDECODER=0")


    if (NOT WASM)
        add_executable(bloomberg src/apps/bloomberg.cpp ${UI_SRCS})
        target_link_libraries(bloomberg PRIVATE
            core
            parsers
            implot_lib
            databento::databento
            readerwriterqueue
        )


        add_executable(gemini_dashboard src/apps/gemini_gui.cpp ${UI_SRCS})
        target_include_directories(gemini_dashboard PUBLIC
            ../queueco/deps/websocketpp
        )
        target_link_libraries(gemini_dashboard PRIVATE
            core
            parsers
            implot_lib
            OpenSSL::SSL
            OpenSSL::Crypto
            nlohmann_json::nlohmann_json
            readerwriterqueue
        )
    endif()
endif()
