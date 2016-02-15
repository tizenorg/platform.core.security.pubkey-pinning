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

/*
 * TODO(k.tak): Separate TPKP::Exception related codes from tpkp_common
 *              not to include "tpkp_common.h" which have lot of dependencies
 */
#include "tpkp_common.h"
#include "tpkp_logger.h"
#include "ui/popup_common.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TPKP_POPUP"

#define TPKP_UI_SOCK_ADDR "/tmp/.tpkp-ui-backend.sock"

using namespace TPKP::UI;

namespace {

struct TpkpPopup {
	/* inputs */
	std::string hostname;
	int timeout;

	/* internal data fields */
	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *box;
	Evas_Object *title;
	Evas_Object *content;
	Evas_Object *buttonAllow;
	Evas_Object *buttonDeny;

	/* output */
	TPKP::UI::Response result;

	TpkpPopup() :
		hostname(),
		timeout(-1),
		win(nullptr),
		popup(nullptr),
		box(nullptr),
		title(nullptr),
		content(nullptr),
		buttonAllow(nullptr),
		buttonDeny(nullptr),
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
		TPKP_E_INTERNAL, "data shouldn't be null on evas callbacks");

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::ALLOW;

	evas_object_del(pdp->win);
}

void answerDenyCb(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("deny answer");

	TPKP_CHECK_THROW_EXCEPTION(data != nullptr,
		TPKP_E_INTERNAL, "data shouldn't be null on evas callbacks");

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::DENY;

	evas_object_del(pdp->win);
}

Eina_Bool timeoutCb(void *data)
{
	TPKP_CHECK_THROW_EXCEPTION(data != nullptr,
		TPKP_E_INTERNAL, "data shouldn't be null on timeout callback");
	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::DENY;

	SLOGI("popup timeout[%d](ms) reached! Let's deny", pdp->timeout);

	evas_object_del(pdp->win);

	return ECORE_CALLBACK_CANCEL;
}

/*
 *  popup layout
 *
 *               window
 *  --------------------------------
 *  |                              |
 *  |            popup             |
 *  | ---------------------------- |
 *  | |    content (description) | |
 *  | |                          | |
 *  | |                          | |
 *  | | -----------  ----------- | |
 *  | | | button1 |  | button2 | | |
 *  | | -----------  ----------- | |
 *  | |                          | |
 *  | ---------------------------- |
 *  --------------------------------
 */
/* TODO(k.tak): UI layout refinement */
void showPopup(TpkpPopup *pdp)
{
	SLOGD("Start to make popup");

	TPKP_CHECK_THROW_EXCEPTION(pdp != nullptr,
		TPKP_E_INTERNAL, "pdp shouldn't be null");

	pdp->win = elm_win_add(nullptr, "tpkp popup", ELM_WIN_NOTIFICATION);

	elm_win_autodel_set(pdp->win, EINA_TRUE);
	elm_win_indicator_opacity_set(pdp->win, ELM_WIN_INDICATOR_TRANSLUCENT);
	elm_win_alpha_set(pdp->win, true);
	evas_object_show(pdp->win);

	pdp->popup = elm_popup_add(pdp->win);
	evas_object_show(pdp->popup);

	pdp->box = elm_box_add(pdp->popup);
	evas_object_size_hint_weight_set(pdp->box, EVAS_HINT_EXPAND, 0);
	evas_object_size_hint_align_set(pdp->box, EVAS_HINT_FILL, 0.0);
	evas_object_show(pdp->box);

	pdp->title = elm_label_add(pdp->popup);
	elm_object_style_set(pdp->title, "elm.text.title");
	elm_object_text_set(pdp->title, dgettext(PROJECT_NAME, "SID_TITLE_PUBLIC_KEY_MISMATCHED"));
	evas_object_show(pdp->title);
	elm_box_pack_end(pdp->box, pdp->title);

	pdp->content = elm_label_add(pdp->popup);
	elm_object_style_set(pdp->content, "elm.swallow.content");
	elm_label_line_wrap_set(pdp->content, ELM_WRAP_MIXED);
	char *contentFormat = dgettext(PROJECT_NAME, "SID_CONTENT_PUBLIC_KEY_MISMATCHED");
	char *content = nullptr;
	if (asprintf(&content, contentFormat, pdp->hostname.c_str()) == -1) {
		SLOGE("Failed to alloc memory for popup text. Just go for it.");
		elm_object_text_set(pdp->content, contentFormat);
	} else {
		elm_object_text_set(pdp->content, content);
		free(content);
	}

	evas_object_size_hint_weight_set(pdp->content, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(pdp->content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(pdp->content);
	elm_box_pack_end(pdp->box, pdp->content);

	elm_object_part_content_set(pdp->popup, "default", pdp->box);

	pdp->buttonAllow = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->buttonAllow, "elm.swallow.content.button1");
	elm_object_text_set(pdp->buttonAllow, dgettext(PROJECT_NAME, "SID_BTN_ALLOW"));
	elm_object_part_content_set(pdp->popup, "button1", pdp->buttonAllow);
	evas_object_smart_callback_add(pdp->buttonAllow, "clicked", answerAllowCb, pdp);
	evas_object_show(pdp->buttonAllow);

	pdp->buttonDeny = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->buttonDeny, "elm.swallow.content.button2");
	elm_object_text_set(pdp->buttonDeny, dgettext(PROJECT_NAME, "SID_BTN_DENY"));
	elm_object_part_content_set(pdp->popup, "button2  ", pdp->buttonDeny);
	evas_object_smart_callback_add(pdp->buttonDeny, "clicked", answerDenyCb, pdp);
	evas_object_show(pdp->buttonDeny);

	if (pdp->timeout > 0) {
		ecore_timer_add(pdp->timeout / 1000, timeoutCb, pdp);
	}

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
		if (sd_is_socket_unix(fd, SOCK_STREAM, 1, TPKP_UI_SOCK_ADDR, 0) > 0) {
			SLOGD("Get socket from systemd. fd[%d]", fd);
			return fd;
		}
	}
	TPKP_THROW_EXCEPTION(TPKP_E_INTERNAL, "Failed to get sock from systemd.");
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
			TPKP_CHECK_THROW_EXCEPTION(ret >= 0,
				TPKP_E_INTERNAL, "poll() error. errno: " << errno);

			if (ret == 0) {
				SLOGD("tpkp-popup backend service timeout. Let's be deactivated");
				return 0;
			}

			/* ready to accept! */

			memset(&clientaddr, 0, client_len);

			int clientFd = accept(fds[0].fd, (struct sockaddr *)&clientaddr, (socklen_t *)&client_len);
			TPKP_CHECK_THROW_EXCEPTION(clientFd >= 0, TPKP_E_INTERNAL, "Error in func accept()");
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
