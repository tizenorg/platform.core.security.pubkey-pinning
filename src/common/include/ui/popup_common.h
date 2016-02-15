/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @file        popup_common.h
 * @author      Kyungwook Tak <k.tak@samsung.com>
 * @version     1.0
 */
#pragma once

#include "ui/serialization.h"

#define EXPORT_API __attribute__((visibility("default")))

namespace TPKP {
namespace UI {

enum class PopupStatus : int {
	NO_ERROR,
	EXIT_ERROR
};

enum class Response : int {
	ALLOW,
	DENY,
	ERROR
};

class EXPORT_API BinaryStream : public IStream {
public:
	BinaryStream();
	virtual ~BinaryStream();
	BinaryStream(BinaryStream &&other);
	BinaryStream &operator=(BinaryStream &&other);

	virtual void Read(size_t num, void *bytes) override;
	virtual void Write(size_t num, const void *bytes) override;

	const unsigned char *data() const;
	size_t size() const;

	template <typename... Args>
	static BinaryStream Serialize(const Args&... args)
	{
		BinaryStream stream;
		Serializer<Args...>::Serialize(stream, args...);
		return stream;
	}

	template <typename... Args>
	void Deserialize(Args&... args)
	{
		Deserializer<Args...>::Deserialize(*this, args...);
	}

private:
	std::vector<unsigned char> m_data;
	size_t m_readPosition;
};

void sendStream(int fd, const BinaryStream &stream);
BinaryStream receiveStream(int fd);

} // UI
} // TPKP
