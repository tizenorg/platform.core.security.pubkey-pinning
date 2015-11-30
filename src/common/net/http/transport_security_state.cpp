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

// This file is subset of chromium-efl/net/http/transport_security_state.cc

#include "net/http/transport_security_state.h"
#include "net/http/transport_security_state_static.h"
#include "base/logging.h"

namespace {

// BitReader is a class that allows a bytestring to be read bit-by-bit.
class BitReader {
 public:
  BitReader(const uint8* bytes, size_t num_bits)
      : bytes_(bytes),
        num_bits_(num_bits),
        num_bytes_((num_bits + 7) / 8),
        current_byte_index_(0),
        num_bits_used_(8) {}

  // Next sets |*out| to the next bit from the input. It returns false if no
  // more bits are available or true otherwise.
  bool Next(bool* out) {
    if (num_bits_used_ == 8) {
      if (current_byte_index_ >= num_bytes_) {
        return false;
      }
      current_byte_ = bytes_[current_byte_index_++];
      num_bits_used_ = 0;
    }

    *out = 1 & (current_byte_ >> (7 - num_bits_used_));
    num_bits_used_++;
    return true;
  }

  // Read sets the |num_bits| least-significant bits of |*out| to the value of
  // the next |num_bits| bits from the input. It returns false if there are
  // insufficient bits in the input or true otherwise.
  bool Read(unsigned num_bits, uint32* out) {
    DCHECK_LE(num_bits, 32u);

    uint32 ret = 0;
    for (unsigned i = 0; i < num_bits; ++i) {
      bool bit;
      if (!Next(&bit)) {
        return false;
      }
      ret |= static_cast<uint32>(bit) << (num_bits - 1 - i);
    }

    *out = ret;
    return true;
  }

  // Unary sets |*out| to the result of decoding a unary value from the input.
  // It returns false if there were insufficient bits in the input and true
  // otherwise.
  bool Unary(size_t* out) {
    size_t ret = 0;

    for (;;) {
      bool bit;
      if (!Next(&bit)) {
        return false;
      }
      if (!bit) {
        break;
      }
      ret++;
    }

    *out = ret;
    return true;
  }

  // Seek sets the current offest in the input to bit number |offset|. It
  // returns true if |offset| is within the range of the input and false
  // otherwise.
  bool Seek(size_t offset) {
    if (offset >= num_bits_) {
      return false;
    }
    current_byte_index_ = offset / 8;
    current_byte_ = bytes_[current_byte_index_++];
    num_bits_used_ = offset % 8;
    return true;
  }

 private:
  const uint8* const bytes_;
  const size_t num_bits_;
  const size_t num_bytes_;
  // current_byte_index_ contains the current byte offset in |bytes_|.
  size_t current_byte_index_;
  // current_byte_ contains the current byte of the input.
  uint8 current_byte_;
  // num_bits_used_ contains the number of bits of |current_byte_| that have
  // been read.
  unsigned num_bits_used_;
};

// HuffmanDecoder is a very simple Huffman reader. The input Huffman tree is
// simply encoded as a series of two-byte structures. The first byte determines
// the "0" pointer for that node and the second the "1" pointer. Each byte
// either has the MSB set, in which case the bottom 7 bits are the value for
// that position, or else the bottom seven bits contain the index of a node.
//
// The tree is decoded by walking rather than a table-driven approach.
class HuffmanDecoder {
 public:
  HuffmanDecoder(const uint8* tree, size_t tree_bytes)
      : tree_(tree),
        tree_bytes_(tree_bytes) {}

  bool Decode(BitReader* reader, char* out) {
    const uint8* current = &tree_[tree_bytes_-2];

    for (;;) {
      bool bit;
      if (!reader->Next(&bit)) {
        return false;
      }

      uint8 b = current[bit];
      if (b & 0x80) {
        *out = static_cast<char>(b & 0x7f);
        return true;
      }

      unsigned offset = static_cast<unsigned>(b) * 2;
      DCHECK_LT(offset, tree_bytes_);
      if (offset >= tree_bytes_) {
        return false;
      }

      current = &tree_[offset];
    }
  }

 private:
  const uint8* const tree_;
  const size_t tree_bytes_;
};

} // namespace anonymous

