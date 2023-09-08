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

#include <rapidjson/document.h>
#include <string>

#include "src/common/base/base.h"
#include "src/common/base/utils.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/decode.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/types.h"
#include "src/stirling/utils/binary_decoder.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

ParseState ProcessOpMsg(BinaryDecoder* decoder, Frame* frame) {
  PX_ASSIGN_OR(uint32_t flag_bits, decoder->ExtractLEInt<uint32_t>(),
               return ParseState::kInvalid);

  // Find relavent flag bit information and ensure remaining bits are not set.
  // Bits 0-15 are required and bits 16-31 are optional.
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

  // Determine the number of checksum bytes in the buffer.
  uint8_t checksum_bytes = (frame->checksum_present) ? 4 : 0;

  // Get the section(s) data from the buffer.
  while (decoder->BufSize() > checksum_bytes) {
    mongodb::Section section;
    PX_ASSIGN_OR(section.kind, decoder->ExtractLEInt<uint8_t>(),
                 return ParseState::kInvalid);
    // Length of the section still remaining in the buffer.
    int32_t remaining_section_length = 0;

    if (section.kind == 0) {
      section.length = utils::LEndianBytesToInt<int32_t, 4>(decoder->Buf());
      if (section.length <= 0) {
        return ParseState::kInvalid;
      }
      remaining_section_length = section.length;
      
    } else if (section.kind == 1) {
      PX_ASSIGN_OR(section.length, decoder->ExtractLEInt<uint32_t>(),
                   return ParseState::kInvalid);
      if (section.length <= 0) {
        return ParseState::kInvalid;
      }

      // Get the sequence ID (command argument).
      PX_ASSIGN_OR(std::string_view seq_id, decoder->ExtractStringUntil('\0'),
                   return ParseState::kInvalid);
      // Make sure the sequence ID is a valid OP_MSG command argument.
      if (seq_id != "documents" && seq_id != "updates" && seq_id != "deletes") {
        return ParseState::kInvalid;
      }
      remaining_section_length = section.length - kSectionLengthSize - seq_id.length() - 1;

    } else {
      return ParseState::kInvalid;
    }

    // Extract the document(s) from the section and convert it from type BSON to JSON string.
    while (decoder->BufSize() > decoder->BufSize() - remaining_section_length) {
      auto document_length = utils::LEndianBytesToInt<int32_t, 4>(decoder->Buf());
      PX_ASSIGN_OR(auto section_body, decoder->ExtractString<uint8_t>(document_length),
                   return ParseState::kInvalid);

      bson_t* bson_doc = bson_new_from_data(section_body.data(), document_length);
      if (bson_doc == NULL) {
        return ParseState::kInvalid;
      }

      char* json = bson_as_canonical_extended_json(bson_doc, NULL);
      if (json == NULL) {
        return ParseState::kInvalid;
      }

      // Find the type of command argument if it is a request. 
      if (section.kind == 0) {
        rapidjson::Document doc;
        doc.Parse(json);

        // The type of all request commands and the response to a find requests 
        // will always be the first key.
        auto command = doc.MemberBegin()->name.GetString();
        if ((command == insert || command == delete_ || command == update || 
              command == find || command == cursor)) {
          frame->command = command;
        } else {
          // Find the "ok" key and its value.
          auto itr = doc.FindMember("ok");
          if (itr == doc.MemberEnd()) {
            return ParseState::kInvalid;
          }
          frame->command.append(itr->name.GetString()).append(": ");
          if (itr->value.IsObject()){
            auto key = itr->value.MemberBegin()->name.GetString();
            auto value = itr->value.MemberBegin()->value.GetString();
            frame->command.append("{").append(key).append(": ").append(value).append("}");
          } else if (itr->value.IsNumber()) {
            frame->command.append(std::to_string(itr->value.GetInt()));
          }
        }

        LOG(INFO) << absl::Substitute("$0", frame->command);
      }

      section.documents.push_back(json);

      bson_free(json);
      bson_destroy(bson_doc);
    }

    //cLOG(INFO) << absl::Substitute("$0", section.documents[0]);
    frame->sections.push_back(section);
  }

  // Get the checksum data, if necessary.
  if (frame->checksum_present) {
    PX_ASSIGN_OR(frame->checksum, decoder->ExtractLEInt<uint32_t>(),
                 return ParseState::kNeedsMoreData);
  }
  return ParseState::kSuccess;
}

ParseState ProcessPayload(BinaryDecoder* decoder, Frame* frame) {
  Type frame_type = static_cast<Type>(frame->op_code);
  switch (frame_type) {
    case Type::kOPMsg:
      return ProcessOpMsg(decoder, frame);
    case Type::kOPCompressed:
      return ParseState::kIgnored;
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
