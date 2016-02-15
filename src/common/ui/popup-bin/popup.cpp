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
#include <libintl.h>
#include <sys/select.h>
#include <time.h>

#include <Elementary.h>

#include "ui/popup_common.h"
#include "tpkp_logger.h"

using namespace TPKP::UI;

namespace {

void initialize_popup_data(struct tpkp_popup_data *data)
{
	if (data == nullptr)
		return;

	data->hostname.clear();
	data->result = Response::ERROR;
	data->win = nullptr;
	data->popup = nullptr;
	data->box = nullptr;
	data->title = nullptr;
	data->content = nullptr;
	data->allow_button = nullptr;
	data->deny_button = nullptr;
}

void on_done(void)
{
	SLOGD("elm_exit()");
	elm_exit();
}

void allow_answer(void *data, Evas_Object * /* obj */, void * /* event_info */)
{
	SLOGD("allow_answer");

	if (data == nullptr) {
		SLOGE("data is nullptr");
		return;
	}

	struct tpkp_popup_data *pdp = static_cast<struct tpkp_popup_data *>(data);
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

	struct tpkp_popup_data *pdp = static_cast<struct tpkp_popup_data *>(data);
	pdp->result = Response::DENY;

	on_done();
}

void show_popup(struct tpkp_popup_data *pdp)
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

	pdp->allow_button = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->allow_button, "elm.swallow.content.button1");
	elm_object_text_set(pdp->allow_button, dgettext(PROJECT_NAME, "SID_BTN_ALLOW"));
	elm_object_part_content_set(pdp->popup, "button1", pdp->allow_button);
	evas_object_smart_callback_add(pdp->allow_button, "clicked", allow_answer, pdp);
	evas_object_show(pdp->allow_button);

	pdp->deny_button = elm_button_add(pdp->popup);
	elm_object_style_set(pdp->deny_button, "elm.swallow.content.button2");
	elm_object_text_set(pdp->deny_button, dgettext(PROJECT_NAME, "SID_BTN_DENY"));
	elm_object_part_content_set(pdp->popup, "button2  ", pdp->deny_button);
	evas_object_smart_callback_add(pdp->deny_button, "clicked", deny_answer, pdp);
	evas_object_show(pdp->deny_button);

	SLOGD("elm_run start");

	elm_run();

	SLOGD("elm_shutdown()");
	elm_shutdown();
}

static int wait_for_parent_info(int pipe_in)
{
	// wait for parameters from pipe_in
	// timeout is set for 10 seconds
	struct timeval timeout = {10L, 0L};
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(pipe_in, &readfds);

	int sel = select(pipe_in + 1, &readfds, nullptr, nullptr, &timeout);
	if (sel == -1) {
		SLOGE("Cannot get info from parent. Exit popup - ERROR (%d)", errno);
		close(pipe_in);
		return -1;
	} else if (sel == 0) {
		SLOGE("Timeout reached! Exit popup - ERROR");
		close(pipe_in);
		return -1;
	}

	return 0;
}

void deserialize(tpkp_popup_data *pdp, char *line, ssize_t line_length)
{
	BinaryStream stream;
	stream.Write(line_length, static_cast<void *>(line));

	std::string hostname;

	SLOGD("------- Deserialization -------");
	Deserialization::Deserialize(stream, hostname);

	SLOGD("hostname: %s", hostname.c_str());

	pdp->hostname = std::move(hostname);
}

} // namespace anonymous

EAPI_MAIN
int elm_main(int argc, char **argv)
{
	// int pipe_in and int pipe_out should be passed to Popup via args.

	struct tpkp_popup_data pd;
	struct tpkp_popup_data *pdp = &pd;

	initialize_popup_data(pdp);

	SLOGD("############################ popup binary ################################");

	setlocale(LC_ALL, "");

	if (argc < 3) {
		SLOGE("To few args passed in exec to popup-bin - should be at least 3:");
		SLOGE("(binary-name, pipe_in, pipe_out)");
		SLOGE("return ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	SLOGD("Passed args: %s, %s, %s", argv[0], argv[1], argv[2]);

	int pipe_in;
	int pipe_out;

	// Parsing args (pipe_in, pipe_out)
	if (sscanf(argv[1], "%d", &pipe_in) == 0) {
		SLOGE("Error while parsing pipe_in; return ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}
	if (sscanf(argv[2], "%d", &pipe_out) == 0) {
		SLOGE("Error while parsing pipe_out; return ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}
	SLOGD("Parsed pipes: IN: %d, OUT: %d", pipe_in, pipe_out);

	if (wait_for_parent_info(pipe_in) == -1) {
		close(pipe_out);
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	constexpr int buff_size = 1024;
	char line[buff_size];

	ssize_t count = 0;

	do {
		count = TEMP_FAILURE_RETRY(read(pipe_in, line, buff_size));
	} while (0 == count);

	SLOGD("Read bytes : %d", count);
	close(pipe_in);

	if (count < 0) {
		close(pipe_out);
		SLOGE("read returned a negative value (%d)", count);
		SLOGE("errno: %s", strerror(errno));
		SLOGE("Exit popup - ERROR");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	}

	deserialize(pdp, line, count);

	show_popup(pdp);

	SLOGD("pdp->result : %d", static_cast<int>(pdp->result));

	BinaryStream stream;
	Serialization::Serialize(stream, static_cast<int>(pdp->result));

	int ret = TEMP_FAILURE_RETRY(write(pipe_out, stream.data(), stream.size()));

	close(pipe_out);

	if (ret == -1) {
		SLOGE("Write to pipe failed!");
		return static_cast<int>(PopupStatus::EXIT_ERROR);
	} else {
		SLOGD("tpkp-popup done successfully!");
		return static_cast<int>(PopupStatus::NO_ERROR);
	}
}
ELM_MAIN()
