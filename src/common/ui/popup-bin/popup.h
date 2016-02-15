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
#include <Elementary.h>

#include "ui/popup_common.h"

struct tpkp_popup_data {
	std::string hostname;
	TPKP::UI::Response result;

	Evas_Object *win;
	Evas_Object *popup;
	Evas_Object *box;
	Evas_Object *title;
	Evas_Object *content;
	Evas_Object *allow_button;
	Evas_Object *deny_button;
};
