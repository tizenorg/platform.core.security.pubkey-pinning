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
 * @file        tpkp_gnutls.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning interface for gnutls.
 */
#ifndef TPKP_GNUTLS_H_
#define TPKP_GNUTLS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gnutls/gnutls.h>
#include <tpkp_error.h>

/*
 *  @brief   gnutls_certificate_verify_function of verifying pubkey pinning.
 *
 *  @remarks Set by gnutls_certificate_verify_function().
 *  @remarks System trusted certificates store should be set by
 *           tpkp_gnutls_set_trusted_cert_store() to verifying peer certificates.
 *  @remarks tpkp_gnutls_set_url_data() should be called to set url data before.
 *  @remarks Verify callback should be called in same thread which calls
 *           tpkp_gnutls_set_url_data().
 *
 *  @param[in] session  gnutls session of current connection.
 *
 *  @return return 0 for the handshake to continue, otherwise return non-zero to terminate.
 *
 *  @see tpkp_gnutls_set_trusted_cert_store()
 *  @see tpkp_gnutls_set_url_data()
 */
int tpkp_gnutls_certificate_verify_callback(gnutls_session_t session);

/*
 *  @brief   Sets current url to check pinned info by certificate verify callback.
 *
 *  @remarks Url data is saved thread-specifically.
 *  @remarks tpkp_gnutls_cleanup() should be called before current thread ended or
 *           tpkp_gnutls_cleanup_all() should be called on thread globally before the
 *           process ended to use gnutls.
 *
 *  @param[in] url  url which is null terminated c string
 *
 *  @return #TPKP_E_NONE on success.
 *
 *  @see tpkp_gnutls_cleanup()
 *  @see tpkp_gnutls_cleanup_all()
 */
tpkp_e tpkp_gnutls_set_url_data(const char *url);

/*
 *  @brief   Cleans up memory of current thread.
 *
 *  @remarks Only cleans up current thread's specific memory. It should be called inside
 *           of thread before end.
 *  @remarks Call beside of gnutls_deinit().
 *
 *  @see tpkp_gnutls_set_url_data()
 */
void tpkp_gnutls_cleanup(void);

/*
 *  @brief   Cleans up all memory used by tpkp_gnutls API.
 *
 *  @remarks Should be called thread-globally, after all jobs done by worker threads.
 *
 *  @see tpkp_gnutls_set_url_data()
 */
void tpkp_gnutls_cleanup_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TPKP_GNUTLS_H_ */
