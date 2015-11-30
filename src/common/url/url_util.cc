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
#include "url/url_util.h"

#include <string.h>
#include <vector>

#include "base/logging.h"
#include "url/url_util_internal.h"

namespace url {

namespace {

// ASCII-specific tolower.  The standard library's tolower is locale sensitive,
// so we don't want to use it here.
template<class Char>
inline Char ToLowerASCII(Char c) {
  return (c >= 'A' && c <= 'Z') ? (c + ('a' - 'A')) : c;
}

// Backend for LowerCaseEqualsASCII.
template<typename Iter>
inline bool DoLowerCaseEqualsASCII(Iter a_begin, Iter a_end, const char* b) {
  for (Iter it = a_begin; it != a_end; ++it, ++b) {
    if (!*b || ToLowerASCII(*it) != *b)
      return false;
  }
  return *b == 0;
}

const int kNumStandardURLSchemes = 9;
const char* kStandardURLSchemes[kNumStandardURLSchemes] = {
  kHttpScheme,
  kHttpsScheme,
  kFileScheme,  // Yes, file urls can have a hostname!
  kFtpScheme,
  kGopherScheme,
  kWsScheme,    // WebSocket.
  kWssScheme,   // WebSocket secure.
  kFileSystemScheme,
  "rtsp",
};

// List of the currently installed standard schemes. This list is lazily
// initialized by InitStandardSchemes and is leaked on shutdown to prevent
// any destructors from being called that will slow us down or cause problems.
std::vector<const char*>* standard_schemes = NULL;

// See the LockStandardSchemes declaration in the header.
bool standard_schemes_locked = false;

// Ensures that the standard_schemes list is initialized, does nothing if it
// already has values.
void InitStandardSchemes() {
  if (standard_schemes)
    return;
  standard_schemes = new std::vector<const char*>;
  for (int i = 0; i < kNumStandardURLSchemes; i++)
    standard_schemes->push_back(kStandardURLSchemes[i]);
}

// Given a string and a range inside the string, compares it to the given
// lower-case |compare_to| buffer.
template<typename CHAR>
inline bool DoCompareSchemeComponent(const CHAR* spec,
                                     const Component& component,
                                     const char* compare_to) {
  if (!component.is_nonempty())
    return compare_to[0] == 0;  // When component is empty, match empty scheme.
  return LowerCaseEqualsASCII(&spec[component.begin],
                              &spec[component.end()],
                              compare_to);
}

// Returns true if the given scheme identified by |scheme| within |spec| is one
// of the registered "standard" schemes.
template<typename CHAR>
bool DoIsStandard(const CHAR* spec, const Component& scheme) {
  if (!scheme.is_nonempty())
    return false;  // Empty or invalid schemes are non-standard.

  InitStandardSchemes();
  for (size_t i = 0; i < standard_schemes->size(); i++) {
    if (LowerCaseEqualsASCII(&spec[scheme.begin], &spec[scheme.end()],
                             standard_schemes->at(i)))
      return true;
  }
  return false;
}


}  // namespace

void Initialize() {
  InitStandardSchemes();
}

void Shutdown() {
  if (standard_schemes) {
    delete standard_schemes;
    standard_schemes = NULL;
  }
}

void LockStandardSchemes() {
  standard_schemes_locked = true;
}



// Front-ends for LowerCaseEqualsASCII.
bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}

bool LowerCaseEqualsASCII(const char* a_begin,
                          const char* a_end,
                          const char* b_begin,
                          const char* b_end) {
  while (a_begin != a_end && b_begin != b_end &&
         ToLowerASCII(*a_begin) == *b_begin) {
    a_begin++;
    b_begin++;
  }
  return a_begin == a_end && b_begin == b_end;
}

bool LowerCaseEqualsASCII(const base::char16* a_begin,
                          const base::char16* a_end,
                          const char* b) {
  return DoLowerCaseEqualsASCII(a_begin, a_end, b);
}


//=========================================

bool IsStandard(const char* spec, const Component& scheme) {
  return DoIsStandard(spec, scheme);
}

bool IsStandard(const base::char16* spec, const Component& scheme) {
  return DoIsStandard(spec, scheme);
}

bool CompareSchemeComponent(const char* spec,
                            const Component& component,
                            const char* compare_to) {
  return DoCompareSchemeComponent(spec, component, compare_to);
}

bool CompareSchemeComponent(const base::char16* spec,
                            const Component& component,
                            const char* compare_to) {
  return DoCompareSchemeComponent(spec, component, compare_to);
}

}  // namespace url
