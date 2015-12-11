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
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include <string>
#include <memory>
#include <map>
#include <mutex>

#include <gnutls/gnutls.h>

#include <openssl/pem.h>
#include <openssl/x509.h>

#include "tpkp_common.h"
#include "tpkp_gnutls.h"

namespace {

std::map<pid_t, std::string> s_urlmap;
std::mutex s_mutex;

inline int err_tpkp_to_gnutlse(tpkp_e err) noexcept
{
	switch (err) {
	case TPKP_E_NONE:                    return GNUTLS_E_SUCCESS;
	case TPKP_E_MEMORY:                  return GNUTLS_E_MEMORY_ERROR;
	case TPKP_E_INVALID_URL:             return GNUTLS_E_INVALID_SESSION;
	case TPKP_E_NO_URL_DATA:             return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
	case TPKP_E_PUBKEY_MISMATCH:         return GNUTLS_E_CERTIFICATE_KEY_MISMATCH;
	case TPKP_E_INVALID_CERT:
	case TPKP_E_INVALID_PEER_CERT_CHAIN:
	case TPKP_E_FAILED_GET_PUBKEY_HASH:  return GNUTLS_E_PK_SIG_VERIFY_FAILED;
	case TPKP_E_STD_EXCEPTION:
	case TPKP_E_INTERNAL:
	default:                             return GNUTLS_E_INTERNAL_ERROR;
	}
}

bool _verifyCert(X509 *subject, X509 *issuer)
{
	std::unique_ptr<EVP_PKEY, void(*)(EVP_PKEY *)> ppubkey(X509_get_pubkey(issuer), EVP_PKEY_free);
	TPKP_CHECK_THROW_EXCEPTION(!!ppubkey, TPKP_E_INTERNAL, "Failed to get pubkey from issuer cert");

	return X509_verify(subject, ppubkey.get()) == 1;
}

/* only support PEM format */
TPKP::X509Ptr _loadCert(const std::string &path)
{
	std::unique_ptr<FILE, int(*)(FILE *)> fp(fopen(path.c_str(), "rb"), fclose);
	TPKP_CHECK_THROW_EXCEPTION(!!fp, TPKP_E_INTERNAL, "cannot open system cert: " << path);

	TPKP::X509Ptr xp(PEM_read_X509(fp.get(), nullptr, nullptr, nullptr), X509_free);
	if (!xp) {
		rewind(fp.get());
		xp.reset(PEM_read_X509_AUX(fp.get(), nullptr, nullptr, nullptr));
	}

	TPKP_CHECK_THROW_EXCEPTION(!!xp, TPKP_E_INTERNAL, "cannot read x509 from system cert: " << path);

	return xp;
}

std::string _getIssuerHash(X509 *x)
{
	unsigned long ulhash = X509_issuer_name_hash(x);
	char tmp[9] = {0};
	snprintf(tmp, 9, "%08lx", ulhash);

	return std::string(tmp);
}

bool _searchIssuer(const std::string &dir, X509 *x, TPKP::CertDer &out)
{
	std::string issuerHash = _getIssuerHash(x);
	SLOGD("issuer hash to find: %s", issuerHash.c_str());

	std::unique_ptr<DIR, int(*)(DIR *)> pdir(opendir(dir.c_str()), closedir);
	TPKP_CHECK_THROW_EXCEPTION(!!pdir, TPKP_E_INTERNAL,
		"Failed to open system cert dir: " << dir);

	std::unique_ptr<struct dirent> pentry(new struct dirent);

	struct dirent *pdirent = nullptr;
	while (readdir_r(pdir.get(), pentry.get(), &pdirent) == 0 && pdirent) {
		if (pdirent->d_type == DT_DIR)
			continue;

		/*
		 * trusted cert file name format is "(8 hexs of hash) + '.' + (numeric)"
		 * [0-9a-z]{8}\.[0-9]
		 * i.e.: 1234abcd.0
		 */
		auto name = pdirent->d_name;
		if (strlen(name) >= 10 && issuerHash.compare(0, 8, name, 8) != 0)
			continue;

		auto pcandidate = _loadCert(dir + name);
		if (_verifyCert(x, pcandidate.get())) {
			SLOGD("verify certificate success with candidate cert: %s%s", dir.c_str(), name);
			out = TPKP::i2dCert(pcandidate.get());
			return true;
		}
	}

	return false;
}

bool _completeCertChain(TPKP::CertDerChain &chain)
{
	auto px = TPKP::d2iCert(chain.back());
	if (X509_subject_name_hash(px.get()) == X509_issuer_name_hash(px.get())) {
		SLOGD("peer's cert chain is already completed.");
		return true;
	}

	TPKP::CertDer issuer;
	if (!_searchIssuer("/etc/ssl/certs/", px.get(), issuer)) {
		SLOGE("Failed to search root CA cert in system trusted cert store.");
		return false;
	}

	chain.emplace_back(std::move(issuer));

	return true;
}

std::string _verificationStatusToString(unsigned int status)
{
	gnutls_datum_t errdatum = {nullptr, 0};
	gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509, &errdatum, 0);
	std::string errstr = reinterpret_cast<char *>(errdatum.data);
	gnutls_free(errdatum.data);
	return errstr;
}

} /* anonymous namespace */

