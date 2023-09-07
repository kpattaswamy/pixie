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

#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/parse.h"

#include <string>
#include <utility>

#include "src/common/testing/testing.h"

namespace px {
namespace stirling {
namespace protocols {

// clang-format off

constexpr uint8_t mongoDBNeedMoreHeaderData[] = {
    // message length (4 bytes)
    0x00, 0x00, 0x00, 0x0c,
    // request id
    0x82, 0xb7, 0x31, 0x44,
    // response to
    0x00, 0x00, 0x00, 0x00,
    // op code (missing a byte)
    0xdd, 0x07, 0x00,
};

constexpr uint8_t mongoDBNeedMoreData[] = {
    // message length (18 bytes)
    0x12, 0x00, 0x00, 0x00,
    // request id
    0x82, 0xb7, 0x31, 0x44,
    // response to
    0x00, 0x00, 0x00, 0x00,
    // op code
    0xdd, 0x07, 0x00, 0x00,
    // flag bits (missing byte)
    0x00,
};

constexpr uint8_t mongoDBInvalidType[] = {
    // message length (18 bytes)
    0x12, 0x00, 0x00, 0x00,
    // request id
    0x82, 0xb7, 0x31, 0x44,
    // response to
    0x00, 0x00, 0x00, 0x00,
    // op code (2010, does not exist)
    0xda, 0x07, 0x00, 0x00,
    // flag bits
    0x00, 0x00,
};

constexpr uint8_t mongoDBUnsupportedType[] = {
    // message length (18 bytes)
    0x12, 0x00, 0x00, 0x00,
    // request id
    0x82, 0xb7, 0x31, 0x44,
    // response to
    0x00, 0x00, 0x00, 0x00,
    // op code (2004, not supported in newer versions)
    0xd4, 0x07, 0x00, 0x00,
    // flag bits
    0x00, 0x00,
};

constexpr uint8_t mongoDBInvalidFlagBits[] = {
    // message length (45 bytes)
    0x2d, 0x00, 0x00, 0x00,
    // request id (917)
    0x95, 0x03, 0x00, 0x00,
    // response to (444)
    0xbc, 0x01, 0x00, 0x00,
    // op code (2013)
    0xdd, 0x07, 0x00, 0x00,
    // flag bits (invalid bit set)
    0x05, 0x00, 0x00, 0x00,
    // section 1
    0x00, 0x18, 0x00, 0x00, 0x00, 0x10, 0x6e, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x6f, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x3f, 0x00,
};

constexpr uint8_t mongoDBMissingChecksum[] = {
    // message length (157 bytes)
    0x9d, 0x00, 0x00, 0x00,
    // request id (1144108930)
    0x82, 0xb7, 0x31, 0x44,
    // response to (0)
    0x00, 0x00, 0x00, 0x00,
    // op code (2013)
    0xdd, 0x07, 0x00, 0x00,
    // flag bits (checksum set)
    0x01, 0x00, 0x00, 0x00,
    // section 1 (1 kind byte, 82 section body bytes)
    0x00, 0x52, 0x00, 0x00, 0x00, 0x02, 0x69, 0x6e, 0x73, 0x65, 0x72,
    0x74, 0x00, 0x04, 0x00, 0x00, 0x00, 0x63, 0x61, 0x72, 0x00, 0x08,
    0x6f, 0x72, 0x64, 0x65, 0x72, 0x65, 0x64, 0x00, 0x01, 0x03, 0x6c,
    0x73, 0x69, 0x64, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x69, 0x64,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x0e, 0xab, 0xf5, 0xe5, 0x45,
    0xf8, 0x42, 0x5f, 0x8c, 0xb5, 0xb4, 0x0d, 0xff, 0x94, 0x8e, 0x1c,
    0x00, 0x02, 0x24, 0x64, 0x62, 0x00, 0x06, 0x00, 0x00, 0x00, 0x6d,
    0x79, 0x64, 0x62, 0x31, 0x00, 0x00,
    // section 2 (1 kind byte, 53 section body bytes)
    0x01, 0x35, 0x00, 0x00, 0x00, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65,
    0x6e, 0x74, 0x73, 0x00, 0x27, 0x00, 0x00, 0x00, 0x07, 0x5f, 0x69,
    0x64, 0x00, 0x64, 0xdb, 0xd4, 0x67, 0x8f, 0x0e, 0x65, 0x5d, 0x43,
    0x14, 0xd6, 0x8a, 0x02, 0x6e, 0x61, 0x6d, 0x65, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x74, 0x65, 0x73, 0x6c, 0x61, 0x34, 0x00, 0x00,
    // checksum bytes missing
};

constexpr uint8_t mongoDBValidRequest[] = {
    // message length (178 bytes)
    0xb2, 0x00, 0x00, 0x00,
    // request id (444)
    0xbc, 0x01, 0x00, 0x00,
    // response to (0)
    0x00, 0x00, 0x00, 0x00,
    // op code (2013)
    0xdd, 0x07, 0x00, 0x00,
    // flag bits
    0x00, 0x00, 0x00, 0x00,
    // section 1
    0x00, 0x9d, 0x00, 0x00, 0x00, 0x02, 0x69, 0x6e, 0x73, 0x65, 0x72,
    0x74, 0x00, 0x04, 0x00, 0x00, 0x00, 0x63, 0x61, 0x72, 0x00, 0x04,
    0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65, 0x6e, 0x74, 0x73, 0x00, 0x40,
    0x00, 0x00, 0x00, 0x03, 0x30, 0x00, 0x38, 0x00, 0x00, 0x00, 0x02,
    0x6e, 0x61, 0x6d, 0x65, 0x00, 0x18, 0x00, 0x00, 0x00, 0x70, 0x69,
    0x78, 0x69, 0x65, 0x2d, 0x63, 0x61, 0x72, 0x2d, 0x31, 0x30, 0x2d,
    0x76, 0x65, 0x72, 0x73, 0x69, 0x6f, 0x6e, 0x37, 0x2e, 0x30, 0x00,
    0x07, 0x5f, 0x69, 0x64, 0x00, 0x64, 0xe6, 0x72, 0x9c, 0x99, 0x6d,
    0x67, 0x6b, 0xf5, 0x20, 0x9d, 0xba, 0x00, 0x00, 0x08, 0x6f, 0x72,
    0x64, 0x65, 0x72, 0x65, 0x64, 0x00, 0x01, 0x03, 0x6c, 0x73, 0x69,
    0x64, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x69, 0x64, 0x00, 0x10,
    0x00, 0x00, 0x00, 0x04, 0xe7, 0xd7, 0x16, 0xb3, 0x75, 0xb7, 0x4c,
    0x39, 0x8b, 0x75, 0x41, 0x97, 0xc4, 0x97, 0x06, 0xd1, 0x00, 0x02,
    0x24, 0x64, 0x62, 0x00, 0x06, 0x00, 0x00, 0x00, 0x6d, 0x79, 0x64,
    0x62, 0x31, 0x00, 0x00,
};

constexpr uint8_t mongoDBValidResponse[] = {
    // message length (45 bytes)
    0x2d, 0x00, 0x00, 0x00,
    // request id (917)
    0x95, 0x03, 0x00, 0x00,
    // response to (444)
    0xbc, 0x01, 0x00, 0x00,
    // op code (2013)
    0xdd, 0x07, 0x00, 0x00,
    // flag bits
    0x00, 0x00, 0x00, 0x00,
    // section 1
    0x00, 0x18, 0x00, 0x00, 0x00, 0x10, 0x6e, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x01, 0x6f, 0x6b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0x3f, 0x00,
};

constexpr uint8_t mongoDBValidRequestTwoSections[] = {
    // message length (157 bytes)
    0x9d, 0x00, 0x00, 0x00,
    // request id (1144108930)
    0x82, 0xb7, 0x31, 0x44,
    // response to (0)
    0x00, 0x00, 0x00, 0x00,
    // op code (2013)
    0xdd, 0x07, 0x00, 0x00,
    // flag bits
    0x00, 0x00, 0x00, 0x00,
    // section 1 (1 kind byte, 82 section body bytes)
    0x00, 0x52, 0x00, 0x00, 0x00, 0x02, 0x69, 0x6e, 0x73, 0x65, 0x72,
    0x74, 0x00, 0x04, 0x00, 0x00, 0x00, 0x63, 0x61, 0x72, 0x00, 0x08,
    0x6f, 0x72, 0x64, 0x65, 0x72, 0x65, 0x64, 0x00, 0x01, 0x03, 0x6c,
    0x73, 0x69, 0x64, 0x00, 0x1e, 0x00, 0x00, 0x00, 0x05, 0x69, 0x64,
    0x00, 0x10, 0x00, 0x00, 0x00, 0x04, 0x0e, 0xab, 0xf5, 0xe5, 0x45,
    0xf8, 0x42, 0x5f, 0x8c, 0xb5, 0xb4, 0x0d, 0xff, 0x94, 0x8e, 0x1c,
    0x00, 0x02, 0x24, 0x64, 0x62, 0x00, 0x06, 0x00, 0x00, 0x00, 0x6d,
    0x79, 0x64, 0x62, 0x31, 0x00, 0x00,
    // section 2 (1 kind byte, 53 section body bytes)
    0x01, 0x35, 0x00, 0x00, 0x00, 0x64, 0x6f, 0x63, 0x75, 0x6d, 0x65,
    0x6e, 0x74, 0x73, 0x00, 0x27, 0x00, 0x00, 0x00, 0x07, 0x5f, 0x69,
    0x64, 0x00, 0x64, 0xdb, 0xd4, 0x67, 0x8f, 0x0e, 0x65, 0x5d, 0x43,
    0x14, 0xd6, 0x8a, 0x02, 0x6e, 0x61, 0x6d, 0x65, 0x00, 0x07, 0x00,
    0x00, 0x00, 0x74, 0x65, 0x73, 0x6c, 0x61, 0x34, 0x00, 0x00,
};

// clang-format on

class MongoDBParserTest : public ::testing::Test {};

TEST_F(MongoDBParserTest, ParseFrameWhenNeedsMoreHeaderData) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBNeedMoreHeaderData));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(state, ParseState::kNeedsMoreData);
}

