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
 * @file        tpkp_exception.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Exceptions for internal use.
 */
#pragma once

#include <sstream>
#include <string>
#include <exception>
#include <functional>

#include "tpkp_error.h"

#define EXPORT_API __attribute__((visibility("default")))

namespace TPKP {

EXPORT_API
tpkp_e ExceptionSafe(const std::function<void()> &func) noexcept;

class EXPORT_API Exception : public std::exception {
public:
	explicit Exception(tpkp_e code, const std::string &message);
	virtual const char *what(void) const noexcept;
	tpkp_e code(void) const noexcept;

private:
	tpkp_e m_code;
	std::string m_message;
};

}

#define TPKP_THROW_EXCEPTION(code, message) \
do {                                        \
	std::ostringstream log;                 \
	log << message;                         \
	throw TPKP::Exception(code, log.str()); \
} while(false)

#define TPKP_CHECK_THROW_EXCEPTION(cond, code, message) \
	if (!(cond)) TPKP_THROW_EXCEPTION(code, message)
