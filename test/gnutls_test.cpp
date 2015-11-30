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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <iostream>

#include <gnutls/gnutls.h>

#include <tpkp_gnutls.h>

#include <boost/test/unit_test.hpp>

#define SHA1_LENGTH 20

namespace {
const std::string targetUrl = "https://WwW.GooGle.cO.Kr";

const char certStr[] =
	"-----BEGIN CERTIFICATE-----\n"
	"MIIDfTCCAuagAwIBAgIDErvmMA0GCSqGSIb3DQEBBQUAME4xCzAJBgNVBAYTAlVT\n"
	"MRAwDgYDVQQKEwdFcXVpZmF4MS0wKwYDVQQLEyRFcXVpZmF4IFNlY3VyZSBDZXJ0\n"
	"aWZpY2F0ZSBBdXRob3JpdHkwHhcNMDIwNTIxMDQwMDAwWhcNMTgwODIxMDQwMDAw\n"
	"WjBCMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNR2VvVHJ1c3QgSW5jLjEbMBkGA1UE\n"
	"AxMSR2VvVHJ1c3QgR2xvYmFsIENBMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIB\n"
	"CgKCAQEA2swYYzD99BcjGlZ+W988bDjkcbd4kdS8odhM+KhDtgPpTSEHCIjaWC9m\n"
	"OSm9BXiLnTjoBbdqfnGk5sRgprDvgOSJKA+eJdbtg/OtppHHmMlCGDUUna2YRpIu\n"
	"T8rxh0PBFpVXLVDviS2Aelet8u5fa9IAjbkU+BQVNdnARqN7csiRv8lVK83Qlz6c\n"
	"JmTM386DGXHKTubU1XupGc1V3sjs0l44U+VcT4wt/lAjNvxm5suOpDkZALeVAjmR\n"
	"Cw7+OC7RHQWa9k0+bw8HHa8sHo9gOeL6NlMTOdReJivbPagUvTLrGAMoUgRx5asz\n"
	"PeE4uwc2hGKceeoWMPRfwCvocWvk+QIDAQABo4HwMIHtMB8GA1UdIwQYMBaAFEjm\n"
	"aPkr0rKV10fYIyAQTzOYkJ/UMB0GA1UdDgQWBBTAephojYn7qwVkDBF9qn1luMrM\n"
	"TjAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBBjA6BgNVHR8EMzAxMC+g\n"
	"LaArhilodHRwOi8vY3JsLmdlb3RydXN0LmNvbS9jcmxzL3NlY3VyZWNhLmNybDBO\n"
	"BgNVHSAERzBFMEMGBFUdIAAwOzA5BggrBgEFBQcCARYtaHR0cHM6Ly93d3cuZ2Vv\n"
	"dHJ1c3QuY29tL3Jlc291cmNlcy9yZXBvc2l0b3J5MA0GCSqGSIb3DQEBBQUAA4GB\n"
	"AHbhEm5OSxYShjAGsoEIz/AIx8dxfmbuwu3UOx//8PDITtZDOLC5MH0Y0FWDomrL\n"
	"NhGc6Ehmo21/uBPUR/6LWlxz/K7ZGzIZOKuXNBSqltLroxwUCEm2u+WR74M26x1W\n"
	"b8ravHNjkOR/ez4iyz0H7V84dJzjA1BOoa+Y7mHyhD8S\n"
	"-----END CERTIFICATE-----\n";

const char sha1hash[] = // kSPKIHash_GeoTrustGlobal
	    "\xc0\x7a\x98\x68\x8d\x89\xfb\xab\x05\x64"
	    "\x0c\x11\x7d\xaa\x7d\x65\xb8\xca\xcc\x4e";

int tcp_connect(const char *ip, int port)
{
	int sock;
	struct sockaddr_in server_addr;

	BOOST_REQUIRE_MESSAGE(
		(sock = socket(PF_INET, SOCK_STREAM, 0)) >= 0,
		"Cannot create socket!");

	memset(&server_addr, 0x00, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	BOOST_REQUIRE_MESSAGE(
		(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr))) >= 0,
		"Cannot connect to sock:" << sock);

	return sock;
}

void tcp_close(int sd)
{
	close(sd);
}

typedef struct _GIOGnuTLSChannel GIOGnuTLSChannel;

struct _GIOGnuTLSChannel {
    int fd;
};

gnutls_session_t makeDefaultSession(void)
{
	int ret = gnutls_global_init();
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls global init: " << gnutls_strerror(ret));

	gnutls_session_t session;
	ret = gnutls_init(&session, GNUTLS_CLIENT);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls init session: " << gnutls_strerror(ret));

	ret = gnutls_set_default_priority(session);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to set default priority on session: " << gnutls_strerror(ret));

	return session;
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

}

BOOST_AUTO_TEST_SUITE(TPKP_GNUTLS_TEST)

#define MAX_BUF 1024
#define MSG "GET / HTTP/1.0\r\n\r\n"
BOOST_AUTO_TEST_CASE(T00101_positive)
{
	gnutls_certificate_credentials_t cred = makeDefaultCred(&tpkp_gnutls_verify_callback);
	gnutls_session_t session = makeDefaultSession();

	int ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cred);
	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Failed to gnutls_credentials_set: " << gnutls_strerror(ret));

	int sd = tcp_connect("216.58.221.36", 443);

	BOOST_REQUIRE_MESSAGE(
		tpkp_gnutls_set_url_data(targetUrl.c_str()) == TPKP_E_NONE,
		"Failed to tpkp_gnutls_set_url_data.");

	gnutls_transport_set_int(session, sd);
	gnutls_handshake_set_timeout(session, GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

	do {
		ret = gnutls_handshake(session);
	} while (ret != GNUTLS_E_SUCCESS && gnutls_error_is_fatal(ret) == 0);

	BOOST_REQUIRE_MESSAGE(
		ret == GNUTLS_E_SUCCESS,
		"Handshake failed: " << gnutls_strerror(ret));

	gnutls_bye(session, GNUTLS_SHUT_RDWR);

    tcp_close(sd);

    gnutls_certificate_free_credentials(cred);

    gnutls_deinit(session);
    gnutls_global_deinit();
}

BOOST_AUTO_TEST_SUITE_END()
