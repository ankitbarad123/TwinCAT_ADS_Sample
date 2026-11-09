cmake_minimum_required(VERSION 3.20)
project(ads_counter_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Router selection
#   ON  : use the TwinCAT router installed on this PC (TcAdsDll). Use this when the
#         PLC runs on THIS PC (e.g. TwinCAT simulation / local runtime), or when you
#         already have a route set up in TwinCAT. No extra route needed.
#   OFF : use the library's own standalone router (no TwinCAT needed on this PC, but
#         the PLC needs a static route to this PC).
# Default: ON on Windows if TwinCAT's TcAdsDll is found, otherwise OFF.
# ---------------------------------------------------------------------------
option(ADS_USE_TWINCAT_ROUTER "Use the local TwinCAT router (TcAdsDll)" ON)

# ---------------------------------------------------------------------------
# Fetch the Beckhoff ADS library (pinned to a known commit).
# SOURCE_SUBDIR points at a folder that does not exist on purpose: we only want
# the sources downloaded, not the library's own top-level CMakeLists (which
# expects to be the root project). We compile the files we need below.
# ---------------------------------------------------------------------------
include(FetchContent)
FetchContent_Declare(
    beckhoff_ads
    GIT_REPOSITORY https://github.com/Beckhoff/ADS.git
    GIT_TAG        24e598a113edfdc0d34414bfefb847930d94cb87
    GIT_SUBMODULES ""   # skip the submodule step (not needed, and it failed on Windows git)
    SOURCE_SUBDIR  _do_not_add_subdirectory
)
FetchContent_MakeAvailable(beckhoff_ads)
set(ADSLIB_DIR "${beckhoff_ads_SOURCE_DIR}/AdsLib")

# Files shared by both variants
set(ADS_COMMON_SOURCES
    ${ADSLIB_DIR}/AdsDef.cpp
    ${ADSLIB_DIR}/AdsDevice.cpp
    ${ADSLIB_DIR}/AdsFile.cpp
    ${ADSLIB_DIR}/AdsLib.cpp
    ${ADSLIB_DIR}/ECatAccess.cpp
    ${ADSLIB_DIR}/Frame.cpp
    ${ADSLIB_DIR}/LicenseAccess.cpp
    ${ADSLIB_DIR}/Log.cpp
    ${ADSLIB_DIR}/MasterDcStatAccess.cpp
    ${ADSLIB_DIR}/RTimeAccess.cpp
    ${ADSLIB_DIR}/RegistryAccess.cpp
    ${ADSLIB_DIR}/RouterAccess.cpp
    ${ADSLIB_DIR}/Sockets.cpp
    ${ADSLIB_DIR}/SymbolAccess.cpp
    ${ADSLIB_DIR}/SysServAccess.cpp
    ${ADSLIB_DIR}/bhf/ParameterList.cpp
)

set(_use_tc FALSE)
if(ADS_USE_TWINCAT_ROUTER AND WIN32)
    list(APPEND CMAKE_MODULE_PATH "${beckhoff_ads_SOURCE_DIR}/cmake")
    find_package(TcAdsDll)  # looks in C:/TwinCAT/AdsApi/TcAdsDll or C:/Program Files (x86)/Beckhoff/TwinCAT/AdsApi/TcAdsDll
    if(TcAdsDll_FOUND)
        set(_use_tc TRUE)
    else()
        message(WARNING "TcAdsDll not found - falling back to the standalone router. "
                        "If TwinCAT is installed, set -DTcAdsDll_ROOT=<...>/AdsApi/TcAdsDll")
    endif()
endif()

add_library(AdsLib STATIC ${ADS_COMMON_SOURCES})
target_include_directories(AdsLib PUBLIC ${ADSLIB_DIR})
target_compile_definitions(AdsLib PUBLIC CONFIG_DEFAULT_LOGLEVEL=1)

find_package(Threads REQUIRED)
target_link_libraries(AdsLib PUBLIC Threads::Threads)

if(_use_tc)
    message(STATUS "ADS router: TwinCAT (TcAdsDll)")
    target_sources(AdsLib PRIVATE ${ADSLIB_DIR}/TwinCAT/AdsLib.cpp)
    target_compile_definitions(AdsLib PUBLIC USE_TWINCAT_ROUTER)
    target_link_libraries(AdsLib PUBLIC TcAdsDll::TcAdsDll)
else()
    message(STATUS "ADS router: standalone (built into the library)")
    target_sources(AdsLib PRIVATE
        ${ADSLIB_DIR}/standalone/AdsLib.cpp
        ${ADSLIB_DIR}/standalone/AmsConnection.cpp
        ${ADSLIB_DIR}/standalone/AmsNetId.cpp
        ${ADSLIB_DIR}/standalone/AmsPort.cpp
        ${ADSLIB_DIR}/standalone/AmsRouter.cpp
        ${ADSLIB_DIR}/standalone/NotificationDispatcher.cpp
    )
endif()

if(MSVC)
    target_compile_definitions(AdsLib PUBLIC NOMINMAX _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
    target_compile_options(AdsLib PUBLIC /utf-8)
    target_link_libraries(AdsLib PUBLIC wsock32 ws2_32)
elseif(WIN32)  # MinGW
    target_link_libraries(AdsLib PUBLIC wsock32 ws2_32)
else()
    target_compile_definitions(AdsLib PUBLIC _GNU_SOURCE)
endif()

# ---------------------------------------------------------------------------
# The demo application
# ---------------------------------------------------------------------------
add_executable(ads_counter_demo main.cpp)
target_link_libraries(ads_counter_demo PRIVATE AdsLib)
