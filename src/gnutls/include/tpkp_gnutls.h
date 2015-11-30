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
 *  @brief   gnutls_certificate_verify_function of verifying pubkey pinning
 *
 *  @remarks set by gnutls_certificate_verify_function().
 *
 *
 */
int tpkp_gnutls_verify_callback(gnutls_session_t session);

tpkp_e tpkp_gnutls_set_url_data(const char *url);

void tpkp_gnutls_cleanup(void);

void tpkp_gnutls_cleanup_all(void);

#ifdef __cplusplus
}
#endif

#endif /* TPKP_GNUTLS_H_ */
