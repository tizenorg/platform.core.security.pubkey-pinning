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
#include "tpkp_common.h"

#include <cstring>
#include <cctype>
#include <new>
#include <algorithm>
#include <iostream>

#include "net/http/transport_security_state.h"
#include "net/http/transport_security_state_static.h"

#include "tpkp_logger.h"
#include "tpkp_parser.h"
#include "ui/popup_runner.h"

namespace {

template <typename T>
inline size_t _arraySize(const T &t)
{
	return sizeof(t) / sizeof(*t);
}

} // anonymous namespace

namespace TPKP {

class Context::Impl {
public:
	Impl() = delete;
	virtual ~Impl();
	explicit Impl(const std::string &url);

	void addPubkeyHash(HashAlgo algo, const RawBuffer &hashBuf);
	bool checkPubkeyPins(void);
	bool hasPins(void);

private:
	std::string m_host;
	net::PreloadResult m_preloaded;
	HashValueVector m_hashes;

	bool LoadPreloadedPins(void);
	bool HashesIntersect(const char *const *hashesArr);
	bool askUser(void);

	class HashValuesEqual {
	public:
		explicit HashValuesEqual(const char *chash);
		bool operator()(const HashValue &other) const;
	private:
		const char *m_chash;
	};
};

Context::Impl::Impl(const std::string &url)
{
	m_host = Parser::extractHostname(url);

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
		return askUser();
	}

	if (!HashesIntersect(pinset.accepted_pins)) {
		SLOGE("pubkey cannot be found in accepted pins!");
		return askUser();
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

bool Context::Impl::askUser(void)
{
	TPKP::UI::Response response = TPKP::UI::runPopup(m_host);

	switch (response) {
	case TPKP::UI::Response::ALLOW:
		SLOGI("ALLOW returned from tpkp-popup");
		return true;
	case TPKP::UI::Response::DENY:
		SLOGI("DENY returned from tpkp-popup");
		return false;
	default:
		SLOGE("Unknown response returned[%d] from tpkp-popup", static_cast<int>(response));
		return false;
	}
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
