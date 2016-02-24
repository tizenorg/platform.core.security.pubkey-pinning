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
# @brief    TPKP test makefile
#
ADD_DEFINITIONS("-DBOOST_TEST_DYN_LINK")

PKG_CHECK_MODULES(TEST_TPKP_DEP
	REQUIRED
	openssl
	libcurl
	gnutls
	)

INCLUDE_DIRECTORIES(
	SYSTEM
	${TEST_TPKP_DEP_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/src/curl/include   # tpkp API header
	${PROJECT_SOURCE_DIR}/src/gnutls/include # tpkp API header
	${PROJECT_SOURCE_DIR}/src/common/include # tpkp error header in common
	)

SET(TEST_SRCS
	colour_log_formatter.cpp
	colors.cpp
	main.cpp
	gnutls_test.cpp
	curl_test.cpp
	)

ADD_EXECUTABLE(${TARGET_TPKP_TEST} ${TEST_SRCS})

TARGET_LINK_LIBRARIES(${TARGET_TPKP_TEST}
	${TEST_TPKP_DEP_LIBRARIES}
	${TARGET_TPKP_GNUTLS_LIB}
	${TARGET_TPKP_CURL_LIB}
	boost_unit_test_framework
	)

# run popup test
PKG_CHECK_MODULES(TEST_TPKP_POPUP_DEP
	REQUIRED
	dlog
	)

INCLUDE_DIRECTORIES(
	SYSTEM
	${TEST_TPKP_POPUP_DEP_INCLUDE_DIRS}
	${PROJECT_SOURCE_DIR}/src/common/include
	${PROJECT_SOURCE_DIR}/src/common
	)

SET(TEST_POPUP_SRCS
	${PROJECT_SOURCE_DIR}/src/common/url/url_constants.cc
	${PROJECT_SOURCE_DIR}/src/common/url/url_util.cc
	${PROJECT_SOURCE_DIR}/src/common/url/url_parse.cc
	${PROJECT_SOURCE_DIR}/src/common/url/url_parse_file.cc
	${PROJECT_SOURCE_DIR}/src/common/net/http/transport_security_state.cpp
	${PROJECT_SOURCE_DIR}/src/common/src/tpkp_common.cpp
	${PROJECT_SOURCE_DIR}/src/common/src/tpkp_exception.cpp
	${PROJECT_SOURCE_DIR}/src/common/src/tpkp_parser.cpp
	${PROJECT_SOURCE_DIR}/src/common/ui/popup_runner.cpp
	${PROJECT_SOURCE_DIR}/src/common/ui/popup_common.cpp
	${PROJECT_SOURCE_DIR}/src/common/ui/serialization.cpp
	${PROJECT_SOURCE_DIR}/src/common/ui/connection.cpp
	colour_log_formatter.cpp
	colors.cpp
	popup_test_main.cpp
	popup_runner_test.cpp
	)

ADD_EXECUTABLE(${TARGET_TPKP_POPUP_TEST} ${TEST_POPUP_SRCS})

TARGET_LINK_LIBRARIES(${TARGET_TPKP_POPUP_TEST}
	${TEST_TPKP_POPUP_DEP_LIBRARIES}
	boost_unit_test_framework
	)

INSTALL(TARGETS ${TARGET_TPKP_TEST} ${TARGET_TPKP_POPUP_TEST} DESTINATION bin)
