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
 * @file        tpkp_parser.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning url parser.
 */
#include "tpkp_parser.h"

#include <algorithm>

#include "url/third_party/mozilla/url_parse.h"
#include "tpkp_common.h"

namespace {

inline std::string _toLower(const std::string &host)
{
	std::string lowerHost;
	std::for_each(
		host.begin(),
		host.end(),
		[&lowerHost](const std::string::value_type &ch)
		{
			lowerHost.push_back(std::tolower(ch));
		});

	return lowerHost;
}

std::string prependHttps(const std::string &url)
{
	const std::string separator = "://";
	auto pos = url.find(separator);

	if (pos != std::string::npos)
		return url;

	return std::string("https") + separator + url;
}

}

namespace TPKP {

std::string Parser::extractHostname(const std::string &url)
{
	url::Parsed parsed;
	auto newUrl = prependHttps(url);

	url::ParseStandardURL(newUrl.c_str(), newUrl.length(), &parsed);
	TPKP_CHECK_THROW_EXCEPTION(parsed.host.is_valid(),
		TPKP_E_INVALID_URL, "Failed to parse url: " << newUrl);

	std::string hostname = _toLower(newUrl.substr(
			static_cast<size_t>(parsed.host.begin),
			static_cast<size_t>(parsed.host.len)));

	return hostname;
}

}
