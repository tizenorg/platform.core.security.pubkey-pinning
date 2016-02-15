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
 * @file        popup_runner_test.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       popup runner and popup user service communication test
 */
#include "ui/popup_runner.h"

#include <iostream>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(TPKP_POPUP_RUNNER_TEST)

BOOST_AUTO_TEST_CASE(T00101_positive)
{
	TPKP::UI::Response response = TPKP::UI::runPopup("test_hostname", 10);
	switch (response) {
	case TPKP::UI::Response::ALLOW:
		std::cout << "###################################################" << std::endl;
		std::cout << "##############                        #############" << std::endl;
		std::cout << "##############  ALLOW BUTTON PRESSED  #############" << std::endl;
		std::cout << "##############                        #############" << std::endl;
		std::cout << "###################################################" << std::endl;
		break;
	case TPKP::UI::Response::DENY:
		std::cout << "###################################################" << std::endl;
		std::cout << "##############                        #############" << std::endl;
		std::cout << "##############  DENY BUTTON PRESSED   #############" << std::endl;
		std::cout << "##############                        #############" << std::endl;
		std::cout << "###################################################" << std::endl;
		break;
	default:
		std::cerr << "###################################################" << std::endl;
		std::cerr << "##############                        #############" << std::endl;
		std::cerr << "##############  UNKNOWN    RESPONSE   #############" << std::endl;
		std::cerr << "##############                        #############" << std::endl;
		std::cerr << "###################################################" << std::endl;
		break;
	}

	BOOST_REQUIRE_MESSAGE(
		response != TPKP::UI::Response::ERROR,
		"Unknown response from popup user service!");
}

BOOST_AUTO_TEST_SUITE_END()
