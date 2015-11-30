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
 * @file        tpkp_curl.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning implementation for libcurl.
 */
#include <string>
#include <memory>
#include <map>
#include <mutex>

#include <curl/curl.h>

#include "tpkp_common.h"
#include "tpkp_curl.h"

namespace {

std::map<pid_t, std::string> s_urlmap;
std::mutex s_mutex;

inline CURLcode err_tpkp_to_curle(tpkp_e err) noexcept
{
	switch (err) {
	case TPKP_E_NONE:                    return CURLE_OK;
	case TPKP_E_MEMORY:                  return CURLE_OUT_OF_MEMORY;
	case TPKP_E_INVALID_URL:             return CURLE_URL_MALFORMAT;
	case TPKP_E_NO_URL_DATA:             return CURLE_SSL_CERTPROBLEM;
	case TPKP_E_PUBKEY_MISMATCH:         return CURLE_SSL_PINNEDPUBKEYNOTMATCH;
	case TPKP_E_INVALID_CERT:
	case TPKP_E_INVALID_PEER_CERT_CHAIN:
	case TPKP_E_FAILED_GET_PUBKEY_HASH:  return CURLE_PEER_FAILED_VERIFICATION;
	case TPKP_E_STD_EXCEPTION:
	case TPKP_E_INTERNAL:
	default:                             return CURLE_UNKNOWN_OPTION;
	}
}

TPKP::RawBuffer getPubkeyHash(X509 *cert, TPKP::HashAlgo algo)
{
	std::unique_ptr<EVP_PKEY, void(*)(EVP_PKEY *)>
		pubkeyPtr(X509_get_pubkey(cert), EVP_PKEY_free);

	TPKP_CHECK_THROW_EXCEPTION(pubkeyPtr,
		TPKP_E_INVALID_CERT, "Failed to get pubkey from cert.");

	unsigned char *der = nullptr;
	auto len = i2d_PUBKEY(pubkeyPtr.get(), &der);
	TPKP_CHECK_THROW_EXCEPTION(len > 0,
		TPKP_E_INVALID_CERT, "Failed to convert pem pubkey to der.");

	TPKP::RawBuffer pubkeyder(der, der + len);
	free(der);
	unsigned char *hashResult = nullptr;
	TPKP::RawBuffer out;
	switch (algo) {
	case TPKP::HashAlgo::SHA1:
		out.resize(SHA_DIGEST_LENGTH);
		hashResult = SHA1(pubkeyder.data(), pubkeyder.size(), out.data());

		break;

	default:
		TPKP_CHECK_THROW_EXCEPTION(false,
			TPKP_E_INTERNAL, "Invalid hash algo type in get_pubkey_hash");
	}

	TPKP_CHECK_THROW_EXCEPTION(hashResult,
		TPKP_E_FAILED_GET_PUBKEY_HASH, "Failed to get pubkey haso by openssl SHA1.");

	return out;
}

} // anonymous namespace


EXPORT_API
int tpkp_curl_verify_callback(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
	tpkp_e res = TPKP::ExceptionSafe([&]{
		TPKP_CHECK_THROW_EXCEPTION(preverify_ok != 0,
			TPKP_E_INTERNAL, "verify callback already failed before enter tpkp_curl callback");

		auto tid = TPKP::getThreadId();
		std::string url;

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			url = s_urlmap[tid];
		}

		TPKP_CHECK_THROW_EXCEPTION(!url.empty(),
			TPKP_E_NO_URL_DATA, "No url for thread id[" << tid << "] in map");

		SLOGD("get url[%s] of thread id[%u]", url.c_str(), tid);

		TPKP::Context ctx(url);
		if (!ctx.hasPins()) {
			SLOGI("Skip. No static pin data for url: %s", url.c_str());
			return;
		}

		auto chain = X509_STORE_CTX_get1_chain(x509_ctx);
		int num = sk_X509_num(chain);
		TPKP_CHECK_THROW_EXCEPTION(num != -1,
			TPKP_E_INVALID_PEER_CERT_CHAIN,
			"Invalid cert chain from x509_ctx in verify callback.");

		for (int i = 0; i < num; i++)
			ctx.addPubkeyHash(
				TPKP::HashAlgo::SHA1,
				getPubkeyHash(sk_X509_value(chain, i), TPKP::HashAlgo::SHA1));

		TPKP_CHECK_THROW_EXCEPTION(ctx.checkPubkeyPins(),
			TPKP_E_PUBKEY_MISMATCH, "The pubkey mismatched with pinned data!");
	});

	return (res == TPKP_E_NONE) ? 1 : 0;
}

EXPORT_API
tpkp_e tpkp_curl_set_url_data(CURL *curl)
{
	return TPKP::ExceptionSafe([&]{
		char *url = nullptr;
		curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);

		auto tid = TPKP::getThreadId();

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_urlmap[tid] = url;
		}

		SLOGD("set url[%s] of thread id[%u]", url, tid);
	});
}

EXPORT_API
tpkp_e tpkp_curl_set_verify(CURL *curl, SSL_CTX *ssl_ctx)
{
	SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, tpkp_curl_verify_callback);
	return tpkp_curl_set_url_data(curl);
}

EXPORT_API
CURLcode tpkp_curl_ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *)
{
	return err_tpkp_to_curle(tpkp_curl_set_verify(curl, (SSL_CTX *)ssl_ctx));
}

EXPORT_API
void tpkp_curl_cleanup(void)
{
	tpkp_e res = TPKP::ExceptionSafe([&]{
		auto tid = TPKP::getThreadId();

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_urlmap.erase(tid);
		}

		SLOGD("cleanup url data for thread id[%u]", tid);
	});

	(void) res;
}

EXPORT_API
void tpkp_curl_cleanup_all(void)
{
	s_urlmap.clear();
}
