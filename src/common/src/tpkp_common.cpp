/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @file        tpkp_common.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Https Public Key Pinning common implementation.
 */

#include <sys/syscall.h>
#include <unistd.h>

#include <cstring>
#include <cctype>
#include <new>
#include <algorithm>
#include <iostream>

#include "net/http/transport_security_state.h"
#include "net/http/transport_security_state_static.h"
#include "url/third_party/mozilla/url_parse.h"

#include "tpkp_common.h"

namespace {

template <typename T>
inline size_t _arraySize(const T &t)
{
	return sizeof(t) / sizeof(*t);
}

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

} // anonymous namespace

namespace TPKP {

pid_t getThreadId()
{
	return syscall(SYS_gettid);
}

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

class Context::Impl {
public:
	Impl() = delete;
	virtual ~Impl();
	explicit Impl(const std::string &url);

	void addPubkeyHash(HashAlgo algo, const RawBuffer &hashBuf);
	bool checkPubkeyPins(void);
	bool hasPins(void);

private:
	std::string m_url;
	std::string m_host;
	net::PreloadResult m_preloaded;
	HashValueVector m_hashes;

	bool LoadPreloadedPins(void);
	bool HashesIntersect(const char *const *hashesArr);

	class HashValuesEqual {
	public:
		explicit HashValuesEqual(const char *chash);
		bool operator()(const HashValue &other) const;
	private:
		const char *m_chash;
	};
};

Context::Impl::Impl(const std::string &url) : m_url(url)
{
	url::Parsed parsed;
	url::ParseStandardURL(m_url.c_str(), m_url.length(), &parsed);
	TPKP_CHECK_THROW_EXCEPTION(parsed.host.is_valid(),
		TPKP_E_INVALID_URL, "Failed to parse url: " << url);

	m_host = _toLower(m_url.substr(
			static_cast<size_t>(parsed.host.begin),
			static_cast<size_t>(parsed.host.len)));

	SLOGD("HPKP ready to check on host[%s]", m_host.c_str());

	LoadPreloadedPins();
	if (!m_preloaded.has_pins) {
		SLOGD("no pins on static pubkey list.");
		return;
	}
}

Context::Impl::~Impl() {}

void Context::Impl::addPubkeyHash(HashAlgo algo, const RawBuffer &hashBuf)
{
	m_hashes.emplace_back(algo, hashBuf);
}

bool Context::Impl::checkPubkeyPins(void)
{
	if (!hasPins()) {
		SLOGD("no pins on static pubkey list.");
		return true;
	}

	const Pinset &pinset = kPinsets[m_preloaded.pinset_id];

	if (HashesIntersect(pinset.rejected_pins)) {
		SLOGE("pubkey is in rejected pin!");
		return false;
	}

	if (!HashesIntersect(pinset.accepted_pins)) {
		SLOGE("pubkey cannot be found in accepted pins!");
		return false;
	}

	SLOGD("pubkey is pinned one!");

	return true;
}

bool Context::Impl::hasPins(void)
{
	return m_preloaded.has_pins;
}

bool Context::Impl::LoadPreloadedPins(void)
{
	m_preloaded.has_pins = false;
	if (!net::DecodeHSTSPreload(m_host, &m_preloaded))
		return false;

	size_t arrsize = _arraySize(kPinsets);
	if (m_preloaded.pinset_id >= static_cast<uint32>(arrsize))
		return false;

	return true;
}

bool Context::Impl::HashesIntersect(const char *const *hashesArr)
{
	if (!hashesArr)
		return false;

	for (; *hashesArr != nullptr; hashesArr++) {
		if (std::find_if(
				m_hashes.begin(),
				m_hashes.end(),
				HashValuesEqual{*hashesArr}) != m_hashes.end()) {
			SLOGD("hash intersect found!");
			return true;
		}
	}

	return false;
}

Context::Impl::HashValuesEqual::HashValuesEqual(const char *chash) : m_chash(chash) {}

bool Context::Impl::HashValuesEqual::operator()(const HashValue &other) const
{
	size_t len = other.hash.size();

	/*
	 * hash from decode preloaded value is base64 encoded,
	 * so it can be get length by strlen.
	 */
	if (strlen(m_chash) != len)
		return false;

	for (size_t i = 0; i < len; i++) {
		if (m_chash[i] != other.hash[i])
			return false;
	}

	return true;
}

Context::Context(const std::string &url) : pImpl(new Impl{url}) {}
Context::~Context() {}

void Context::addPubkeyHash(HashAlgo algo, const RawBuffer &hashBuf)
{
	SLOGD("add public key hash of algo[%d]", algo);
	pImpl->addPubkeyHash(algo, hashBuf);
}

bool Context::checkPubkeyPins(void)
{
	return pImpl->checkPubkeyPins();
}

bool Context::hasPins(void)
{
	return pImpl->hasPins();
}

}