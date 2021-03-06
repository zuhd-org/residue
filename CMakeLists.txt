cmake_minimum_required (VERSION 2.8.12)

project (Residue)

option (production "Build for production" ON)
option (debug "Enable debug logging" OFF)
option (profiling "Turns profiling on for various scenarios" OFF)
option (enable_extensions "Enables extensions support for the build" ON)
option (use_mine "Use mine whereever possible" OFF)
option (use_boost "Use boost or standalone networking lib" OFF)
option (old_toolchain "Is old toolchain" OFF)

set (RESIDUE_MAJOR "2")
set (RESIDUE_MINOR "3")
set (RESIDUE_PATCH "6")
set (RESIDUE_VERSION "${RESIDUE_MAJOR}.${RESIDUE_MINOR}.${RESIDUE_PATCH}")
set (RESIDUE_NAME "Residue")

add_definitions (-DRESIDUE_VERSION="${RESIDUE_VERSION}")
add_definitions (-DRESIDUE_VERSION_MAJOR="${RESIDUE_MAJOR}")
add_definitions (-DRESIDUE_VERSION_MINOR="${RESIDUE_MINOR}")
add_definitions (-DRESIDUE_VERSION_PATCH="${RESIDUE_PATCH}")

add_definitions (-DRIPE_VERSION="4.1.1-static")

if (enable_extensions)
    add_definitions (-DRESIDUE_HAS_EXTENSIONS)
endif(enable_extensions)

if (old_toolchain)
    message ("==> USING OLD TOOLCHAIN")
    add_definitions (-D_GLIBCXX_USE_NANOSLEEP)
endif(old_toolchain)

if (use_mine)
    add_definitions (-DRESIDUE_USE_MINE)
endif(use_mine)

if (profiling)
    message ("==> PROFILING IS ON")
    add_definitions (-DRESIDUE_PROFILING)
endif(profiling)

if (debug)
    message ("==> DEBUGGING IS ON")
    add_definitions (-DRESIDUE_DEBUG)
endif(debug)

set (test "OFF")
if (production)
    message ("==> PRODUCTION BUILD")
    add_definitions (-DRESIDUE_PRODUCTION)
else(production)
    message ("==> DEVELOPMENT MODE")
    add_definitions (-DRESIDUE_DEV)
    add_definitions (-DELPP_DEBUG_ERRORS)
    add_definitions (-DELPP_DEBUG_INFO)
    set (CMAKE_BUILD_TYPE "DEBUG")
    set (test "ON")
endif(production)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
include(residue-dev)

include (FindPackageHandleStandardArgs)
include (FindCURL)

check_apple()

set (cxx_standard c++11)
set (optimization_level 0)

if (production)
    set (optimization_level 3)
endif (production)

set (extra_flags "")
if (APPLE)
    set (extra_flags "-mmacosx-version-min=10.10")
endif()

list (APPEND CMAKE_CXX_FLAGS " -std=${cxx_standard} -O${optimization_level} ${extra_flags} -Wall -Wextra -pedantic -pedantic-errors -Wno-undefined-var-template -Wno-undef -Wno-unused-local-typedef -Wno-unused-variable -Wno-deprecated-declarations -Wno-unknown-warning-option -Wno-pessimizing-move ")

if (CMAKE_BUILD_TYPE EQUAL "DEBUG" OR CMAKE_BUILD_TYPE EQUAL "RELWITHDEBINFO")
    message ("==> EXPORTING DEBUG SYMBOLS")
endif()

## Prefer static linking
# SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

find_package (CURL REQUIRED)

if (CURL_FOUND)
    add_definitions (-DRESIDUE_HAS_CURL)
    include_directories (${CURL_INCLUDE_DIRS})
    message ("-- libcurl: " ${CURL_LIBRARIES} " version: " ${CURL_VERSION_STRING})
endif()

find_package(ZLIB REQUIRED)
if (ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIRS})
    message ("-- libz: " ${ZLIB_LIBRARIES} " version: " ${ZLIB_VERSION_STRING})
endif(ZLIB_FOUND)

