# Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
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
#
# @file     CMakeLists.txt
# @author   Janusz Kozerski (j.kozerski@samsung.com)
# @version  1.0
#
PKG_CHECK_MODULES(TPKP_POPUP_DEP
	REQUIRED
	elementary
	libsystemd-daemon
	vconf
	)

INCLUDE_DIRECTORIES(
	${TPKP_POPUP_DEP_INCLUDE_DIRS}
	.
	${PROJECT_SOURCE_DIR}/src/common/include
	)

SET(POPUP_SRCS
	popup.cpp
	)

SET_SOURCE_FILES_PROPERTIES(${POPUP_SRCS}
	PROPERTIES
		COMPILE_FLAGS "-D_GNU_SOURCE -fPIE"
		LINK_FLAGS "-pie"
	)

ADD_EXECUTABLE(${TARGET_TPKP_POPUP} ${POPUP_SRCS})

TARGET_LINK_LIBRARIES(${TARGET_TPKP_POPUP}
	${TPKP_POPUP_DEP_LIBRARIES}
	${TARGET_TPKP_COMMON_LIB}
	)

INSTALL(TARGETS ${TARGET_TPKP_POPUP} DESTINATION bin)
