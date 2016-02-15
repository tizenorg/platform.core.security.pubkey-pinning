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
 * @file        popup_runner.cpp
 * @author      Janusz Kozerski (j.kozerski@samsung.com)
 * @version     1.0
 */
#include "ui/popup_runner.h"

#include "tpkp_common.h"
#include "tpkp_logger.h"
#include "ui/popup_common.h"
#include "ui/connection.h"

namespace TPKP {
namespace UI {

namespace {

struct TpkpPopupParent {
	std::string hostname;
	Response response;
};

/*
 *  parent send list
 *  - std::string hostname
 */
BinaryStream serialize(TpkpPopupParent *pdp)
{
	BinaryStream stream;
	Serialization::Serialize(stream, pdp->hostname);

	return stream;
}

/*
 *  parent receive list
 *  - TPKP::UI::Response response (int)
 */
void deserialize(TpkpPopupParent *pdp, BinaryStream &stream)
{
	int responseInt;
	Deserialization::Deserialize(stream, responseInt);
	pdp->response = static_cast<Response>(responseInt);
}

} // anonymous namespace

Response runPopup(const std::string &hostname, int timeout) noexcept
{
	(void)timeout;
	try {

		SLOGD("hostname: %s", hostname.c_str());

		TpkpPopupParent pd;
		TpkpPopupParent *pdp = &pd;

		pdp->hostname = hostname;

		BinaryStream inStream = serialize(pdp);

		ServiceConnection connection(TPKP_UI_SOCK_ADDR);
		BinaryStream outStream = connection.processRequest(inStream);

		deserialize(pdp, outStream);

		return pdp->response;

	} catch (const TPKP::Exception &e) {
		SLOGE("Exception[%d]: %s", e.code(), e.what());
		return Response::ERROR;
	} catch (const std::bad_alloc &e) {
		SLOGE("bad_alloc std exception: %s", e.what());
		return Response::ERROR;
	} catch (const std::exception &e) {
		SLOGE("std exception: %s", e.what());
		return Response::ERROR;
	} catch (...) {
		SLOGE("Unhandled exception occured!");
		return Response::ERROR;
	}

	SLOGE("This should not happen!!!");
	return Response::ERROR;
}

} // UI
} // TPKP
