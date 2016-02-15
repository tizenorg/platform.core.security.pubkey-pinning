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
 * @file        popup_common.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 */
#include "ui/popup_common.h"

#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "tpkp_common.h"
#include "tpkp_logger.h"

namespace TPKP {
namespace UI {

BinaryStream::BinaryStream() : m_readPosition(0) {}

BinaryStream::~BinaryStream() {}

BinaryStream::BinaryStream(BinaryStream &&other) :
	m_data(std::move(other.m_data)),
	m_readPosition(other.m_readPosition)
{
	other.m_readPosition = other.m_data.size();
}

BinaryStream &BinaryStream::operator=(BinaryStream &&other)
{
	if (this == &other)
		return *this;

	m_data = std::move(other.m_data);
	m_readPosition = other.m_readPosition;
	other.m_readPosition = 0;

	return *this;
}

void BinaryStream::Read(size_t num, void *bytes)
{
	size_t max_size = m_data.size();
	for (size_t i = 0; i < num; ++i) {
		if (i + m_readPosition >= max_size)
			return;
		static_cast<unsigned char *>(bytes)[i] = m_data[i + m_readPosition];
	}
	m_readPosition += num;
}

void BinaryStream::Write(size_t num, const void *bytes)
{
	for (size_t i = 0; i < num; ++i)
		m_data.push_back(static_cast<const unsigned char *>(bytes)[i]);
}

const unsigned char *BinaryStream::data() const
{
	return m_data.data();
}

size_t BinaryStream::size() const
{
	return m_data.size();
}

EXPORT_API
void sendStream(int fd, const BinaryStream &stream)
{
	auto buf = stream.data();
	auto size = stream.size();

	ssize_t ret;
	size_t offset = 0;
	while (offset != size && (ret = send(fd, buf + offset, size - offset, 0)) != 0) {
		if (ret == -1) {
			const int err = errno;
			if (err == EINTR)
				continue;
			else
				TPKP_THROW_EXCEPTION(TPKP_E_INTERNAL,
					"write failed with errno: " << err);
		}
		offset += ret;
	}

	SLOGD("send data successfully");
}

EXPORT_API
BinaryStream receiveStream(int fd)
{
	constexpr size_t size = 1024;
	char buf[size];
	ssize_t ret;

	while ((ret = recv(fd, buf, size, 0)) == -1) {
		if (ret == -1) {
			const int err = errno;
			if (err == EINTR)
				continue;
			else
				TPKP_THROW_EXCEPTION(TPKP_E_INTERNAL,
					"read failed with errno: " << err);
		}
	}

	SLOGD("receive data successfully");

	BinaryStream stream;
	stream.Write(static_cast<size_t>(ret), static_cast<void *>(buf));

	return stream;
}

} // UI
} // TPKP
