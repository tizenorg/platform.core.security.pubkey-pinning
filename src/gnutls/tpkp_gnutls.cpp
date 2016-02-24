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
 * @file        tpkp_gnutls.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning implementation for gnutls.
 */
#include "tpkp_gnutls.h"

#include <string>
#include <memory>
#include <map>
#include <mutex>

#include <gnutls/gnutls.h>
#include <gnutls/abstract.h>
#include <gnutls/x509.h>

#include "tpkp_common.h"
#include "tpkp_logger.h"
#include "tpkp_client_cache.h"

namespace {

using Decision = TPKP::ClientCache::Decision;

TPKP::ClientCache g_cache;

inline int err_tpkp_to_gnutlse(tpkp_e err) noexcept
{
	switch (err) {
	case TPKP_E_NONE:                     return GNUTLS_E_SUCCESS;
	case TPKP_E_MEMORY:                   return GNUTLS_E_MEMORY_ERROR;
	case TPKP_E_INVALID_URL:              return GNUTLS_E_INVALID_SESSION;
	case TPKP_E_NO_URL_DATA:              return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	case TPKP_E_PUBKEY_MISMATCH:          return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
	case TPKP_E_INVALID_CERT:
	case TPKP_E_INVALID_PEER_CERT_CHAIN:
	case TPKP_E_FAILED_GET_PUBKEY_HASH:   return GNUTLS_E_PK_SIG_VERIFY_FAILED;
	case TPKP_E_CERT_VERIFICATION_FAILED: return GNUTLS_E_CERTIFICATE_ERROR;
	case TPKP_E_STD_EXCEPTION:
	case TPKP_E_INTERNAL:
	default:                              return GNUTLS_E_INTERNAL_ERROR;
	}
}

using GnutlsX509Ptr = std::unique_ptr<gnutls_x509_crt_t, void(*)(gnutls_x509_crt_t *)>;
inline GnutlsX509Ptr createGnutlsX509Ptr(void)
{
	return GnutlsX509Ptr(new gnutls_x509_crt_t, [](gnutls_x509_crt_t *ptr) {
		if (!!ptr) gnutls_x509_crt_deinit(*ptr);
	});
}

TPKP::RawBuffer getPubkeyHash(gnutls_x509_crt_t cert, TPKP::HashAlgo algo)
{
	std::unique_ptr<gnutls_pubkey_t, void(*)(gnutls_pubkey_t *)>
		pubkeyPtr(new gnutls_pubkey_t, [](gnutls_pubkey_t *ptr)->void
			{
				if (ptr != nullptr)
					gnutls_pubkey_deinit(*ptr);
			});

	int ret = gnutls_pubkey_init(pubkeyPtr.get());
	TPKP_CHECK_THROW_EXCEPTION(ret == GNUTLS_E_SUCCESS,
		TPKP_E_INTERNAL,
		"Failed to gnutls_pubkey_init. gnutls ret: " << ret);

	ret = gnutls_pubkey_import_x509(*pubkeyPtr, cert, 0);
	TPKP_CHECK_THROW_EXCEPTION(ret == GNUTLS_E_SUCCESS,
		TPKP_E_INVALID_CERT,
		"Failed to gnutls_pubkey_import_x509. gnutls ret: " << ret);

	size_t len = 0;
	ret = gnutls_pubkey_export(*pubkeyPtr, GNUTLS_X509_FMT_DER, nullptr, &len);
	TPKP_CHECK_THROW_EXCEPTION(
		(ret == GNUTLS_E_SHORT_MEMORY_BUFFER || ret == GNUTLS_E_SUCCESS) && len != 0,
		TPKP_E_INVALID_CERT,
		"Failed to gnutls_pubkey_export for getting size. gnutls ret: "
			<< ret << " desc: " << gnutls_strerror(ret) << " size: " << len);

	TPKP::RawBuffer derbuf(len, 0x00);
	ret = gnutls_pubkey_export(*pubkeyPtr, GNUTLS_X509_FMT_DER, derbuf.data(), &len);
	TPKP_CHECK_THROW_EXCEPTION(ret == GNUTLS_E_SUCCESS && len == derbuf.size(),
		TPKP_E_INVALID_CERT,
		"Failed to gnutls_pubkey_export. gnutls ret: "
			<< ret << " desc: " << gnutls_strerror(ret));

	gnutls_datum_t pubkeyder = {
		derbuf.data(),
		static_cast<unsigned int>(derbuf.size())
	};

	auto gnutlsHashAlgo = GNUTLS_DIG_SHA1; /* default hash alog */
	TPKP::RawBuffer out;
	switch (algo) {
	case TPKP::HashAlgo::SHA1:
		out.resize(TPKP::typeCast(TPKP::HashSize::SHA1), 0x00);
		len = out.size();
		gnutlsHashAlgo = GNUTLS_DIG_SHA1;
		break;

	default:
		TPKP_CHECK_THROW_EXCEPTION(
			false,
			TPKP_E_INTERNAL,
			"Invalid hash algo type in getPubkeyHash.");
	}

	ret = gnutls_fingerprint(gnutlsHashAlgo, &pubkeyder, out.data(), &len);
	TPKP_CHECK_THROW_EXCEPTION(ret == GNUTLS_E_SUCCESS && len == out.size(),
		TPKP_E_FAILED_GET_PUBKEY_HASH,
		"Failed to gnutls_fingerprint. gnutls ret: "
			<< ret << " desc: " << gnutls_strerror(ret));

	return out;
}

GnutlsX509Ptr d2iCert(const gnutls_datum_t *datum)
{
	auto crtPtr = createGnutlsX509Ptr();

	TPKP_CHECK_THROW_EXCEPTION(
		gnutls_x509_crt_init(crtPtr.get()) == GNUTLS_E_SUCCESS,
		TPKP_E_INTERNAL, "Failed to gnutls_x509_crt_init.");
	TPKP_CHECK_THROW_EXCEPTION(
		gnutls_x509_crt_import(*crtPtr, datum, GNUTLS_X509_FMT_DER) >= 0,
		TPKP_E_INTERNAL, "Failed to import DER to gnutls crt");

	return crtPtr;
}

/*
 *  Need not to gnutls_x509_crt_deinit for returned value unless GNUTLS_TL_GET_COPY
 *  flag is used.
 *  Refer API description of gnutls_certificate_get_issuer.
 *
 *  gnutls_certificate_get_issuer will return the issuer of a given certificate.
 *  As with gnutls_x509_trust_list_get_issuer() this functions requires the
 *  GNUTLS_TL_GET_COPY flag in order to operate with PKCS11 trust list. In
 *  that case the issuer must be freed using gnutls_x509_crt_init().
 */
gnutls_x509_crt_t getIssuer(gnutls_session_t session, gnutls_x509_crt_t cert)
{
	gnutls_certificate_credentials_t cred;
	TPKP_CHECK_THROW_EXCEPTION(
		gnutls_credentials_get(session, GNUTLS_CRD_CERTIFICATE, (void **)&cred)
			== GNUTLS_E_SUCCESS,
		TPKP_E_INTERNAL, "Failed to get credential on session");

	gnutls_x509_crt_t issuer;
	TPKP_CHECK_THROW_EXCEPTION(
		gnutls_x509_crt_init(&issuer) == GNUTLS_E_SUCCESS,
		TPKP_E_INTERNAL, "Failed to gnutls_x509_crt_init");

	TPKP_CHECK_THROW_EXCEPTION(
		gnutls_certificate_get_issuer(cred, cert, &issuer, 0) == GNUTLS_E_SUCCESS,
		TPKP_E_INTERNAL,
		"Failed to get issuer! It's internal error because verify peer2 success already");

	return issuer;
}

}

