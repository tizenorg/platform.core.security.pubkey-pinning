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
#include "popup.h"

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <memory>
#include <string>
#include <libintl.h>
#include <sys/select.h>
#include <time.h>

#include <Elementary.h>

#include "ui/popup_common.h"
#include "tpkp_logger.h"

using namespace TPKP::UI;


Pipe::Pipe(int in, int out) : m_in(in), m_out(out) {}

Pipe::~Pipe()
{
	close(m_in);
	close(m_out);
}

inline int Pipe::in() const
{
	return m_in;
}

inline int Pipe::out() const
{
	return m_out;
}

void on_done(void)
{
	SLOGD("elm_exit()");
	elm_exit();
}

namespace {

void allow_answer(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("allow_answer");

	if (data == nullptr) {
		SLOGE("data is nullptr");
		return;
	}

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::ALLOW;

	on_done();
}

void deny_answer(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("deny_answer");

	if (data == nullptr) {
		SLOGE("data is nullptr");
		return;
	}

	TpkpPopup *pdp = static_cast<TpkpPopup *>(data);
	pdp->result = Response::DENY;

	on_done();
}

void show_popup(TpkpPopup *pdp)
{
	if (pdp == nullptr) {
		SLOGE("pdp is nullptr");
		return;
	}

	SLOGD("Start to make popup");

	elm_policy_set(ELM_POLICY_QUIT, ELM_POLICY_QUIT_LAST_WINDOW_CLOSED);

	pdp->win = elm_win_add(nullptr,
			dgettext(PROJECT_NAME, "SID_TITLE_PUBLIC_KEY_MISMATCHED"),
			ELM_WIN_NOTIFICATION);

	elm_win_autodel_set(pdp->win, EINA_TRUE);
	elm_win_indicator_opacity_set(pdp->win, ELM_WIN_INDICATOR_TRANSLUCENT);
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
	elm_object_text_set(pdp->content, dgettext(PROJECT_NAME, "SID_CONTENT_PUBLIC_KEY_MISMATCHED"));

	evas_object_size_hint_weight_set(pdp->content, EVAS_HINT_EXPAND, 0.0);
	evas_object_size_hint_align_set(pdp->content, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_show(pdp->content);
	elm_box_pack_end(pdp->box, pdp->content);

	elm_object_part_content_set(pdp->popup, "default", pdp->box);

	pdp->buttonAllow = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->buttonAllow, "elm.swallow.content.button1");
	elm_object_text_set(pdp->buttonAllow, dgettext(PROJECT_NAME, "SID_BTN_ALLOW"));
	elm_object_part_content_set(pdp->popup, "button1", pdp->buttonAllow);
	evas_object_smart_callback_add(pdp->buttonAllow, "clicked", allow_answer, pdp);
	evas_object_show(pdp->buttonAllow);

	pdp->buttonDeny = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->buttonDeny, "elm.swallow.content.button2");
	elm_object_text_set(pdp->buttonDeny, dgettext(PROJECT_NAME, "SID_BTN_DENY"));
	elm_object_part_content_set(pdp->popup, "button2  ", pdp->buttonDeny);
	evas_object_smart_callback_add(pdp->buttonDeny, "clicked", deny_answer, pdp);
	evas_object_show(pdp->buttonDeny);

	SLOGD("elm_run start");

	elm_run();

	SLOGD("elm_shutdown()");
	elm_shutdown();
}

int waitForParent(TpkpPopup *pdp)
{
	int pipeIn = pdp->pipe->in();
	struct timeval timeout = {10L, 0L};
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(pipeIn, &readfds);

	int sel = select(pipeIn + 1, &readfds, nullptr, nullptr, &timeout);
	if (sel == -1) {
		SLOGE("Cannot get info from parent. Exit popup - ERROR (%d)", errno);
		return -1;
	} else if (sel == 0) {
		SLOGE("Timeout reached! Exit popup - ERROR");
		return -1;
	} else {
		SLOGD("Ready to read parent info!");
		return 0;
	}
}

/*
 *  child receive list
 *  - std::string hostname
 */
void deserialize(TpkpPopup *pdp, char *buf, ssize_t count)
{
	BinaryStream stream;
	stream.Write(count, static_cast<void *>(buf));

	std::string hostname;

	SLOGD("------- Deserialization -------");
	Deserialization::Deserialize(stream, hostname);

	SLOGD("hostname: %s", hostname.c_str());

	pdp->hostname = std::move(hostname);
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

int receive(TpkpPopup *pdp)
{
	constexpr int size = 1024;
	char buf[size];
	ssize_t count = 0;

	do {
		count = TEMP_FAILURE_RETRY(read(pdp->pipe->in(), buf, size));
	} while (count == 0);

	if (count < 0) {
		SLOGE("read returned a negative value (%d)", count);
		SLOGE("errno: %s", strerror(errno));
		SLOGE("Exit popup - ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	deserialize(pdp, buf, count);

	SLOGD("receive data from tpkp parent successfully");

	return static_cast<int>(PopupStatus::NO_ERROR);
}

int send(TpkpPopup *pdp)
{
	BinaryStream stream = serialize(pdp);

	int ret = TEMP_FAILURE_RETRY(write(pdp->pipe->out(), stream.data(), stream.size()));

	if (ret == -1) {
		SLOGE("Write to pipe failed!");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	SLOGD("send result to tpkp parent successfully");

	return static_cast<int>(PopupStatus::NO_ERROR);
}

int parseArgs(int argc, char **argv, TpkpPopup *pdp)
{
	if (argc < 3) {
		SLOGE("To few args passed in exec to popup-bin - should be at least 3:");
		SLOGE("(binary-name, pipeIn, pipeOut)");
		SLOGE("return ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	SLOGD("Passed args: %s, %s, %s", argv[0], argv[1], argv[2]);

	std::string pipeInStr(argv[1]);
	std::string pipeOutStr(argv[2]);
	int pipeIn = std::stoi(pipeInStr);
	int pipeOut = std::stoi(pipeOutStr);

	SLOGD("Parsed pipes: IN[%d] OUT[%d]", pipeIn, pipeOut);

	pdp->pipe.reset(new Pipe(pipeIn, pipeOut));

	return static_cast<int>(PopupStatus::NO_ERROR);
}

} // namespace anonymous

EAPI_MAIN
int elm_main(int argc, char **argv)
{
	TpkpPopup pd;
	TpkpPopup *pdp = &pd;

	SLOGD("############################ popup binary ################################");

	setlocale(LC_ALL, "");

	if (parseArgs(argc, argv, pdp) != static_cast<int>(PopupStatus::NO_ERROR))
		return static_cast<int>(PopupStatus::EXIT_ERROR);

	if (waitForParent(pdp) == -1)
		return static_cast<int>(PopupStatus::EXIT_ERROR);

	if (receive(pdp) != static_cast<int>(PopupStatus::NO_ERROR))
		return static_cast<int>(PopupStatus::EXIT_ERROR);

	show_popup(pdp);

	SLOGD("pdp->result : %d", static_cast<int>(pdp->result));

	if (send(pdp) != static_cast<int>(PopupStatus::NO_ERROR))
		return static_cast<int>(PopupStatus::EXIT_ERROR);

	SLOGD("tpkp-popup done successfully!");

	return static_cast<int>(PopupStatus::NO_ERROR);
}
ELM_MAIN()