if (use_boost)
    message ("==> BASED ON BOOST")
    add_definitions (-DRESIDUE_BOOST)
    # Boost
    message ("-- Check for Boost System (static)")
    set (Boost_USE_STATIC_LIBS ON)
    find_package (Boost REQUIRED COMPONENTS system)
    include_directories (${Boost_INCLUDE_DIR})
    message ("-- libboost: " ${Boost_LIBRARIES})
    set(NET_LIBRARIES ${Boost_LIBRARIES})
else()
    message ("==> BASED ON STANDALONE ASIO")
    add_definitions (-DASIO_STANDALONE)
endif(use_boost)

# Check for cryptopp (static)
set(CryptoPP_USE_STATIC_LIBS ON)
find_package(CryptoPP REQUIRED)
message ("-- Crypto++ binary: " ${CRYPTOPP_LIBRARY})
include_directories (${CRYPTOPP_INCLUDE_DIRS})

thread_packages_check()

include_directories (${CMAKE_SOURCE_DIR})
include_directories (${CMAKE_SOURCE_DIR}/deps)
include_directories (${CMAKE_SOURCE_DIR}/src)

if (use_mine)
    set (MINE_SRC deps/mine/mine.cc)
else()
    set (MINE_SRC "")
endif()

set (DEPS_SRC
    ${MINE_SRC}
    deps/linenoise/linenoise.c
    deps/easyloggingpp/easylogging++.cc
    deps/ripe/Ripe.cc
    deps/gason/gason.cc
)

set (SOURCE_FILES
    src/setup.cc

    src/connect/connection-request.cc
    src/connect/connection-request-handler.cc
    src/connect/connection-response.cc

    src/admin/admin-request.cc
    src/admin/admin-request-handler.cc

    src/logging/log-request-handler.cc
    src/logging/log-request.cc
    src/logging/user-log-builder.cc
    src/logging/user-message.cc
    src/logging/logging-queue.cc
    src/logging/client-queue-processor.cc

    src/core/client.cc
    src/core/request.cc
    src/core/request-handler.cc
    src/core/response.cc
    src/core/configuration.cc
    src/core/registry.cc
    src/core/json-doc.cc

    src/crypto/rsa.cc
    src/crypto/aes.cc
    src/crypto/base64.cc
    src/crypto/base16.cc
    src/crypto/zlib.cc

    src/net/server.cc
    src/net/session.cc
    src/net/url.cc
    src/net/http-client.cc

    src/cli/command.cc
    src/cli/command-handler.cc
    src/cli/update.cc
    src/cli/reset.cc
    src/cli/reload-config.cc
    src/cli/stats.cc
    src/cli/rotate.cc
    src/cli/list-logging-files.cc
    src/cli/clients.cc

    src/extensions/extension.cc
    src/extensions/log-extension.cc
    src/extensions/pre-archive-extension.cc
    src/extensions/post-archive-extension.cc

    src/tasks/auto-updater.cc
    src/tasks/client-integrity-task.cc
    src/tasks/task.cc
    src/tasks/log-rotator.cc

    src/utils/tar.cc
    src/utils/utils.cc
)

set (SHARED_REQUIRED_LIBS
    ${NET_LIBRARIES}
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${ZLIB_LIBRARIES}
    ${CRYPTOPP_LIBRARIES}
)

if (enable_extensions)
    set (SHARED_REQUIRED_LIBS ${SHARED_REQUIRED_LIBS} ${CMAKE_DL_LIBS})
endif(enable_extensions)

############# RESIDUE CORE LIB (STATIC) ###############

add_library (residue-core STATIC ${SOURCE_FILES})

set_target_properties (residue-core PROPERTIES
    VERSION ${RESIDUE_VERSION}
)

target_link_libraries (residue-core ${SHARED_REQUIRED_LIBS})

target_compile_definitions (residue-core PUBLIC
    ELPP_FORCE_USE_STD_THREAD
    ELPP_THREAD_SAFE
    ELPP_NO_GLOBAL_LOCK
)