EXPORT_API
int tpkp_gnutls_certificate_verify_callback(gnutls_session_t session)
{
	tpkp_e res = TPKP::ExceptionSafe([&]{
		gnutls_certificate_type_t type = gnutls_certificate_type_get(session);
		if (type != GNUTLS_CRT_X509) {
			/*
			 * TODO: what should we do if it's not x509 type cert?
			 * for now, just return which means verification success
			 */
			SLOGW("Certificate type of session isn't X509. skipt for now...");
			return;
		}

		unsigned int status = 0;
		int res = gnutls_certificate_verify_peers2(session, &status);
		TPKP_CHECK_THROW_EXCEPTION(res == GNUTLS_E_SUCCESS && status == 0,
			TPKP_E_INVALID_PEER_CERT_CHAIN,
			"Failed to verify peer!"
				<< " res: " << res
				<< " desc: " << gnutls_strerror(res)
				<< " status: " << _verificationStatusToString(status));

		auto tid = TPKP::getThreadId();
		std::string url;

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			url = s_urlmap[tid];
		}

		TPKP_CHECK_THROW_EXCEPTION(
			!url.empty(),
			TPKP_E_NO_URL_DATA,
			"No url of thread id[" << tid << "]");

		SLOGD("get url[%s] of thread id[%u]", url.c_str(), tid);

		TPKP::Context ctx(url);
		if (!ctx.hasPins()) {
			SLOGI("Skip. No static pin data for url: %s", url.c_str());
			return;
		}

		unsigned int listSize = 0;
		/*
		 *  TODO: is gnuChain need not to be freed?
		 */
		auto gnuChain = gnutls_certificate_get_peers(session, &listSize);
		TPKP_CHECK_THROW_EXCEPTION(gnuChain != nullptr && listSize != 0,
			TPKP_E_INVALID_PEER_CERT_CHAIN,
			"no certificate from peer!");

		TPKP::CertDerChain chain;
		for (unsigned int i = 0; i < listSize; i++)
			chain.emplace_back(
				gnuChain[i].data, gnuChain[i].data + gnuChain[i].size);

		if (!_completeCertChain(chain)) {
			/*
			 *  TODO: what should we do if peer's root CA cert of cert chain
			 *        isn't exist in system trusted cert store?
			 *        Should we just pass or terminate...
			 *        for now, just return which means verification success
			 */
			SLOGW("Peer's certificate chain cannot be completed!!");
			return;
		}

		ctx.extractPubkeyHashes(chain);

		TPKP_CHECK_THROW_EXCEPTION(ctx.checkPubkeyPins(),
			TPKP_E_PUBKEY_MISMATCH, "The pubkey mismatched with pinned data!");
	});

	return err_tpkp_to_gnutlse(res);
}

EXPORT_API
int tpkp_gnutls_set_trusted_cert_store(gnutls_certificate_credentials_t credentials)
{
	return gnutls_certificate_set_x509_trust_file(
		credentials,
		"/etc/ssl/ca-bundle.pem",
		GNUTLS_X509_FMT_PEM);
}

EXPORT_API
tpkp_e tpkp_gnutls_set_url_data(const char *url)
{
	return TPKP::ExceptionSafe([&]{
		pid_t tid = TPKP::getThreadId();

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_urlmap[tid] = url;
		}

		SLOGD("set url[%s] of thread id[%u]", url, tid);
	});
}

EXPORT_API
void tpkp_gnutls_cleanup(void)
{
	TPKP::ExceptionSafe([&]{
		auto tid = TPKP::getThreadId();

		{
			std::lock_guard<std::mutex> lock(s_mutex);
			s_urlmap.erase(tid);
		}

		SLOGD("cleanup url data from thread id[%u]", tid);
	});
}

EXPORT_API
void tpkp_gnutls_cleanup_all(void)
{
	s_urlmap.clear();
}
