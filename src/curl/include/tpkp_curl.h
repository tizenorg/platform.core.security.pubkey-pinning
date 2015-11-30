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
 * @file        tpkp_curl.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning interface for libcurl.
 */
#ifndef TPKP_CURL_H_
#define TPKP_CURL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ssl.h>
#include <curl/curl.h>
#include <tpkp_error.h>

/*
 *  @brief   verify_callback of OpenSSL SSL_CTX_set_verify.
 *
 *  @remarks verify_callback which verifies HPKP with url set by
 *           tpkp_curl_set_url_data() in ssl_ctx_callback set by
 *           CURLOPT_SSL_CTX_FUNCTION curl option.
 *  @remarks tpkp_curl_set_url_data() should be used to set url data
 *           on ssl_ctx_callback.
 *
 *
 *  @param[in] preverify_ok Whether the verification of the certificate in
 *                          question was passed(1) or not(0)
 *  @param[in] x509_ctx     pointer to the complete context used for
 *                          certificate chain verification
 *
 *  @return return 1 on success, otherwise return 0.
 *
 *  @see tpkp_curl_set_url_data()
 */
int tpkp_curl_verify_callback(int preverify_ok, X509_STORE_CTX *x509_ctx);

/*
 *  @brief   Sets current url to check pinned info later (in tpkp_curl_verify_callback())
 *
 *  @remarks Url data is saved thread-specifically.
 *  @remarks tpkp_curl_cleanup() should be called before current thread ended
 *           or tpkp_curl_cleanup_all() should be called on thread globally
 *           before the process ended to use libcurl.
 *
 *  @param[in]     curl     cURL handle
 *
 *  @return #TPKP_E_NONE on success.
 *
 *  @see tpkp_curl_cleanup()
 *  @see tpkp_curl_cleanup_all()
 */
tpkp_e tpkp_curl_set_url_data(CURL *curl);

/*
 *  @brief   Sets verify_callback with SSL_CTX_set_verify.
 *
 *  @remarks For clients who doesn't use SSL_CTX_set_verify in ssl_ctx_callback.
 *  @remarks url data is set internall, so client doesn't needed to call
 *           tpkp_curl_set_url_data().
 *  @remarks tpkp_curl_cleanup() should be called before current thread ended
 *           or tpkp_curl_cleanup_all() should be called on thread globally
 *           before the process ended to use libcurl.
 *
 *  @param[in]     curl     cURL handle
 *  @param[in/out] ssl_ctx  OpenSSL SSL_CTX
 *
 *  @return #TPKP_E_NONE on success.
 *
 *  @see tpkp_curl_cleanup()
 *  @see tpkp_curl_cleanup_all()
 */
tpkp_e tpkp_curl_set_verify(CURL *curl, SSL_CTX *ssl_ctx);

/*
 *  @brief SSL context callback for OpenSSL. callback type is defined by libcurl.
 *
 *  @remarks Refer CURLOPT_SSL_CTX_FUNCTION and ssl_ctx_callback defined in libcurl.
 *  @remarks tpkp_curl_cleanup() should be called before current thread ended
 *           or tpkp_curl_cleanup_all() should be called on thread globally
 *           before the process ended to use libcurl.
 *
 *  @param[in]     curl     cURL handle
 *  @param[in/out] ssl_ctx  OpenSSL SSL_CTX
 *  @param[in]     userptr  Not used
 *
 *  @return TPKP_E_NONE on success.
 *
 *  @retval #TPKP_E_NONE
 *
 *  @see tpkp_curl_cleanup()
 *  @see tpkp_curl_cleanup_all()
 */
CURLcode tpkp_curl_ssl_ctx_callback(CURL *curl, void *ssl_ctx, void *userptr);

/*
 *  @brief   Cleans up memory of current thread.
 *
 *  @remarks Only cleans up current thread's specific memory. It should be called
 *           inside of thread before end.
 *  @remarks Call beside of curl_[easy|multi]_cleanup().
 *
 *  @see tpkp_curl_ssl_ctx_callback()
 *  @see tpkp_curl_set_verify()
 *  @see tpkp_curl_set_url_data()
 */
void tpkp_curl_cleanup(void);

/*
 *  @brief   Cleans up all memory used by tpkp_curl API.
 *
 *  @remarks Should be called thread-globally, after all jobs done by worker threads.
 *  @remarks Call beside of curl_global_cleanup().
 *
 *  @see tpkp_curl_ssl_ctx_callback()
 *  @see tpkp_curl_set_verify()
 *  @see tpkp_curl_set_url_data()
 */
void tpkp_curl_cleanup_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TPKP_CURL_H_ */
