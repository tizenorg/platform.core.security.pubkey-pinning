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

int wait_for_popup(int popup_pid, int timeout)
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

	if (ret == SIGCHLD && info.si_pid == popup_pid) {
		// call waitpid on the child process to get rid of zombie process
		waitpid(popup_pid, nullptr, 0);

		// The proper signal has been caught and its pid matches to popup_pid
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
	kill(popup_pid, SIGKILL);

	// call waitpid on the child process to get rid of zombie process
	waitpid(popup_pid, nullptr, 0);

	return -1;
}

void child_process(int fd_send_to_child[2], int fd_send_to_parent[2])
{
	SLOGD("Child");

	// read data from parent
	close(fd_send_to_child[1]);

	// send data to parent
	close(fd_send_to_parent[0]);

	std::stringstream pipe_in_buff;
	std::stringstream pipe_out_buff;
	pipe_in_buff  << fd_send_to_parent[1];
	pipe_out_buff << fd_send_to_child[0];
	std::string pipe_in  = pipe_in_buff.str();
	std::string pipe_out = pipe_out_buff.str();

	SLOGD("Passed file descriptors: %d, %d", fd_send_to_child[0], fd_send_to_parent[1]);

	if (execl(POPUP_EXEC, POPUP_EXEC, pipe_out.c_str(), pipe_in.c_str(), nullptr) < 0)
		SLOGE("execl FAILED");

	SLOGE("This should not happen!!!");
	_exit(static_cast<int>(Response::ERROR));
}

int send_message_to_child(const BinaryStream &stream, int fd_send_to_child)
{
	SLOGD("Sending message to popup-bin process");

	unsigned int begin = 0;
	while (begin < stream.size()) {
		int tmp = TEMP_FAILURE_RETRY(
				write(fd_send_to_child, stream.data() + begin, stream.size() - begin));
		if (-1 == tmp) {
			SLOGE("Write to pipe failed!");
			return -1;
		}
		begin += tmp;
	}

	SLOGD("Message has been sent");

	return 0;
}

std::string response_to_str(Response response)
{
	switch (response) {
	case Response::ALLOW:
		return "ALLOW";
	case Response::DENY:
		return "DENY";
	default:
		return "ERROR";
	}
}

Response parse_response(int count, char *data)
{
	SLOGD("RESULT FROM POPUP PIPE (CHILD) : [ %d ]", count);

	int response_int;
	Response response;
	BinaryStream stream;

	stream.Write(count, data);
	Deserialization::Deserialize(stream, response_int);
	response = static_cast<Response>(response_int);

	SLOGD("response: %s", response_to_str(response).c_str());

	return response;
}

} // anonymous namespace

Response runPopup(const std::string &hostname, int timeout)
{
	SLOGD("hostname: %s", hostname.c_str());

	int fd_send_to_child[2];
	if (pipe(fd_send_to_child) != 0) {
		SLOGE("Cannot create pipes!");
		return Response::ERROR;
	}

	int fd_send_to_parent[2];
	if (pipe(fd_send_to_parent) != 0) {
		SLOGE("Cannot create pipes!");
		close(fd_send_to_child[0]);
		close(fd_send_to_child[1]);
		return Response::ERROR;
	}

	pid_t childpid;
	if ((childpid = fork()) == -1) {
		SLOGE("Fork() ERROR");
		close(fd_send_to_child[0]);
		close(fd_send_to_child[1]);
		close(fd_send_to_parent[0]);
		close(fd_send_to_parent[1]);
		return Response::ERROR;
	}

	if (childpid == 0) { // Child process
		child_process(fd_send_to_child, fd_send_to_parent);
	} else { // Parent process
		SLOGD("Parent (child pid: %zu)", childpid);

		// send data to child
		close(fd_send_to_child[0]);

		// read data from child
		close(fd_send_to_parent[1]);

		BinaryStream stream;
		Serialization::Serialize(stream, hostname);

		// writing to child
		if (send_message_to_child(stream, fd_send_to_child[1])) {
			SLOGE("send_message_to_child failed!");
			close(fd_send_to_parent[0]);
			close(fd_send_to_child[1]);
			return Response::ERROR;
		}

		// wait for child
		if (wait_for_popup(childpid, timeout) != 0) {
			SLOGE("wait_for_popup failed!");
			close(fd_send_to_parent[0]);
			close(fd_send_to_child[1]);
			return Response::ERROR;
		}

		// Read message from popup (child)
		constexpr int buff_size = 1024;
		char result[buff_size];
		int count = TEMP_FAILURE_RETRY(read(fd_send_to_parent[0], result, buff_size));

		// Parsing response from child
		Response response;
		if (count < 0) {
			response = parse_response(count, result);
		} else {
			SLOGE("ERROR, count = %d", count);
			close(fd_send_to_parent[0]);
			close(fd_send_to_child[1]);
			return Response::ERROR;
		}

		SLOGD("popup-runner: EXIT SUCCESS");

		// cleanup
		close(fd_send_to_parent[0]);
		close(fd_send_to_child[1]);

		return response;
	}

	SLOGE("This should not happen!!!");

	return Response::ERROR;
}

} // UI
} // TPKP
