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

#include <libbson-1.0/bson.h>

#include <string>

#include "src/common/base/base.h"
#include "src/common/base/utils.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/decode.h"
#include "src/stirling/utils/binary_decoder.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

ParseState ProcessOpMsg(BinaryDecoder* decoder, Frame* frame) {
  PX_ASSIGN_OR(uint32_t flag_bits, decoder->ExtractIntFromLEndianBytes<uint32_t>(),
               return ParseState::kInvalid);

  // Find relavent flag bit information and ensure remaining bits are not set.
  for (int i = 0; i < 17; i++) {
    if (i == 0 && (flag_bits >> i) & 1) {
      frame->checksum_present = true;
    } else if (i == 1 && (flag_bits >> i) & 1) {
      frame->more_to_come = true;
    } else if (i == 16 && (flag_bits >> i) & 1) {
      frame->exhaust_allowed = true;
    } else if (flag_bits >> i) {
      return ParseState::kInvalid;
    }
  }

  frame->flag_bits = flag_bits;

  // Determine the number of checksum bytes in the buffer.
  uint8_t checksum_bytes = 0;
  if (frame->checksum_present) {
    checksum_bytes = 4;
  }

  // Get the section(s) data from the buffer.
  while (decoder->BufSize() > checksum_bytes) {
    mongodb::Section section;
    PX_ASSIGN_OR(section.kind, decoder->ExtractIntFromLEndianBytes<uint8_t>(),
                 return ParseState::kInvalid);
    int32_t section_length = utils::LEndianBytesToInt<int32_t, 4>(decoder->Buf());
    section.length = section_length;
    PX_ASSIGN_OR(auto section_body, decoder->ExtractString<uint8_t>(section.length),
                 return ParseState::kInvalid);

    bson_t* b = bson_new_from_data(section_body.data(), section.length);
    char* json;

    if (b != nullptr) {
      json = bson_as_canonical_extended_json(b, NULL);
      if (json != nullptr) {
        section.body = json;
        bson_free(json);
      }
      bson_destroy(b);
    }
    LOG(INFO) << absl::Substitute("$0", section.body);
    frame->sections.push_back(section);
  }

  // Get the checksum data, if necessary.
  if (frame->checksum_present) {
    PX_ASSIGN_OR(frame->checksum, decoder->ExtractIntFromLEndianBytes<uint32_t>(),
                 return ParseState::kNeedsMoreData);
  }
  return ParseState::kSuccess;
}

ParseState ProcessOpCompressed(BinaryDecoder* decoder, Frame* frame) {
  // Get the original op code.
  PX_ASSIGN_OR(frame->original_opcode, decoder->ExtractIntFromLEndianBytes<int32_t>(),
               return ParseState::kInvalid);

  // Get the uncompressed size.
  PX_ASSIGN_OR(frame->uncompressed_size, decoder->ExtractIntFromLEndianBytes<int32_t>(),
               return ParseState::kInvalid);

  // Get the compressor ID.
  PX_ASSIGN_OR(frame->compressor_id, decoder->ExtractIntFromLEndianBytes<uint8_t>(),
               return ParseState::kInvalid);

  // Get the compressed message.
  uint8_t pre_message_length = 9;
  uint32_t message_length = frame->length - mongodb::kHeaderLength - pre_message_length;
  PX_ASSIGN_OR(frame->compressed_message, decoder->ExtractString(message_length),
               return ParseState::kInvalid);

  // TODO(kpattaswamy): Add support to decompress the compressed message.
  return ParseState::kSuccess;
}

ParseState ProcessPayload(BinaryDecoder* decoder, Frame* frame) {
  Type frame_type = static_cast<Type>(frame->op_code);

  switch (frame_type) {
    case Type::kOPMsg:
      return ProcessOpMsg(decoder, frame);
    case Type::kOPCompressed:
      return ProcessOpCompressed(decoder, frame);
    case Type::kReserved:
      return ParseState::kIgnored;
    default:
      return ParseState::kInvalid;
  }
  return ParseState::kSuccess;
}

}  // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