EXPORT_API
int tpkp_gnutls_verify_callback(gnutls_session_t session)
{
	tpkp_e res = TPKP::ExceptionSafe([&]{
		gnutls_certificate_type_t type = gnutls_certificate_type_get(session);
		if (type != GNUTLS_CRT_X509) {
			/*
			 * TODO: what should we do if it's not x509 type cert?
			 * for now, just return 0 which means verification success
			 */
			SLOGW("Certificate type of session isn't X509. skipt for now...");
			return;
		}

		unsigned int status = 0;
		int res = gnutls_certificate_verify_peers2(session, &status);
		TPKP_CHECK_THROW_EXCEPTION(res == GNUTLS_E_SUCCESS,
			TPKP_E_CERT_VERIFICATION_FAILED,
			"Failed to certificate verify peers2.. res: " << gnutls_strerror(res));

		TPKP_CHECK_THROW_EXCEPTION(status == 0,
			TPKP_E_CERT_VERIFICATION_FAILED,
			"Peer certificate verification failed!! status: " << status);

		std::string url = g_cache.getUrl();

		TPKP_CHECK_THROW_EXCEPTION(
			!url.empty(),
			TPKP_E_NO_URL_DATA,
			"No url of found in client cache!!");

		switch (g_cache.getDecision(url)) {
		case Decision::ALLOWED:
			SLOGD("allow decision exist on url[%s]", url.c_str());
			return;

		case Decision::DENIED:
			TPKP_THROW_EXCEPTION(TPKP_E_PUBKEY_MISMATCH,
				"deny decision exist on url: " << url);

		default:
			break; /* go ahead to make decision */
		}

		TPKP::Context ctx(url);
		if (!ctx.hasPins()) {
			SLOGI("Skip. No static pin data for url: %s", url.c_str());
			return;
		}

		unsigned int listSize = 0;
		const gnutls_datum_t *certChain = gnutls_certificate_get_peers(session, &listSize);
		TPKP_CHECK_THROW_EXCEPTION(certChain != nullptr && listSize != 0,
			TPKP_E_INVALID_PEER_CERT_CHAIN,
			"no certificate from peer!");

		for (unsigned int i = 0; i < listSize; i++) {
			auto crtPtr = d2iCert(certChain++);

			ctx.addPubkeyHash(
				TPKP::HashAlgo::SHA1,
				getPubkeyHash(*crtPtr, TPKP::HashAlgo::SHA1));

			/* add additional root CA cert for last one */
			if (i == listSize - 1) {
				auto issuer = getIssuer(session, *crtPtr);

				ctx.addPubkeyHash(
					TPKP::HashAlgo::SHA1,
					getPubkeyHash(issuer, TPKP::HashAlgo::SHA1));
			}
		}

		bool isMatched = ctx.checkPubkeyPins();

		/* update decision cache */
		g_cache.setDecision(url, isMatched ? Decision::ALLOWED : Decision::DENIED);

		TPKP_CHECK_THROW_EXCEPTION(isMatched,
			TPKP_E_PUBKEY_MISMATCH, "THe pubkey mismatched with pinned data!");
	});

	return err_tpkp_to_gnutlse(res);
}

EXPORT_API
tpkp_e tpkp_gnutls_set_url_data(const char *url)
{
	return TPKP::ExceptionSafe([&]{
		g_cache.setUrl(url);
	});
}

EXPORT_API
void tpkp_gnutls_cleanup(void)
{
	tpkp_e res = TPKP::ExceptionSafe([&]{
		g_cache.eraseUrl();
	});

	(void) res;
}

EXPORT_API
void tpkp_gnutls_cleanup_all(void)
{
	g_cache.eraseUrlAll();
}
