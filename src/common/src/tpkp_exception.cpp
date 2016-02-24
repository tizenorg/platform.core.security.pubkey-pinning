/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
/*
 * @file        tpkp_exception.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Exceptions for internal uses
 */
#include "tpkp_exception.h"

#include "tpkp_logger.h"

namespace TPKP {

Exception::Exception(tpkp_e code, const std::string &message)
	: m_code(code)
	, m_message(message)
{}

const char *Exception::what(void) const noexcept
{
	return m_message.c_str();
}

tpkp_e Exception::code(void) const noexcept
{
	return m_code;
}

tpkp_e ExceptionSafe(const std::function<void()> &func)
{
	try {
		func();
		return TPKP_E_NONE;
	} catch (const Exception &e) {
		SLOGE("Exception: %s", e.what());
		return e.code();
	} catch (const std::bad_alloc &e) {
		SLOGE("bad_alloc std exception: %s", e.what());
		return TPKP_E_MEMORY;
	} catch (const std::exception &e) {
		SLOGE("std exception: %s", e.what());
		return TPKP_E_STD_EXCEPTION;
	} catch (...) {
		SLOGE("Unhandled exception occured!");
		return TPKP_E_INTERNAL;
	}
}

}
