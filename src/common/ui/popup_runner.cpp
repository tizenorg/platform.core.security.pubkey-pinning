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
 * @file        popup_runner.cpp
 * @author      Janusz Kozerski (j.kozerski@samsung.com)
 * @version     1.0
 */
#include "ui/popup_runner.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>

#include "tpkp_logger.h"
#include "ui/popup_common.h"

namespace TPKP {
namespace UI {

namespace {

const char *POPUP_EXEC = "/usr/bin/tpkp-popup";

class Pipes {
public:
	Pipes();
	virtual ~Pipes();

	void set(pid_t pid);
	int in() const;
	int out() const;

private:
	const int FD_UNKNOWN = -1;
	const int IDX_READ = 0;
	const int IDX_WRITE = 1;

	void closeExclusives();

	int m_fdParent[2];
	int m_fdChild[2];
	int m_in;
	int m_out;
};


/* TODO(k.tak): handle error on pipe() */
Pipes::Pipes() : m_in(FD_UNKNOWN), m_out(FD_UNKNOWN)
{
	m_fdParent[IDX_READ] = FD_UNKNOWN;
	m_fdParent[IDX_WRITE] = FD_UNKNOWN;
	m_fdChild[IDX_READ] = FD_UNKNOWN;
	m_fdChild[IDX_WRITE] = FD_UNKNOWN;

	if (pipe(m_fdParent) != 0) {
		SLOGE("Pipe creation failed");
	}

	if (pipe(m_fdChild) != 0) {
		SLOGE("Pipe creation failed");
	}
}

Pipes::~Pipes()
{
	if (m_fdParent[IDX_READ] != FD_UNKNOWN)
		close(m_fdParent[IDX_READ]);
	if (m_fdParent[IDX_WRITE] != FD_UNKNOWN)
		close(m_fdParent[IDX_WRITE]);

	if (m_fdChild[IDX_READ] != FD_UNKNOWN)
		close(m_fdChild[IDX_READ]);
	if (m_fdChild[IDX_WRITE] != FD_UNKNOWN)
		close(m_fdChild[IDX_WRITE]);
}

void Pipes::set(pid_t pid)
{
	if (pid == 0) {
		m_in = m_fdParent[IDX_READ];
		m_out = m_fdChild[IDX_WRITE];
	} else {
		m_in = m_fdChild[IDX_READ];
		m_out = m_fdParent[IDX_WRITE];
	}

	closeExclusives();
}

inline int Pipes::in() const
{
	return m_in;
}

inline int Pipes::out() const
{
	return m_out;
}

void Pipes::closeExclusives()
{
	if (m_fdParent[IDX_READ] != FD_UNKNOWN && m_fdParent[IDX_READ] != m_in) {
		close(m_fdParent[IDX_READ]);
		m_fdParent[IDX_READ] = FD_UNKNOWN;
	}

	if (m_fdParent[IDX_WRITE] != FD_UNKNOWN && m_fdParent[IDX_WRITE] != m_out) {
		close(m_fdParent[IDX_WRITE]);
		m_fdParent[IDX_WRITE] = FD_UNKNOWN;
	}

	if (m_fdChild[IDX_READ] != FD_UNKNOWN && m_fdChild[IDX_READ] != m_in) {
		close(m_fdChild[IDX_READ]);
		m_fdChild[IDX_READ] = FD_UNKNOWN;
	}

	if (m_fdChild[IDX_WRITE] != FD_UNKNOWN && m_fdChild[IDX_WRITE] != m_out) {
		close(m_fdChild[IDX_WRITE]);
		m_fdChild[IDX_WRITE] = FD_UNKNOWN;
	}
}

struct TpkpPopupParent {
	std::unique_ptr<Pipes> pipes;
	std::string hostname;
	pid_t childpid;
	Response response;

	TpkpPopupParent() :
		pipes(new Pipes),
		hostname(),
		childpid(0),
		response(Response::ERROR) {}

	void setup(pid_t child)
	{
		pipes->set(child);
		childpid = child;
	}
};

int waitForChild(pid_t childpid, int timeout)
{
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGCHLD);
	sigprocmask(SIG_BLOCK, &set, nullptr);

	siginfo_t info;
	struct timespec time = {timeout, 0L};
	int ret;

	if (timeout > 0)
		ret = TEMP_FAILURE_RETRY(sigtimedwait(&set, &info, &time));
	else
		ret = TEMP_FAILURE_RETRY(sigwaitinfo(&set, &info));

	sigprocmask(SIG_UNBLOCK, &set, nullptr);

