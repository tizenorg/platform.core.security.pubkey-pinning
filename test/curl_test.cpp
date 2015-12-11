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
 * @file        libcurl_sample.cpp
 * @author      Kyungwook Tak (k.tak@samsung.com)
 * @version     1.0
 * @brief       tpkp_curl unit test.
 */
#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/sha.h>
#include <openssl/x509.h>
#include <curl/curl.h>
#include <thread>
#include <vector>

#include <tpkp_curl.h>

#include <boost/test/unit_test.hpp>

static std::vector<std::string> UrlList = {
	"https://www.google.com",
	"https://www.facebook.com",
	"https://www.twitter.com",
	"https://www.dropbox.com",
	"https://www.spideroak.com",
	"https://www.youtube.com",
	"https://thehackernews.com" /* no static pinned data */
};

const std::string targetUrl        = "https://WwW.GooGle.cO.Kr";
const std::string targetInvalidUrl = "https://WwW.GooGle.cO.Kr11143343jiuj::";

int verify_callback(int preverify_ok, X509_STORE_CTX *x509_ctx)
{
	if (preverify_ok == 0)
		return 0;

	/*
	 *  Do something which isn't related with HPKP here
	 *  And update value to preverify_ok of validation result
	 */

	/* call tpkp_verify_callback as additional step */
	return tpkp_curl_verify_callback(preverify_ok, x509_ctx);
}

static CURLcode ssl_ctx_callback_set_verify(CURL *curl, void *ssl_ctx, void *userptr)
{
	(void)userptr;

	SSL_CTX_set_verify((SSL_CTX *)ssl_ctx, SSL_VERIFY_PEER, verify_callback);
	tpkp_e res = tpkp_curl_set_url_data(curl);
	if (res != TPKP_E_NONE)
		return CURLE_FAILED_INIT;

	return CURLE_OK;
}

static CURLcode ssl_ctx_callback_not_set_verify(CURL *curl, void *ssl_ctx, void *userptr)
{
	(void)userptr;

	tpkp_e res = tpkp_curl_set_verify(curl, (SSL_CTX *)ssl_ctx);
	if (res != TPKP_E_NONE)
		return CURLE_FAILED_INIT;

	return CURLE_OK;
}

static CURL *makeLocalDefaultHandle(std::string url)
{
	CURL *handle = curl_easy_init();

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_URL, url.c_str()) == CURLE_OK,
		"Failed to set opt url : " << url);

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_VERBOSE, 0L) == CURLE_OK,
		"Failed to set opt verbose");

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L) == CURLE_OK,
		"Failed to set opt verify peer");

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 2L) == CURLE_OK,
		"Failed to set opt verify host");

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L) == CURLE_OK,
		"Failed to set opt follow location");

	BOOST_REQUIRE_MESSAGE(
		curl_easy_setopt(handle, CURLOPT_NOBODY, 1L) == CURLE_OK,
		"Failed to set opt no body");

	return handle;
}

static CURL *makeDefaultHandle(std::string url)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	return makeLocalDefaultHandle(url);
}

static void performWithUrl(std::string url)
{
	CURL *curl = makeLocalDefaultHandle(url);
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	char *ip = nullptr;
	long port = 0;
	res = curl_easy_getinfo(curl, CURLINFO_PRIMARY_IP, &ip);
	BOOST_REQUIRE_MESSAGE(res == CURLE_OK, "Failed to getinfo of ip: " << curl_easy_strerror(res));
	res = curl_easy_getinfo(curl, CURLINFO_PRIMARY_PORT, &port);
	BOOST_REQUIRE_MESSAGE(res == CURLE_OK, "Failed to getinfo of port: " << curl_easy_strerror(res));
	std::cout << "url: " << url << " ip: " << ip << " port: " << port << std::endl;

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
}

BOOST_AUTO_TEST_SUITE(TPKP_CURL_TEST)

BOOST_AUTO_TEST_CASE(T00101_posivite_notusing_ssl_ctx_func_opt)
{
	CURL *curl = makeDefaultHandle(targetUrl);
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00102_posivite_using_ssl_ctx_func_opt_notusing_ssl_ctx_set_verify)
{
	CURL *curl = makeDefaultHandle(targetUrl);
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback_not_set_verify);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00103_posivite_using_ssl_ctx_func_opt_using_ssl_ctx_set_verify)
{
	CURL *curl = makeDefaultHandle(targetUrl);
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback_set_verify);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00104_negative_invalid_url)
{
	CURL *curl = makeDefaultHandle(targetInvalidUrl);
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_ctx_callback_set_verify);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res != CURLE_OK,
		"Shouldnot success perform curl: " << curl_easy_strerror(res));
	std::cout << "code: " << res << " description: " << curl_easy_strerror(res) << std::endl;

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00105_positive_facebook_with_https)
{
	CURL *curl = makeDefaultHandle("https://www.facebook.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00106_positive_facebook_with_http)
{
	CURL *curl = makeDefaultHandle("http://www.facebook.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00107_positive_facebook_with_hostname)
{
	CURL *curl = makeDefaultHandle("www.facebook.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00108_positive_twitter_with_https)
{
	CURL *curl = makeDefaultHandle("https://www.twitter.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00109_positive_dropbox_with_https)
{
	CURL *curl = makeDefaultHandle("https://www.dropbox.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00110_positive_spideroak_with_https)
{
	CURL *curl = makeDefaultHandle("https://www.spideroak.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00111_positive_https_but_no_pinned_data_youtube)
{
	CURL *curl = makeDefaultHandle("https://www.youtube.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00112_positive_https_but_no_pinned_data_hackernews)
{
	CURL *curl = makeDefaultHandle("https://thehackernews.com");
	CURLcode res;

	res = curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, tpkp_curl_ssl_ctx_callback);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to set opt ssl ctx function. code: " << curl_easy_strerror(res));

	res = curl_easy_perform(curl);
	BOOST_REQUIRE_MESSAGE(
		res == CURLE_OK,
		"Failed to perform curl: " << curl_easy_strerror(res));

	tpkp_curl_cleanup();
	curl_easy_cleanup(curl);
	curl_global_cleanup();

}

BOOST_AUTO_TEST_CASE(T00113_positive_threads)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	std::vector<std::thread> threads;

	for (const auto &url : UrlList)
		threads.emplace_back(performWithUrl, url);

	for (auto &t : threads)
		t.join();

	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00114_positive_threads_2times)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	std::vector<std::thread> threads;

	for (int i = 0; i < 2; i++) {
		for (const auto &url : UrlList)
			threads.emplace_back(performWithUrl, url);
	}

	for (auto &t : threads)
		t.join();

	curl_global_cleanup();
}

BOOST_AUTO_TEST_CASE(T00113_positive_threads_3times)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);

	std::vector<std::thread> threads;

	for (int i = 0; i < 3; i++) {
		for (const auto &url : UrlList)
			threads.emplace_back(performWithUrl, url);
	}

	for (auto &t : threads)
		t.join();

	curl_global_cleanup();
}

BOOST_AUTO_TEST_SUITE_END()
