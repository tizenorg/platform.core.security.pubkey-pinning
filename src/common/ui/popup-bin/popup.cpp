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
 * @file        popup.cpp
 * @author      Janusz Kozerski (j.kozerski@samsung.com)
 * @version     1.0
 */
#include <unistd.h>
#include <vector>
#include <memory>
#include <string>
#include <libintl.h>
#include <poll.h>
#include <sys/un.h>
#include <time.h>

#include <Elementary.h>
#include <Ecore.h>
#include <systemd/sd-daemon.h>
#include <vconf.h>

#include "tpkp_exception.h"
#include "tpkp_logger.h"
#include "ui/popup_common.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TPKP_POPUP"

using namespace TPKP::UI;

namespace {

struct TpkpPopup {
	/* inputs */
	std::string hostname;
	int timeout;

	/* internal data fields */
	Evas_Object *win;

	/* output */
	TPKP::UI::Response result;

	TpkpPopup() :
		hostname(),
		timeout(-1),
		win(nullptr),
		result(TPKP::UI::Response::ERROR) {}
};

struct SockRaii {
	int sock;
	SockRaii() : sock(-1) {}
	SockRaii(int _sock) : sock(_sock) {}
	~SockRaii()
	{
		if (sock != -1)
			close(sock);
	}
};

struct ElmRaii {
	ElmRaii(int argc, char **argv)
	{
		SLOGD("elm_init()");
		elm_init(argc, argv);

		elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);
	}

	virtual ~ElmRaii()
	{
		SLOGD("elm_shutdown()");
		elm_shutdown();
	}
};

void answerAllowCb(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("allow answer");

	TPKP_CHECK_THROW_EXCEPTION(data != nullptr,
		TPKP_E_INVALID_PARAMETER, "data shouldn't be null on evas callbacks");

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::ALLOW;

	evas_object_del(pdp->win);
}

void answerDenyCb(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("deny answer");

	TPKP_CHECK_THROW_EXCEPTION(data != nullptr,
		TPKP_E_INVALID_PARAMETER, "data shouldn't be null on evas callbacks");

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::DENY;

	evas_object_del(pdp->win);
}

Eina_Bool timeoutCb(void *data)
{
	TPKP_CHECK_THROW_EXCEPTION(data != nullptr,
		TPKP_E_INVALID_PARAMETER, "data shouldn't be null on timeout callback");
	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::DENY;

	SLOGI("popup timeout[%d](ms) reached! Let's deny", pdp->timeout);

	evas_object_del(pdp->win);

	return ECORE_CALLBACK_CANCEL;
}

std::unique_ptr<char> getPopupContentString(TpkpPopup *pdp)
{
	char *contentFormat = dgettext(PROJECT_NAME, "SID_CONTENT_PUBLIC_KEY_MISMATCHED");
	char *content = nullptr;

	if (asprintf(&content, contentFormat, pdp->hostname.c_str()) == -1)
		TPKP_THROW_EXCEPTION(TPKP_E_MEMORY,
			"Failed to alloc memory for popup text");

	return std::unique_ptr<char>(content);
}

/*
 *  popup layout
 *
 *               window
 *  --------------------------------
 *  |                              |
 *  |            popup             |
 *  | ---------------------------- |
 *  | |          title           | |
 *  | |--------------------------| |
 *  | |    content (description) | |
 *  | |                          | |
 *  | | -----------  ----------- | |
 *  | | | button1 |  | button2 | | |
 *  | | -----------  ----------- | |
 *  | ---------------------------- |
 *  |                              |
 *  --------------------------------
 */
