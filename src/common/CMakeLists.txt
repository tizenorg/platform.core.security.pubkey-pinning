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
# @brief    TPKP common lib makefile
#
PKG_CHECK_MODULES(TPKP_COMMON_DEP
	REQUIRED
	dlog
	openssl
	)

INCLUDE_DIRECTORIES(
	SYSTEM
	${TPKP_COMMON_DEP_INCLUDE_DIRS}
	.         # common internal headers
	include # common interface headers
	)

SET(TPKP_COMMON_SRCS
	url/url_constants.cc
	url/url_util.cc
	url/url_parse.cc
	url/url_parse_file.cc
	net/http/transport_security_state.cpp
	src/tpkp_common.cpp
	)

ADD_LIBRARY(${TARGET_TPKP_COMMON_LIB} SHARED ${TPKP_COMMON_SRCS})

SET_TARGET_PROPERTIES(${TARGET_TPKP_COMMON_LIB}
	PROPERTIES
		COMPILE_FLAGS "-D_GNU_SOURCE -fPIC -fvisibility=hidden"
		SOVERSION ${SO_VERSION}
		VERSION ${VERSION}
	)

TARGET_LINK_LIBRARIES(${TARGET_TPKP_COMMON_LIB} ${TPKP_COMMON_DEP_LIBRARIES})

INSTALL(TARGETS ${TARGET_TPKP_COMMON_LIB} DESTINATION ${LIB_INSTALL_DIR})
INSTALL(FILES include/tpkp_error.h DESTINATION ${INCLUDEDIR}/tpkp/common)
