/*
 * Copyright 2018- The Pixie Authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>
#include <utility>
#include <vector>

#include "src/stirling/source_connectors/socket_tracer/protocols/common/event_parser.h"
#include "src/stirling/utils/utils.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

enum class Type : int32_t {
  kOPMsg = 2013,
  kOPReply = 1,
  kOPUpdate = 2001,
  kOPInsert = 2002,
  kReserved = 2003,
  kOPQuery = 2004,
  kOPGetMore = 2005,
  kOPDelete = 2006,
  kOPKillCursors = 2007,
  kOPCompressed = 2012,
};

const uint8_t kMessageLengthSize = 4;
const uint8_t kHeaderLength = 16;

struct Section {
  uint8_t kind = 0;
  int32_t length = 0;
  std::string_view body;
};

// MongoDB's Wire Protocol documentation can be found here:
// https://www.mongodb.com/docs/manual/reference/mongodb-wire-protocol/#std-label-wire-msg-sections

// The header frame for MongoDB is:
// 0          4          8          12         16
// +----------+----------+----------+----------+    +----------------+
// |  length  |requestID |responseTo|  opCode  |    |    payload     |
// +----------+----------+----------+----------+    +----------------+
//     long       long       long       long           length - 16

struct Frame : public FrameBase {
  // Message Header Fields
  // Length of the mongodb header and the application protocol data.
  uint32_t length = 0;
  int32_t request_id = 0;
  int32_t response_to = 0;
  int32_t op_code = 0;

  // OP_MSG Fields
  // Bits 0-15 are required and bits 16-31 are optional.
  uint32_t flag_bits = 0;
  bool checksum_present = false;
  bool more_to_come = false;
  bool exhaust_allowed = false;
  std::vector<Section> sections;
  uint32_t checksum = 0;

  // OP_COMPRESSED Fields
  int32_t original_opcode = 0;
  // Length of the uncompressed message, excluding the message header.
  int32_t uncompressed_size = 0;
  uint8_t compressor_id = 0;
  std::string_view compressed_message;

  bool consumed = false;
  size_t ByteSize() const override { return sizeof(Frame); }

  std::string ToString() const override {
    return absl::Substitute(
        "MongoDB message [length=$0 requestID=$1 responseTo=$2 opCode=$3 flagBits=$4 checksum=$5]",
        length, request_id, response_to, op_code, flag_bits, checksum);
  }
};

struct Record {
  Frame req;
  Frame resp;

  std::string ToString() const {
    return absl::Substitute("req=[$0] resp=[$1]", req.ToString(), resp.ToString());
  }
};

struct ProtocolTraits : public BaseProtocolTraits<Record> {
  using frame_type = Frame;
  using record_type = Record;
  using state_type = NoState;
};

}  // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