TEST_F(MongoDBParserTest, ParseFrameWhenNeedsMoreData) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBNeedMoreData));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(state, ParseState::kNeedsMoreData);
}

TEST_F(MongoDBParserTest, ParseFrameWhenNotValidMongoDBType) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBInvalidType));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(state, ParseState::kInvalid);
}

TEST_F(MongoDBParserTest, ParseFrameWhenUnsupportedMongoDBType) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBUnsupportedType));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(state, ParseState::kIgnored);
}

TEST_F(MongoDBParserTest, ParseFrameInvalidMongoDBFlagBits) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBInvalidFlagBits));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(state, ParseState::kInvalid);
}

TEST_F(MongoDBParserTest, ParseFrameMissingMongoDBChecksum) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBMissingChecksum));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(frame.length, 157);
  ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  ASSERT_EQ(frame.sections[0].length, 82);
  ASSERT_EQ(frame.sections[1].length, 53);
  ASSERT_EQ(state, ParseState::kNeedsMoreData);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBRequestInsert) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBValidRequest));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(frame.length, 178);
  ASSERT_EQ(frame.request_id, 444);
  ASSERT_EQ(frame.response_to, 0);
  ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBResponseInsert) {
  auto frame_view = CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBValidResponse));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kResponse, &frame_view, &frame);

  ASSERT_EQ(frame.length, 45);
  ASSERT_EQ(frame.request_id, 917);
  ASSERT_EQ(frame.response_to, 444);
  ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  ASSERT_EQ(frame.sections[0].length, 24);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBRequestDelete) {
  std::string_view input = CreateStringView<char>("\xad\x00\x00\x00\x5d\x00\x00\x00\x00\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x98\x00\x00\x00\x02\x64\x65\x6c\x65\x74\x65" \
"\x00\x04\x00\x00\x00\x66\x6f\x6f\x00\x04\x64\x65\x6c\x65\x74\x65" \
"\x73\x00\x3e\x00\x00\x00\x03\x30\x00\x36\x00\x00\x00\x03\x71\x00" \
"\x23\x00\x00\x00\x02\x6e\x61\x6d\x65\x00\x0b\x00\x00\x00\x44\x6f" \
"\x63\x75\x6d\x65\x6e\x74\x20\x31\x00\x10\x61\x67\x65\x00\x19\x00" \
"\x00\x00\x00\x10\x6c\x69\x6d\x69\x74\x00\x00\x00\x00\x00\x00\x00" \
"\x08\x6f\x72\x64\x65\x72\x65\x64\x00\x01\x03\x6c\x73\x69\x64\x00" \
"\x1e\x00\x00\x00\x05\x69\x64\x00\x10\x00\x00\x00\x04\xce\x8b\xe1" \
"\x9b\x4e\xf7\x4d\x40\xa1\x46\x28\xfb\x26\x6b\x01\xc8\x00\x02\x24" \
"\x64\x62\x00\x05\x00\x00\x00\x74\x65\x73\x74\x00\x00");
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBResponseDelete) {
  std::string_view input = CreateStringView<char>("\x2d\x00\x00\x00\xf8\x12\x00\x00\x5d\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x18\x00\x00\x00\x10\x6e\x00\x04\x00\x00\x00" \
"\x01\x6f\x6b\x00\x00\x00\x00\x00\x00\x00\xf0\x3f\x00");
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}



