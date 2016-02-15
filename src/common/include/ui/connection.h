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
 * @file        client.h
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 */
#pragma once

#include <string>

#include "ui/popup_common.h"

#define TPKP_UI_SOCK_ADDR "/tmp/.tpkp-ui-backend.sock"

namespace TPKP {
namespace UI {

class SockRaii {
public:
	SockRaii();
	virtual ~SockRaii();

	void connect(const std::string &interface);
	void disconnect(void);
	bool isConnected(void) const;
	int get(void) const;
	int waitForSocket(int event, int timeout);

protected:
	void connectWrapper(int socket, const std::string &interface);
	int m_sock;
};

class ServiceConnection {
public:
	ServiceConnection(const std::string &interface);
	virtual ~ServiceConnection();

	void send(const BinaryStream &stream);
	BinaryStream receive(void);

	void prepareConnection(void);
	BinaryStream processRequest(const BinaryStream &input);

protected:
	SockRaii m_socket;
	std::string m_serviceInterface;

};

}
}
