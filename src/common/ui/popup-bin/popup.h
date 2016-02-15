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
 * @file        popup.h
 * @author      Janusz Kozerski (j.kozerski@samsung.com)
 * @version     1.0
 */
#pragma once

#include <string>
#include <memory>

#include <Elementary.h>

#include "tpkp_logger.h"
#include "ui/popup_common.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "TPKP_POPUP"

class Pipe {
public:
	explicit Pipe(int in, int out);
	virtual ~Pipe();
	int in() const;
	int out() const;

private:
	int m_in;
	int m_out;
};

struct TpkpPopup {
	std::string hostname;
	TPKP::UI::Response result;
	std::unique_ptr<Pipe> pipe;

	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *box;
	Evas_Object *title;
	Evas_Object *content;
	Evas_Object *buttonAllow;
	Evas_Object *buttonDeny;

	TpkpPopup() :
		hostname(),
		result(TPKP::UI::Response::ERROR),
		pipe(nullptr),
		win(nullptr),
		popup(nullptr),
		box(nullptr),
		title(nullptr),
		content(nullptr),
		buttonAllow(nullptr),
		buttonDeny(nullptr) {}
};
