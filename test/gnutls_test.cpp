/*
 * Copyright (c) 2015 Samsung Electronics Co., Ltd All Rights Reserved
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
 * @file        gnutls_sample.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       tpkp_gnutls unit test.
 */
#include <iostream>
#include <vector>
#include <string>
#include <thread>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <gnutls/gnutls.h>
#include <tpkp_gnutls.h>
#include <boost/test/unit_test.hpp>

namespace {

struct DataSet {
	gnutls_session_t session;
	gnutls_certificate_credentials_t cred;
	int sockfd;
};

static std::vector<std::string> s_urlList = {
	"www.google.com",
	"www.youtube.com",
	"www.spideroak.com",
	"www.facebook.com",
	"www.dropbox.com",
	"www.twitter.com",
	"www.hackerrank.com", /* no pinned data exist */
	"www.algospot.com"    /* no pinned data exist */
};

void connectWithUrl(const std::string &url, int &sockfd)
{
	struct addrinfo *result;
	struct addrinfo hints;
	memset(&hints, 0x00, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	int s = getaddrinfo(url.c_str(), "https", &hints, &result);
	BOOST_REQUIRE_MESSAGE(s == 0, "getaddrinfo err code: " << s << " desc: " << gai_strerror(s));

	struct addrinfo *rp;
	for (rp = result; rp != nullptr; rp = rp->ai_next) {
		sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sockfd == -1)
			continue;

		if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			char *ipaddr = inet_ntoa(*((struct in_addr *)rp->ai_addr));
			std::cout << "url: " << url << " connected with addr: " << ipaddr << std::endl;
			break;
		}

		close(sockfd);
	}

	BOOST_REQUIRE_MESSAGE(rp != nullptr, "Could not connect on url: " << url);

	std::cout << "url[" << url << "] canonname[" << result->ai_canonname << "] connected!" << std::endl;

	freeaddrinfo(result);
}

inline gnutls_certificate_credentials_t makeDefaultCred(gnutls_certificate_verify_function *verify_callback)
{
	gnutls_certificate_credentials_t cred;

	int ret = gnutls_certificate_allocate_credentials(&cred);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls_certificate_allocate_credentials: " << gnutls_strerror(ret));

	ret = gnutls_certificate_set_x509_trust_file(cred, "/etc/ssl/ca-bundle.pem", GNUTLS_X509_FMT_PEM);
	BOOST_REQUIRE_MESSAGE(
		ret > 0,
		"Failed to gnutls_certificate_set_x509_trust_file ret: " << ret);
	std::cout << "x509 trust file loaded. cert num: " << ret << std::endl;

	gnutls_certificate_set_verify_function(cred, verify_callback);

	return cred;
}

DataSet makeDefaultSession(const std::string &url)
{
	DataSet data;

	data.cred = makeDefaultCred(&tpkp_gnutls_verify_callback);

	int ret = gnutls_init(&data.session, GNUTLS_CLIENT);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls init session: " << gnutls_strerror(ret));

	ret = gnutls_set_default_priority(data.session);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to set default priority on session: " << gnutls_strerror(ret));

	ret = gnutls_credentials_set(data.session, GNUTLS_CRD_CERTIFICATE, data.cred);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls_credentials_set: " << gnutls_strerror(ret));

	connectWithUrl(url, data.sockfd);

	BOOST_REQUIRE_MESSAGE(
		tpkp_gnutls_set_url_data(url.c_str()) == TPKP_E_NONE,
		"Failed to tpkp_gnutls_set_url_data.");

	gnutls_transport_set_int(data.session, data.sockfd);
	gnutls_handshake_set_timeout(data.session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

	return data;
}

DataSet makeSessionWithoutPinning(const std::string &url)
{
	DataSet data;

	int ret = gnutls_certificate_allocate_credentials(&data.cred);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls_certificate_allocate_credentials: " << gnutls_strerror(ret));

	ret = gnutls_init(&data.session, GNUTLS_CLIENT);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls init session: " << gnutls_strerror(ret));

	ret = gnutls_set_default_priority(data.session);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to set default priority on session: " << gnutls_strerror(ret));

	ret = gnutls_credentials_set(data.session, GNUTLS_CRD_CERTIFICATE, data.cred);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls_credentials_set: " << gnutls_strerror(ret));

	connectWithUrl(url, data.sockfd);

	gnutls_transport_set_int(data.session, data.sockfd);
	gnutls_handshake_set_timeout(data.session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

	return data;
}

DataSet makeDefaultSessionGlobal(const std::string &url)
{
	int ret = gnutls_global_init();
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls global init: " << gnutls_strerror(ret));

	return makeDefaultSession(url);
}

void performHandshake(DataSet &data)
{
	int ret;
	do {
		ret = gnutls_handshake(data.session);
	} while (ret != GNUTLS_E_SUCCESS && gnutls_error_is_fatal(ret) == 0);

	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Handshake failed! err code: " << ret << " desc: " << gnutls_strerror(ret));
}

void cleanup(DataSet &data)
{
	gnutls_bye(data.session, GNUTLS_SHUT_RDWR);
	close(data.sockfd);
	gnutls_certificate_free_credentials(data.cred);
	gnutls_deinit(data.session);

	tpkp_gnutls_cleanup();
}

void cleanupGlobal(DataSet &data)
{
	cleanup(data);
	gnutls_global_deinit();
}

void perform(const std::string &url)
{
	DataSet data = makeDefaultSession(url);
	performHandshake(data);
	cleanup(data);
}

void performWithoutPinning(const std::string &url)
{
	DataSet data = makeSessionWithoutPinning(url);
	performHandshake(data);
	cleanup(data);
}

}

BOOST_AUTO_TEST_SUITE(TPKP_GNUTLS_TEST)

BOOST_AUTO_TEST_CASE(T00101_positive_1)
{
	gnutls_global_init();

	perform(s_urlList[0]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00102_positive_2)
{
	gnutls_global_init();

	perform(s_urlList[1]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00103_positive_3)
{
	gnutls_global_init();

	perform(s_urlList[2]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00104_positive_4)
{
	gnutls_global_init();

	perform(s_urlList[3]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00105_positive_5)
{
	gnutls_global_init();

	perform(s_urlList[4]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00106_positive_6)
{
	gnutls_global_init();

	perform(s_urlList[5]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00107_positive_7)
{
	gnutls_global_init();

	perform(s_urlList[6]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00108_positive_8)
{
	gnutls_global_init();

	perform(s_urlList[7]);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00109_positive_all_single_thread)
{
	gnutls_global_init();

	for (const auto &url : s_urlList)
		perform(url);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_CASE(T00110_positive_all_single_thread_without_pinning)
{
	gnutls_global_init();

	for (const auto &url : s_urlList)
		performWithoutPinning(url);

	gnutls_global_deinit();
}

BOOST_AUTO_TEST_SUITE_END()
