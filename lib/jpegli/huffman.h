// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#ifndef LIB_JPEGLI_HUFFMAN_H_
#define LIB_JPEGLI_HUFFMAN_H_

#include <stdint.h>
#include <stdlib.h>

#include "lib/jpegli/common.h"

namespace jpegli {

constexpr int kJpegHuffmanRootTableBits = 8;
// Maximum huffman lookup table size.
// Requirements: alphabet of 257 symbols (256 + 1 special symbol for the all 1s
// code) and max bit length 16, the root table has 8 bits.
// zlib/examples/enough.c works with an assumption that Huffman code is
// "complete". Input JPEGs might have this assumption broken, hence the
// following sum is used as estimate:
//  + number of 1-st level cells
//  + number of symbols
//  + asymptotic amount of repeated 2nd level cells
// The third number is 1 + 3 + ... + 255 i.e. it is assumed that sub-table of
// each "size" might be almost completely be filled with repetitions.
// Total sum is slightly less than 1024,...
constexpr int kJpegHuffmanLutSize = 1024;

struct HuffmanTableEntry {
  uint8_t bits;    // number of bits used for this symbol
  uint16_t value;  // symbol value or table offset
};

void BuildJpegHuffmanTable(const uint32_t* count, const uint32_t* symbols,
                           HuffmanTableEntry* lut);

// This function will create a Huffman tree.
//
// The (data,length) contains the population counts.
// The tree_limit is the maximum bit depth of the Huffman codes.
//
// The depth contains the tree, i.e., how many bits are used for
// the symbol.
//
// See http://en.wikipedia.org/wiki/Huffman_coding
void CreateHuffmanTree(const uint32_t* data, size_t length, int tree_limit,
                       uint8_t* depth);

void ValidateHuffmanTable(j_common_ptr cinfo, const JHUFF_TBL* table,
                          bool is_dc);

void AddStandardHuffmanTables(j_common_ptr cinfo, bool is_dc);

}  // namespace jpegli

#endif  // LIB_JPEGLI_HUFFMAN_H_
