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

namespace TPKP {
namespace UI {

BinaryStream::BinaryStream() : m_readPosition(0) {}

BinaryStream::~BinaryStream() {}

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

void BinaryStream::Write(size_t num, const void * bytes)
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

} // UI
} // TPKP
