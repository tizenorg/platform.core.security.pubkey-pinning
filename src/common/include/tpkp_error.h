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

typedef enum {
	TPKP_E_NONE = 0,
	TPKP_E_MEMORY,
	TPKP_E_INVALID_URL,
	TPKP_E_INVALID_CERT,
	TPKP_E_FAILED_GET_PUBKEY_HASH,
	TPKP_E_NO_URL_DATA,
	TPKP_E_INVALID_PEER_CERT_CHAIN,
	TPKP_E_PUBKEY_MISMATCH,
	TPKP_E_STD_EXCEPTION,
	TPKP_E_INTERNAL
} tpkp_e;

#endif /* TPKP_ERROR_H_ */