# Set RPATH to library install path.
set (CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

############# RESIDUE EXTENSION LIB (STATIC) ###############

if (enable_extensions)
    set (EXTENSION_FILES
        src/extensions/extension.cc
        src/extensions/log-extension.cc
        src/extensions/dispatch-error-extension.cc
        src/extensions/pre-archive-extension.cc
        src/extensions/post-archive-extension.cc
        src/core/json-doc.cc
        deps/gason/gason.cc
    )

    add_library (residue-extension-st STATIC ${EXTENSION_FILES})

    set_target_properties (residue-extension-st PROPERTIES
        VERSION ${RESIDUE_VERSION}
    )

    target_compile_definitions (residue-extension-st PUBLIC
        RESIDUE_EXTENSION_LIB
    )

    set_target_properties (residue-extension-st PROPERTIES OUTPUT_NAME "residue-extension-st")
    # install (TARGETS residue-extension-st DESTINATION lib)

    ######### Shared lib for extension
    add_library (residue-extension SHARED ${EXTENSION_FILES})

    set_target_properties (residue-extension PROPERTIES
        VERSION ${RESIDUE_VERSION}
    )
    target_link_libraries (residue-extension
        residue-extension-st
    )

    target_compile_definitions (residue-extension PUBLIC
        RESIDUE_EXTENSION_LIB
    )

    set_target_properties (residue-extension PROPERTIES OUTPUT_NAME "residue-extension")
    install (TARGETS residue-extension DESTINATION lib)

    file(GLOB EXTENSIONS_H "src/extensions/*.h")
    install (FILES ${EXTENSIONS_H} DESTINATION "include/residue/extensions/")
    install (FILES src/core/json-doc.h DESTINATION "include/residue/core/")
    install (FILES deps/gason/gason.h DESTINATION "include/residue/gason/")
    install (FILES include/extensions.h DESTINATION "include/residue/")

endif(enable_extensions)

############# CONFIG VALIDATOR TOOL ###############

if (true)
    add_executable (residue-config-validator-bin
        tools/config-validator/main.cc
        ${DEPS_SRC}
    )

    target_compile_definitions (residue-config-validator-bin PUBLIC
        ELPP_NO_DEFAULT_LOG_FILE
        ELPP_NO_LOG_TO_FILE
        ELPP_STL_LOGGING
        AUTO_INITIALIZE_EASYLOGGINGPP
        ELPP_DEFAULT_LOGGING_FLAGS=4096
    )

    target_link_libraries (residue-config-validator-bin
        residue-core)

    set_target_properties (residue-config-validator-bin PROPERTIES
        OUTPUT_NAME "residue-config-validator"
        VERSION ${RESIDUE_VERSION}
    )

    #install (TARGETS residue-config-validator-bin DESTINATION bin)
endif()

############# RESIDUE BINARY ###############

add_executable (residue-bin
    src/main.cc
    ${DEPS_SRC}
)

target_compile_definitions (residue-bin PUBLIC
    ELPP_STL_LOGGING
    ELPP_STACKTRACE
    ELPP_FEATURE_CRASH_LOG
    ELPP_NO_DEFAULT_LOG_FILE
)

set_target_properties (residue-bin PROPERTIES
    VERSION ${RESIDUE_VERSION}
    OUTPUT_NAME "residue"
)

target_link_libraries (residue-bin residue-core)

set_target_properties (residue-bin PROPERTIES OUTPUT_NAME "residue")
install (TARGETS residue-bin DESTINATION bin)

#############################################

# Reference all headers, to make certain IDEs happy.
file (GLOB_RECURSE all_headers ${CMAKE_SOURCE_DIR}/*.h)
add_custom_target (all_placeholder SOURCES ${all_headers})


############# RESIDUE TESTS ###############
if (test)

    find_package (gtest REQUIRED)

    include_directories (${GTEST_INCLUDE_DIRS})

    enable_testing()

    add_executable (residue-unit-tests
        test/main.cc
        ${DEPS_SRC}
    )

    target_compile_definitions (residue-unit-tests PUBLIC
        ELPP_FEATURE_ALL
        ELPP_STL_LOGGING
        ELPP_DEFAULT_LOG_FILE="/tmp/logs/residue_unit_tests.log"
        ELPP_DEFAULT_LOGGING_FLAGS=4096
    )

    # Standard linking to gtest stuff.
    target_link_libraries (residue-unit-tests ${GTEST_LIBRARIES})

    # Extra linking for the project.
    target_link_libraries (residue-unit-tests residue-core)

    target_link_libraries (residue-unit-tests ${SHARED_REQUIRED_LIBS})

    add_test (NAME residueUnitTests COMMAND residue-unit-tests)
endif() ## test
