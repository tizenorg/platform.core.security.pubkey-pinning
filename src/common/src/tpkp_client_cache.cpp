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
 * @file        tpkp_client_cache.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Https Public Key Pinning client cache implementation.
 */
#include "tpkp_client_cache.h"

#include <string>
#include <map>
#include <sys/syscall.h>
#include <unistd.h>

#include "tpkp_parser.h"
#include "tpkp_logger.h"

namespace {

pid_t _getThreadId()
{
	return syscall(SYS_gettid);
}

}

namespace TPKP {

ClientCache::ClientCache() {}

ClientCache::~ClientCache() {}

void ClientCache::setUrl(const std::string &url)
{
	auto tid = _getThreadId();
	{
		std::lock_guard<std::mutex> lock(m_url_mutex);
		m_urls[tid] = url;
	}

	SLOGD("set url[%s] of thread id[%u]", url.c_str(), tid);
}

std::string ClientCache::getUrl(void)
{
	std::string url;

	auto tid = _getThreadId();
	{
		std::lock_guard<std::mutex> lock(m_url_mutex);
		url = m_urls[tid];
	}

	SLOGD("get url[%s] from thread id[%u]", url.c_str(), tid);

	return url;
}

void ClientCache::eraseUrl(void)
{
	auto tid = _getThreadId();
	{
		std::lock_guard<std::mutex> lock(m_url_mutex);
		m_urls.erase(tid);
	}

	SLOGD("erase url of mapped by thread id[%u]", tid);
}

void ClientCache::eraseUrlAll(void)
{
	m_urls.clear();

	SLOGD("erase all urls saved of client");
}

void ClientCache::setDecision(const std::string &url, ClientCache::Decision decision)
{
	auto hostname = Parser::extractHostname(url);

	{
		std::lock_guard<std::mutex> lock(m_decision_mutex);
		m_decisions[hostname] = ClientCache::DecisionStruct(decision);
	}
}

ClientCache::Decision ClientCache::getDecision(const std::string &url)
{
	ClientCache::Decision decision;
	auto hostname = Parser::extractHostname(url);

	{
		std::lock_guard<std::mutex> lock(m_decision_mutex);
		decision = m_decisions[hostname].decision;
	}

	return decision;
}

}