TEST_F(MongoDBParserTest, ParseFrameValidMongoDBRequestFind) {
  std::string_view input = CreateStringView<char>("\x85\x00\x00\x00\x13\x00\x00\x00\x00\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x70\x00\x00\x00\x02\x66\x69\x6e\x64\x00\x04" \
"\x00\x00\x00\x66\x6f\x6f\x00\x03\x66\x69\x6c\x74\x65\x72\x00\x23" \
"\x00\x00\x00\x02\x6e\x61\x6d\x65\x00\x0b\x00\x00\x00\x44\x6f\x63" \
"\x75\x6d\x65\x6e\x74\x20\x31\x00\x10\x61\x67\x65\x00\x19\x00\x00" \
"\x00\x00\x03\x6c\x73\x69\x64\x00\x1e\x00\x00\x00\x05\x69\x64\x00" \
"\x10\x00\x00\x00\x04\xce\x8b\xe1\x9b\x4e\xf7\x4d\x40\xa1\x46\x28" \
"\xfb\x26\x6b\x01\xc8\x00\x02\x24\x64\x62\x00\x05\x00\x00\x00\x74" \
"\x65\x73\x74\x00\x00");
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBResponseCursor) {
  std::string_view input = CreateStringView<char>("\x98\x00\x00\x00\x5d\x12\x00\x00\x13\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x83\x00\x00\x00\x03\x63\x75\x72\x73\x6f\x72" \
"\x00\x6a\x00\x00\x00\x04\x66\x69\x72\x73\x74\x42\x61\x74\x63\x68" \
"\x00\x3c\x00\x00\x00\x03\x30\x00\x34\x00\x00\x00\x07\x5f\x69\x64" \
"\x00\x64\xf8\xe1\x1d\x27\x2b\x48\x9a\x9d\xc1\x02\xef\x02\x6e\x61" \
"\x6d\x65\x00\x0b\x00\x00\x00\x44\x6f\x63\x75\x6d\x65\x6e\x74\x20" \
"\x31\x00\x10\x61\x67\x65\x00\x19\x00\x00\x00\x00\x00\x12\x69\x64" \
"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x6e\x73\x00\x09\x00\x00" \
"\x00\x74\x65\x73\x74\x2e\x66\x6f\x6f\x00\x00\x01\x6f\x6b\x00\x00" \
"\x00\x00\x00\x00\x00\xf0\x3f\x00");
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBRequestUpdate) {
  std::string_view input = CreateStringView<char>("\xd6\x00\x00\x00\xa3\x00\x00\x00\x00\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\xc1\x00\x00\x00\x02\x75\x70\x64\x61\x74\x65" \
"\x00\x04\x00\x00\x00\x66\x6f\x6f\x00\x04\x75\x70\x64\x61\x74\x65" \
"\x73\x00\x67\x00\x00\x00\x03\x30\x00\x5f\x00\x00\x00\x03\x71\x00" \
"\x1a\x00\x00\x00\x02\x6e\x61\x6d\x65\x00\x0b\x00\x00\x00\x44\x6f" \
"\x63\x75\x6d\x65\x6e\x74\x20\x31\x00\x00\x03\x75\x00\x3a\x00\x00" \
"\x00\x03\x24\x73\x65\x74\x00\x0e\x00\x00\x00\x10\x61\x67\x65\x00" \
"\x32\x00\x00\x00\x00\x03\x24\x63\x75\x72\x72\x65\x6e\x74\x44\x61" \
"\x74\x65\x00\x13\x00\x00\x00\x08\x6c\x61\x73\x74\x55\x70\x64\x61" \
"\x74\x65\x64\x00\x01\x00\x00\x00\x00\x08\x6f\x72\x64\x65\x72\x65" \
"\x64\x00\x01\x03\x6c\x73\x69\x64\x00\x1e\x00\x00\x00\x05\x69\x64" \
"\x00\x10\x00\x00\x00\x04\xce\x8b\xe1\x9b\x4e\xf7\x4d\x40\xa1\x46" \
"\x28\xfb\x26\x6b\x01\xc8\x00\x02\x24\x64\x62\x00\x05\x00\x00\x00" \
"\x74\x65\x73\x74\x00\x00");
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBResponseUpdate) {
  std::string_view input = CreateStringView<char>("\x3c\x00\x00\x00\x88\x13\x00\x00\xa3\x00\x00\x00\xdd\x07\x00\x00" \
"\x00\x00\x00\x00\x00\x27\x00\x00\x00\x10\x6e\x00\x00\x00\x00\x00" \
"\x10\x6e\x4d\x6f\x64\x69\x66\x69\x65\x64\x00\x00\x00\x00\x00\x01" \
"\x6f\x6b\x00\x00\x00\x00\x00\x00\x00\xf0\x3f\x00"
);
  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &input, &frame);

  // ASSERT_EQ(frame.length, 178);
  // ASSERT_EQ(frame.request_id, 444);
  // ASSERT_EQ(frame.response_to, 0);
  // ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  // ASSERT_EQ(frame.sections[0].length, 157);
  ASSERT_EQ(state, ParseState::kSuccess);
}

TEST_F(MongoDBParserTest, ParseFrameValidMongoDBRequestTwoSections) {
  auto frame_view =
      CreateStringView<char>(CharArrayStringView<uint8_t>(mongoDBValidRequestTwoSections));

  mongodb::Frame frame;
  ParseState state = ParseFrame(message_type_t::kRequest, &frame_view, &frame);

  ASSERT_EQ(frame.length, 157);
  ASSERT_EQ(frame.request_id, 1144108930);
  ASSERT_EQ(frame.response_to, 0);
  ASSERT_EQ(frame.op_code, static_cast<int32_t>(mongodb::Type::kOPMsg));
  ASSERT_EQ(frame.sections[0].length, 82);
  ASSERT_EQ(frame.sections[1].length, 53);
  ASSERT_EQ(state, ParseState::kSuccess);
}

namespace mongodb {}  // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
