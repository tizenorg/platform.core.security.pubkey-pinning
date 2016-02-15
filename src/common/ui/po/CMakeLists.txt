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
# @file        CMakeLists.txt
# @author      Adam Malinowski <a.malinowsk2@partner.samsung.com>
#

SET(POFILES
	en.po
	en_GB.po
	en_US.po
	)

SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(SHAREDIR "${PREFIX}/share")
SET(LOCALEDIR "${SHAREDIR}/locale")
SET(MSGFMT "${PREFIX}/bin/msgfmt")

FOREACH(POFILE ${POFILES})
	SET(POFILE ${CMAKE_CURRENT_SOURCE_DIR}/${POFILE})
	MESSAGE("PO: ${POFILE}")

	GET_FILENAME_COMPONENT(ABS_POFILE ${POFILE} ABSOLUTE)
	GET_FILENAME_COMPONENT(LANG ${ABS_POFILE} NAME_WE)

	SET(MOFILE ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.mo)

	ADD_CUSTOM_COMMAND(
		OUTPUT ${MOFILE}
		COMMAND ${MSGFMT} -o ${MOFILE} ${ABS_POFILE}
		DEPENDS ${ABS_POFILE}
	)

	INSTALL(FILES ${MOFILE}
		DESTINATION ${LOCALEDIR}/${LANG}/LC_MESSAGES RENAME ${PROJECT_NAME}.mo)

	SET(MOFILES ${MOFILES} ${MOFILE})
ENDFOREACH(POFILE)

ADD_CUSTOM_TARGET(po ALL DEPENDS ${MOFILES})
