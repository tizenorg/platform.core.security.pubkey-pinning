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
 * @file        tpkp_error.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning error codes.
 */
#ifndef TPKP_ERROR_H_
#define TPKP_ERROR_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	TPKP_E_NONE                     = 0,
	TPKP_E_MEMORY                   = -1,
	TPKP_E_INVALID_URL              = -2,
	TPKP_E_INVALID_CERT             = -3,
	TPKP_E_FAILED_GET_PUBKEY_HASH   = -4,
	TPKP_E_NO_URL_DATA              = -5,
	TPKP_E_INVALID_PEER_CERT_CHAIN  = -6,
	TPKP_E_PUBKEY_MISMATCH          = -7,
	TPKP_E_CERT_VERIFICATION_FAILED = -8,
	TPKP_E_INVALID_PARAMETER        = -9,
	TPKP_E_IO                       = -10,
	TPKP_E_OUT_OF_MEMORY            = -11,
	TPKP_E_TIMEOUT                  = -12,
	TPKP_E_PERMISSION_DENIED        = -13,
	TPKP_E_STD_EXCEPTION            = -99,
	TPKP_E_INTERNAL                 = -100
} tpkp_e;

#ifdef __cplusplus
}
#endif

#endif /* TPKP_ERROR_H_ */
