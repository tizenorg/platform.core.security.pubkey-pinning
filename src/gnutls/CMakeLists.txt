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
# @brief    TPKP gnutls lib makefile
#
PKG_CHECK_MODULES(TPKP_GNUTLS_DEP
	REQUIRED
	gnutls
	dlog
	)

INCLUDE_DIRECTORIES(
	SYSTEM
	include
	${PROJECT_SOURCE_DIR}/src/common/include # common library interface header
	${TPKP_GNUTLS_DEP_INCLUDE_DIRS}
	)

SET(TPKP_GNUTLS_SRCS
	tpkp_gnutls.cpp
	)

ADD_LIBRARY(${TARGET_TPKP_GNUTLS_LIB} SHARED ${TPKP_GNUTLS_SRCS})

SET_TARGET_PROPERTIES(${TARGET_TPKP_GNUTLS_LIB}
	PROPERTIES
		COMPILE_FLAGS "-D_GNU_SOURCE -fPIC -fvisibility=hidden"
		SOVERSION ${SO_VERSION}
		VERSION ${VERSION}
	)

TARGET_LINK_LIBRARIES(${TARGET_TPKP_GNUTLS_LIB}
	${TARGET_TPKP_COMMON_LIB}
	${TPKP_GNUTLS_DEP_LIBRARIES}
	)

INSTALL(TARGETS ${TARGET_TPKP_GNUTLS_LIB} DESTINATION ${LIB_INSTALL_DIR})

INSTALL(FILES include/tpkp_gnutls.h DESTINATION ${INCLUDEDIR}/tpkp/gnutls)
