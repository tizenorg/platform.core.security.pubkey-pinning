/*
 * Copyright 2014 The Chromium Authors. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:

 *    * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *    * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include <string>

#include "base/strings/string16.h"
#include "url/url_constants.h"
#include "url/url_export.h"
#include "url/third_party/mozilla/url_parse.h"

namespace url {

// Returns true if the given string represents a standard URL. This means that
// either the scheme is in the list of known standard schemes.
URL_EXPORT bool IsStandard(const char* spec, const Component& scheme);
URL_EXPORT bool IsStandard(const base::char16* spec, const Component& scheme);

// TODO(brettw) remove this. This is a temporary compatibility hack to avoid
// breaking the WebKit build when this version is synced via Chrome.
inline bool IsStandard(const char* spec,
                       int spec_len,
                       const Component& scheme) {
  return IsStandard(spec, scheme);
}

// Compare the lower-case form of the given string against the given ASCII
// string.  This is useful for doing checking if an input string matches some
// token, and it is optimized to avoid intermediate string copies.
//
// The versions of this function that don't take a b_end assume that the b
// string is NULL terminated.
URL_EXPORT bool LowerCaseEqualsASCII(const char* a_begin,
                                     const char* a_end,
                                     const char* b);
URL_EXPORT bool LowerCaseEqualsASCII(const char* a_begin,
                                     const char* a_end,
                                     const char* b_begin,
                                     const char* b_end);
URL_EXPORT bool LowerCaseEqualsASCII(const base::char16* a_begin,
                                     const base::char16* a_end,
                                     const char* b);


}  // namespace url