namespace net {

// DecodeHSTSPreloadRaw resolves |hostname| in the preloaded data. It returns
// false on internal error and true otherwise. After a successful return,
// |*out_found| is true iff a relevant entry has been found. If so, |*out|
// contains the details.
//
// Don't call this function, call DecodeHSTSPreload, below.
//
// Although this code should be robust, it never processes attacker-controlled
// data -- it only operates on the preloaded data built into the binary.
//
// The preloaded data is represented as a trie and matches the hostname
// backwards. Each node in the trie starts with a number of characters, which
// must match exactly. After that is a dispatch table which maps the next
// character in the hostname to another node in the trie.
//
// In the dispatch table, the zero character represents the "end of string"
// (which is the *beginning* of a hostname since we process it backwards). The
// value in that case is special -- rather than an offset to another trie node,
// it contains the HSTS information: whether subdomains are included, pinsets
// etc. If an "end of string" matches a period in the hostname then the
// information is remembered because, if no more specific node is found, then
// that information applies to the hostname.
//
// Dispatch tables are always given in order, but the "end of string" (zero)
// value always comes before an entry for '.'.
bool DecodeHSTSPreloadRaw(const std::string& hostname,
                          bool* out_found,
                          PreloadResult* out) {
  HuffmanDecoder huffman(kHSTSHuffmanTree, sizeof(kHSTSHuffmanTree));
  BitReader reader(kPreloadedHSTSData, kPreloadedHSTSBits);
  size_t bit_offset = kHSTSRootPosition;
  static const char kEndOfString = 0;
  static const char kEndOfTable = 127;

  *out_found = false;

  if (hostname.empty()) {
    return true;
  }
  // hostname_offset contains one more than the index of the current character
  // in the hostname that is being considered. It's one greater so that we can
  // represent the position just before the beginning (with zero).
  size_t hostname_offset = hostname.size();

  for (;;) {
    // Seek to the desired location.
    if (!reader.Seek(bit_offset)) {
      return false;
    }

    // Decode the unary length of the common prefix.
    size_t prefix_length;
    if (!reader.Unary(&prefix_length)) {
      return false;
    }

    // Match each character in the prefix.
    for (size_t i = 0; i < prefix_length; ++i) {
      if (hostname_offset == 0) {
        // We can't match the terminator with a prefix string.
        return true;
      }

      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (hostname[hostname_offset - 1] != c) {
        return true;
      }
      hostname_offset--;
    }

    bool is_first_offset = true;
    size_t current_offset = 0;

    // Next is the dispatch table.
    for (;;) {
      char c;
      if (!huffman.Decode(&reader, &c)) {
        return false;
      }
      if (c == kEndOfTable) {
        // No exact match.
        return true;
      }

      if (c == kEndOfString) {
        PreloadResult tmp;
        if (!reader.Next(&tmp.sts_include_subdomains) ||
            !reader.Next(&tmp.force_https) ||
            !reader.Next(&tmp.has_pins)) {
          return false;
        }

        tmp.pkp_include_subdomains = tmp.sts_include_subdomains;

        if (tmp.has_pins) {
          if (!reader.Read(4, &tmp.pinset_id) ||
              !reader.Read(9, &tmp.domain_id) ||
              (!tmp.sts_include_subdomains &&
               !reader.Next(&tmp.pkp_include_subdomains))) {
            return false;
          }
        }

        tmp.hostname_offset = hostname_offset;

        if (hostname_offset == 0 || hostname[hostname_offset - 1] == '.') {
          *out_found =
              tmp.sts_include_subdomains || tmp.pkp_include_subdomains;
          *out = tmp;

          if (hostname_offset > 0) {
            out->force_https &= tmp.sts_include_subdomains;
          } else {
            *out_found = true;
            return true;
          }
        }

        continue;
      }

      // The entries in a dispatch table are in order thus we can tell if there
      // will be no match if the current character past the one that we want.
      if (hostname_offset == 0 || hostname[hostname_offset-1] < c) {
        return true;
      }

      if (is_first_offset) {
        // The first offset is backwards from the current position.
        uint32 jump_delta_bits;
        uint32 jump_delta;
        if (!reader.Read(5, &jump_delta_bits) ||
            !reader.Read(jump_delta_bits, &jump_delta)) {
          return false;
        }

        if (bit_offset < jump_delta) {
          return false;
        }

        current_offset = bit_offset - jump_delta;
        is_first_offset = false;
      } else {
        // Subsequent offsets are forward from the target of the first offset.
        uint32 is_long_jump;
        if (!reader.Read(1, &is_long_jump)) {
          return false;
        }

        uint32 jump_delta;
        if (!is_long_jump) {
          if (!reader.Read(7, &jump_delta)) {
            return false;
          }
        } else {
          uint32 jump_delta_bits;
          if (!reader.Read(4, &jump_delta_bits) ||
              !reader.Read(jump_delta_bits + 8, &jump_delta)) {
            return false;
          }
        }

        current_offset += jump_delta;
        if (current_offset >= bit_offset) {
          return false;
        }
      }

      DCHECK_LT(0u, hostname_offset);
      if (hostname[hostname_offset - 1] == c) {
        bit_offset = current_offset;
        hostname_offset--;
        break;
      }
    }
  }
}

bool DecodeHSTSPreload(const std::string& hostname, PreloadResult* out)
{
  bool found;
  if (!DecodeHSTSPreloadRaw(hostname, &found, out)) {
    DCHECK(false, "Internal error in DecodeHSTSPreloadRaw for hostname %s", hostname.c_str());
    return false;
  }

  return found;
}

} // namespace TPKP