void showPopup(TpkpPopup *pdp)
{
	SLOGD("Start to make popup");

	TPKP_CHECK_THROW_EXCEPTION(pdp != nullptr,
		TPKP_E_INVALID_PARAMETER, "pdp shouldn't be null");

	/* create win */
	Evas_Object *win = elm_win_add(nullptr, "tpkp popup", ELM_WIN_NOTIFICATION);
	elm_win_autodel_set(win, EINA_TRUE);
	elm_win_indicator_opacity_set(win, ELM_WIN_INDICATOR_TRANSLUCENT);
	elm_win_borderless_set(win, EINA_TRUE);
	elm_win_alpha_set(win, EINA_TRUE);
	evas_object_show(win);

	/* create popup */
	auto contentString = getPopupContentString(pdp);
	Evas_Object *popup = elm_popup_add(win);
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_text_set(popup, contentString.get());
	elm_object_part_text_set(popup, "title,text",
			dgettext(PROJECT_NAME, "SID_TITLE_PUBLIC_KEY_MISMATCHED"));
	evas_object_show(popup);

	/* create allow button */
	Evas_Object *buttonAllow = elm_button_add(popup);
	elm_object_style_set(buttonAllow, "bottom");
	elm_object_text_set(buttonAllow, dgettext(PROJECT_NAME, "SID_BTN_ALLOW"));
	elm_object_part_content_set(popup, "button1", buttonAllow);
	evas_object_smart_callback_add(buttonAllow, "clicked", answerAllowCb, pdp);
	evas_object_show(buttonAllow);

	/* create deny button */
	Evas_Object *buttonDeny = elm_button_add(popup);
	elm_object_style_set(buttonDeny, "bottom");
	elm_object_text_set(buttonDeny, dgettext(PROJECT_NAME, "SID_BTN_DENY"));
	elm_object_part_content_set(popup, "button2", buttonDeny);
	evas_object_smart_callback_add(buttonDeny, "clicked", answerDenyCb, pdp);
	evas_object_show(buttonDeny);

	if (pdp->timeout > 0)
		ecore_timer_add(pdp->timeout / 1000, timeoutCb, pdp);

	pdp->win = win;

	SLOGD("elm_run start");
	elm_run();
}

/*
 *  child receive list
 *  - std::string hostname
 */
void deserialize(TpkpPopup *pdp, BinaryStream &stream)
{
	Deserialization::Deserialize(stream, pdp->hostname);
	Deserialization::Deserialize(stream, pdp->timeout);

	SLOGD("Params from popup_runner: hostname[%s] timeout[%d]",
		pdp->hostname.c_str(), pdp->timeout);
}

/*
 *  child send list
 *  - TPKP::UI::Response response (int)
 */
BinaryStream serialize(TpkpPopup *pdp)
{
	BinaryStream stream;
	Serialization::Serialize(stream, static_cast<int>(pdp->result));

	return stream;
}

int getSockFromSystemd(void)
{
	int n = sd_listen_fds(0);

	for (int fd = SD_LISTEN_FDS_START; fd < SD_LISTEN_FDS_START + n; ++fd) {
		if (sd_is_socket_unix(fd, SOCK_STREAM, 1, SOCK_PATH, 0) > 0) {
			SLOGD("Get socket from systemd. fd[%d]", fd);
			return fd;
		}
	}
	TPKP_THROW_EXCEPTION(TPKP_E_IO, "Failed to get sock from systemd.");
}

} // namespace anonymous

int main(int argc, char **argv)
{
	SLOGI("tpkp popup backend server start!");

	/* init/shutdown elm automatically */
	ElmRaii elmRaii(argc, argv);

	setlocale(LC_ALL, vconf_get_str(VCONFKEY_LANGSET));

	try {
		struct sockaddr_un clientaddr;
		int client_len = sizeof(clientaddr);

		struct pollfd fds[1];
		fds[0].fd = getSockFromSystemd();
		fds[0].events = POLLIN;

		SLOGD("server fd from systemd: %d", fds[0].fd);

		while (true) {
			/* non blocking poll */
			int ret = poll(fds, 1, 0);
			if (ret < 0) {
				TPKP_THROW_EXCEPTION(TPKP_E_IO, "poll() error. errno: " << errno);
			} else if (ret == 0) {
				SLOGD("tpkp-popup backend service timeout. Let's be deactivated");
				return 0;
			}

			/* ready to accept! */

			memset(&clientaddr, 0, client_len);

			int clientFd = accept(fds[0].fd, (struct sockaddr *)&clientaddr, (socklen_t *)&client_len);
			TPKP_CHECK_THROW_EXCEPTION(clientFd >= 0, TPKP_E_IO, "Error in func accept()");
			SLOGD("client accepted with fd: %d", clientFd);

			SockRaii clientSock(clientFd);

			TpkpPopup pd;
			TpkpPopup *pdp = &pd;

			/* receive arguments */
			BinaryStream stream = receiveStream(clientFd);
			deserialize(pdp, stream);

			/* get user response */
			showPopup(pdp);
			SLOGD("pdp->result : %d", static_cast<int>(pdp->result));

			/* send result */
			stream = serialize(pdp);
			sendStream(clientFd, stream);

			SLOGD("tpkp-popup done successfully!");
		}
	} catch (const TPKP::Exception &e) {
		SLOGE("Exception[%d]: %s", e.code(), e.what());
	} catch (const std::bad_alloc &e) {
		SLOGE("bad_alloc std exception: %s", e.what());
	} catch (const std::exception &e) {
		SLOGE("std exception: %s", e.what());
	} catch (...) {
		SLOGE("Unhandled exception occured!");
	}

	return 0;
}
