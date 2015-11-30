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

INSTALL(TARGETS ${TARGET_TPKP_TEST} DESTINATION bin)
