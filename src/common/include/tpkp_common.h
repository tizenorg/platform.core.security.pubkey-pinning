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
 * @file        tpkp_common.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       Tizen Https Public Key Pinning common library interface.
 */
#pragma once

#include <string>
#include <vector>
#include <memory>
#include <type_traits>

#include "tpkp_exception.h"
#include "tpkp_error.h"

#define EXPORT_API __attribute__((visibility("default")))

/*
 *  classes under this header may throw exception which declared in tpkp_exception.h
 */

namespace TPKP {

enum class HashAlgo : int {
	SHA1,
	SHA256, // currently not supported. preloaded public keys were hashed with SHA1
	COUNT,
	DEFAULT = SHA1
};

/* hash size as a byte */
enum class HashSize : size_t {
	SHA1 = 20,
	SHA256 = 32,
	DEFAULT = SHA1
};

template<typename E>
constexpr auto typeCast(E e) -> typename std::underlying_type<E>::type
{
	return static_cast<typename std::underlying_type<E>::type>(e);
}

using RawBuffer = std::vector<unsigned char>;

struct HashValue {
	HashAlgo algo;
	RawBuffer hash;

	HashValue() = delete;
	explicit HashValue(HashAlgo _algo, const RawBuffer &_hash)
		: algo(_algo)
		, hash(_hash)
	{}
};

using HashValueVector = std::vector<HashValue>;

class EXPORT_API Context {
public:
	Context() = delete;
	virtual ~Context();
	explicit Context(const std::string &url);

	void addPubkeyHash(HashAlgo algo, const RawBuffer &hashBuf);
	bool checkPubkeyPins(void);
	bool hasPins(void);

private:
	class Impl;
	std::unique_ptr<Impl> pImpl;
};

}
