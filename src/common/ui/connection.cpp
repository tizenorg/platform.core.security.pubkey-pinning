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
 * @file        connection.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 */
#include "ui/connection.h"

#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "tpkp_common.h"
#include "tpkp_logger.h"
#include "tpkp_error.h"

namespace TPKP {
namespace UI {

SockRaii::SockRaii() : m_sock(-1) {}

SockRaii::~SockRaii()
{
	disconnect();
}

void SockRaii::connect(const std::string &interface)
{
	TPKP_CHECK_THROW_EXCEPTION(!interface.empty(),
		TPKP_E_INVALID_PARAMETER, "No valid interface address given.");

	int sock = socket(AF_UNIX, SOCK_STREAM, 0);

	SLOGD("make client sock: %d", sock);

	TPKP_CHECK_THROW_EXCEPTION(sock >= 0,
		TPKP_E_IO, "Error to create sock");

	connectWrapper(sock, interface);

	disconnect(); /* refresh in case of old socket remained */
	m_sock = sock;
}

void SockRaii::connectWrapper(int sock, const std::string &interface)
{
	sockaddr_un clientaddr;

	TPKP_CHECK_THROW_EXCEPTION(
		interface.length() < sizeof(clientaddr.sun_path),
		TPKP_E_INVALID_PARAMETER,
		"Error: interface name[" << interface << "] is too long");

	memset(&clientaddr, 0, sizeof(clientaddr));
	clientaddr.sun_family = AF_UNIX;
	strcpy(clientaddr.sun_path, interface.c_str());

	int ret = TEMP_FAILURE_RETRY(
			::connect(sock, (struct sockaddr *)&clientaddr, SUN_LEN(&clientaddr)));

	const int err = errno;
	if (ret == -1) {
		if (err == EACCES)
			TPKP_THROW_EXCEPTION(TPKP_E_PERMISSION_DENIED,
				"Access denied to interface: " << interface);

		TPKP_THROW_EXCEPTION(TPKP_E_IO,
			"Error on connect socket. errno: " << err);
	}
}

bool SockRaii::isConnected(void) const
{
	return m_sock > -1;
}

void SockRaii::disconnect(void)
{
	if (isConnected()) {
		close(m_sock);
		SLOGD("close sock[%d] on client", m_sock);
	}

	m_sock = -1;
}

void SockRaii::waitForStreamIn(int timeout)
{
	int ret;

	pollfd fds[1];

	fds[0].fd = m_sock;
	fds[0].events = POLLIN;

	while (((ret = poll(fds, 1, timeout)) == -1) && errno == EINTR) {
		timeout >>= 1;
		errno = 0;
	}

	if (ret == 0)
		TPKP_THROW_EXCEPTION(TPKP_E_TIMEOUT, "Poll timeout[" << timeout << "]!!");
	else if (ret == -1)
		TPKP_THROW_EXCEPTION(TPKP_E_IO, "Error in poll! errno: " << errno);
}

int SockRaii::get(void) const
{
	return m_sock;
}

ServiceConnection::ServiceConnection(const std::string &interface, int timeout)
	: m_serviceInterface(interface), m_timeout(timeout) {}

ServiceConnection::~ServiceConnection() {}

void ServiceConnection::prepareConnection(void)
{
	if (!m_socket.isConnected())
		m_socket.connect(m_serviceInterface);
}

void ServiceConnection::send(const BinaryStream &stream)
{
	prepareConnection();
	sendStream(m_socket.get(), stream);
}

BinaryStream ServiceConnection::receive(void)
{
	TPKP_CHECK_THROW_EXCEPTION(m_socket.isConnected(),
		TPKP_E_IO, "Not connected!");

	m_socket.waitForStreamIn(m_timeout);

	return receiveStream(m_socket.get());
}

BinaryStream ServiceConnection::processRequest(const BinaryStream &input)
{
	send(input);

	return receive();
}

}
}
