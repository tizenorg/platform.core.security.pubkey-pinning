# Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#        http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#
# @file     CMakeLists.txt
# @author   Kyungwook Tak (k.tak@samsung.com)
# @brief    TPKP Project makefile on top
#
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(pubkey-pinning)

INCLUDE(FindPkgConfig)

STRING(REGEX MATCH "([^.]*)" SO_VERSION "${VERSION}")

# compiler options
SET(GC_SECTIONS_FLAGS         "-fdata-sections -ffunction-sections -Wl,--gc-sections")
SET(CMAKE_C_FLAGS             "${CMAKE_C_FLAGS} ${GC_SECTIONS_FLAGS}")
SET(CMAKE_CXX_FLAGS           "${CMAKE_CXX_FLAGS} ${GC_SECTIONS_FLAGS}")
SET(CMAKE_C_FLAGS_RELEASE     "-g -O2")
SET(CMAKE_CXX_FLAGS_RELEASE   "-g -std=c++0x -O2")
SET(CMAKE_C_FLAGS_DEBUG       "-g -O0 -Wp,-U_FORTIFY_SOURCE")
SET(CMAKE_CXX_FLAGS_DEBUG     "-g -std=c++0x -O0 -Wp,-U_FORTIFY_SOURCE")
SET(CMAKE_C_FLAGS_CCOV        "-g -O2 --coverage")
SET(CMAKE_CXX_FLAGS_CCOV      "-g -std=c++0x -O2 --coverage")
SET(CMAKE_SHARED_LINKER_FLAGS "-Wl,--as-needed")
SET(CMAKE_EXE_LINKER_FLAGS    "-Wl,--as-needed")
SET(CMAKE_SKIP_RPATH          "TRUE")

IF (CMAKE_BUILD_TYPE MATCHES "DEBUG")
ADD_DEFINITIONS("-DBUILD_TYPE_DEBUG")
ADD_DEFINITIONS("-DDPL_LOGS_ENABLED")
ENDIF (CMAKE_BUILD_TYPE MATCHES "DEBUG")

# compiler warning flags
ADD_DEFINITIONS("-Wall")
ADD_DEFINITIONS("-Wextra")
ADD_DEFINITIONS("-Werror")

SET(TARGET_TPKP_COMMON_LIB "tpkp-common")
SET(TARGET_TPKP_CURL_LIB "tpkp-curl")
SET(TARGET_TPKP_GNUTLS_LIB "tpkp-gnutls")

CONFIGURE_FILE(tpkp-curl.pc.in tpkp-curl.pc @ONLY)
CONFIGURE_FILE(tpkp-gnutls.pc.in tpkp-gnutls.pc @ONLY)
INSTALL(
	FILES
		tpkp-curl.pc
		tpkp-gnutls.pc
	DESTINATION
		${LIB_INSTALL_DIR}/pkgconfig
	)

ADD_SUBDIRECTORY(src)

IF (DEFINED PUBKEY_PINNING_TEST_BUILD)
SET(TARGET_TPKP_TEST "tpkp-internal-test")
ADD_SUBDIRECTORY(test)
ENDIF (DEFINED PUBKEY_PINNING_TEST_BUILD)
