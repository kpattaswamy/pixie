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

#include <string>
#include <utility>

#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/parse.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/types.h"
#include "src/stirling/utils/binary_decoder.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

ParseState ParseFullFrame(BinaryDecoder* decoder, Frame* frame) {
  // Get the Op Code (type).
  PX_ASSIGN_OR(frame->op_code, decoder->ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);
  if (frame->op_code < 1 || frame->op_code > 2013) {
    return ParseState::kInvalid;
  }

  // Make sure the Op Code is a valid type for MongoDB.
  Type frame_type = static_cast<Type>(frame->op_code);
  if (!(frame_type == Type::kOPMsg || frame_type == Type::kOPReply ||
        frame_type == Type::kOPUpdate || frame_type == Type::kOPInsert ||
        frame_type == Type::kReserved || frame_type == Type::kOPQuery ||
        frame_type == Type::kOPGetMore || frame_type == Type::kOPDelete ||
        frame_type == Type::kOPKillCursors || frame_type == Type::kOPCompressed)) {
    return ParseState::kInvalid;
  }

  if (frame_type == Type::kOPMsg) {
    PX_ASSIGN_OR(uint32_t flag_bits, decoder->ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);
    // Check if checksumPresent is set.
    if (flag_bits & 1) {
      frame->checksum_present = true;
    }
    // Check if moreToCome is set.
    if ((flag_bits >> 1) & 1) {
      frame->more_to_come = true;
    }
    // Check if exhaustAllowed is set. 
    if ((flag_bits >> 16) & 1) {
      frame->exhaust_allowed = true;
    }
    frame->flag_bits = flag_bits;

    // Determine the number of checksum bytes in the buffer.
    uint8_t checksum_bytes = 0;
    if (frame->checksum_present) {
      checksum_bytes = 4;
    }

    // Get the section(s) data from the buffer.
    while(decoder->BufSize() > checksum_bytes) {
      mongodb::Section section;
      PX_ASSIGN_OR(section.kind, decoder->ExtractIntFromLEndianBytes<uint8_t>(), return ParseState::kInvalid);

      PX_ASSIGN_OR(section.length, decoder->ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);

      uint32_t body_length = section.length - 4; 
      PX_ASSIGN_OR(section.body, decoder->ExtractString(body_length), return ParseState::kInvalid);

      frame->sections.push_back(section);
    }

    // Get the checksum data if necessary.
    if (frame->checksum_present) {
      PX_ASSIGN_OR(frame->checksum, decoder->ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);
    }

    // for (auto& it : frame->sections[1].body) {
    //   LOG(INFO) << absl::Substitute("sections[1] $0", it);
    // }

  } else {
    return ParseState::kIgnored;
  }

  return ParseState::kSuccess;
}

}  // namespace mongodb

template <>
ParseState ParseFrame(message_type_t type, std::string_view* buf, mongodb::Frame* frame, NoState*) {
  if (type != message_type_t::kRequest && type != message_type_t::kResponse) {
    return ParseState::kInvalid;
  }

  BinaryDecoder decoder(*buf);

  if (decoder.BufSize() < mongodb::kHeaderLength) {
    return ParseState::kNeedsMoreData;
  }

  // Get the length of the packet. This length contains the size of the field containing the message's length itself. 
  PX_ASSIGN_OR(uint32_t message_length, decoder.ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);
  if (decoder.BufSize() < message_length - mongodb::kMessageLengthSize) {
    return ParseState::kNeedsMoreData;
  }
  frame->length = message_length - mongodb::kMessageLengthSize;
   
  // Get the Request ID.
  PX_ASSIGN_OR(frame->request_id, decoder.ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);

  // Get the Response To.
  PX_ASSIGN_OR(frame->response_to, decoder.ExtractIntFromLEndianBytes<uint32_t>(), return ParseState::kInvalid);

  // Parse the full frame, including Op Code.
  ParseState parse_state = mongodb::ParseFullFrame(&decoder, frame);
  if (parse_state == ParseState::kSuccess) {
    buf->remove_prefix(frame->length + mongodb::kMessageLengthSize);
  }

  return parse_state;
}

template <>
size_t FindFrameBoundary<mongodb::Frame>(message_type_t, std::string_view, size_t, NoState*) {
  // Not implemented.
  return std::string::npos;
}

}  // namespace protocols
}  // namespace stirling
}  // namespace px