	if (ret == SIGCHLD && info.si_pid == childpid) {
		// call waitpid on the child process to get rid of zombie process
		waitpid(childpid, nullptr, 0);

		// The proper signal has been caught and its pid matches to childpid
		// Now check the popup exit status
		int status = WEXITSTATUS(info.si_status);
		SLOGD("STATUS EXIT ON POPUP (CHILD: %zu): %d", info.si_pid, status);

		switch (static_cast<PopupStatus>(status)) {
		case PopupStatus::NO_ERROR:
			SLOGD("NO_ERROR");
			return 0;

		case PopupStatus::EXIT_ERROR:
			SLOGE("ERROR");
			return -1;

		default:
			SLOGE("UNKNOWN_ERROR");
			return -1;
		}
	}

	if  (ret == -1 && errno == EAGAIN)
		SLOGE("POPUP TIMEOUT");
	else
		SLOGE("Some other signal has been caught (pid: %zu, signal: %d)", info.si_pid, info.si_signo);

	// kill popup process and return error
	kill(childpid, SIGKILL);

	// call waitpid on the child process to get rid of zombie process
	waitpid(childpid, nullptr, 0);

	return -1;
}

/*
 *  parent send list
 *  - std::string hostname
 */
BinaryStream serialize(TpkpPopupParent *pdp)
{
	BinaryStream stream;

	Serialization::Serialize(stream, pdp->hostname);

	return stream;
}

/*
 *  parent receive list
 *  - TPKP::UI::Response response (int)
 */
void deserialize(TpkpPopupParent *pdp, char *buf, ssize_t count)
{
	BinaryStream stream;
	stream.Write(count, static_cast<void *>(buf));

	int responseInt;
	Deserialization::Deserialize(stream, responseInt);
	pdp->response = static_cast<Response>(responseInt);
}

int send(TpkpPopupParent *pdp)
{
	BinaryStream stream = serialize(pdp);

	unsigned int begin = 0;
	while (begin < stream.size()) {
		int tmp = TEMP_FAILURE_RETRY(
				write(pdp->pipes->out(), stream.data() + begin, stream.size() - begin));
		if (tmp == -1) {
			SLOGE("Write to pipe failed!");
			return -1;
		}
		begin += tmp;
	}

	SLOGD("send parent info to tpkp popup child successfully");

	return 0;
}

int receive(TpkpPopupParent *pdp)
{
	constexpr int size = 1024;
	char buf[size];
	ssize_t count = TEMP_FAILURE_RETRY(read(pdp->pipes->in(), buf, size));

	SLOGD("RESULT FROM POPUP PIPE (CHILD) : [ %d ]", count);

	deserialize(pdp, buf, count);

	SLOGD("receive data from tpkp child popup successfully");

	return 0;
}

void childProcess(TpkpPopupParent *pdp)
{
	int pipeIn = pdp->pipes->in();
	int pipeOut = pdp->pipes->out();

	SLOGD("Passed file descriptors: %d, %d", pipeIn, pipeOut);

	if (execl(POPUP_EXEC,
			POPUP_EXEC,
			std::to_string(pipeIn).c_str(),
			std::to_string(pipeOut).c_str(),
			nullptr) < 0)
		SLOGE("execl FAILED");

	SLOGE("This should not happen!!!");
	_exit(static_cast<int>(Response::ERROR));
}

Response parentProcess(TpkpPopupParent *pdp, int timeout)
{
	if (send(pdp) != 0)
		return Response::ERROR;

	if (waitForChild(pdp->childpid, timeout) != 0)
		return Response::ERROR;

	if (receive(pdp) != 0)
		return Response::ERROR;

	SLOGD("tpkp popup parent process successfully done with response[%d]",
		static_cast<int>(pdp->response));

	return pdp->response;
}

} // anonymous namespace

Response runPopup(const std::string &hostname, int timeout)
{
	SLOGD("hostname: %s", hostname.c_str());

	TpkpPopupParent pd;
	TpkpPopupParent *pdp = &pd;

	pdp->hostname = hostname;

	pid_t childpid;
	if ((childpid = fork()) == -1) {
		SLOGE("Fork() ERROR");
		return Response::ERROR;
	}

	pdp->setup(childpid);

	if (childpid == 0) {
		SLOGD("Child");
		childProcess(pdp);
	} else {
		SLOGD("Parent (child pid: %zu)", childpid);
		return parentProcess(pdp, timeout);
	}

	SLOGE("This should not happen!!!");

	return Response::ERROR;
}

} // UI
} // TPKP
