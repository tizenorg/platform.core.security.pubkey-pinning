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
 * @file        tpkp_client_cache.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning client cache declaration.
 */
#pragma once

#include <sys/types.h>
#include <string>
#include <map>
#include <mutex>

#define EXPORT_API __attribute__((visibility("default")))

namespace TPKP {

class EXPORT_API ClientCache {
public:
	enum class Decision : int {
		UNKNOWN,
		ALLOWED,
		DENIED
	};

	ClientCache();
	virtual ~ClientCache();

	/* thread-specific url mapped */
	void setUrl(const std::string &url);
	std::string getUrl(void);
	void eraseUrl(void);
	void eraseUrlAll(void);

	/* thread-globally user decision mapped to hostname extracted from url */
	void setDecision(const std::string &url, Decision decision);
	Decision getDecision(const std::string &url);

private:
	struct DecisionStruct {
		Decision decision;
		DecisionStruct() : decision(Decision::UNKNOWN) {}
		DecisionStruct(Decision d) : decision(d) {}
	};

	std::map<pid_t, std::string> m_urls;
	std::mutex m_url_mutex;

	std::map<std::string, DecisionStruct> m_decisions;
	std::mutex m_decision_mutex;
};

}
