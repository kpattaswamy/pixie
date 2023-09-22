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

#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/stitcher.h"

#include <string>

#include "src/common/testing/testing.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

using ::testing::IsEmpty;
using ::testing::SizeIs;

class MongoDBStitchFramesTest : public ::testing::Test {};

Frame CreateMongoDBFrame(uint64_t ts_ns, mongodb::Type type, int32_t request_id,
                         int32_t response_to, bool more_to_come, std::string doc = "") {
  mongodb::Frame frame;
  frame.timestamp_ns = ts_ns;

  frame.op_code = static_cast<int32_t>(type);
  frame.request_id = request_id;
  frame.response_to = response_to;
  frame.more_to_come = more_to_come;

  mongodb::Section section;
  section.documents.push_back(doc);

  frame.sections.push_back(section);
  return frame;
}

TEST_F(MongoDBStitchFramesTest, VerifyOnetoOneMatching) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 0, false),
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 0, false),
      CreateMongoDBFrame(4, mongodb::Type::kOPMsg, 5, 0, false),
      CreateMongoDBFrame(6, mongodb::Type::kOPMsg, 7, 0, false),
      CreateMongoDBFrame(8, mongodb::Type::kOPMsg, 9, 0, false),
      CreateMongoDBFrame(10, mongodb::Type::kOPMsg, 11, 0, false),
      CreateMongoDBFrame(12, mongodb::Type::kOPMsg, 13, 0, false),
      CreateMongoDBFrame(14, mongodb::Type::kOPMsg, 15, 0, false),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 1, false),
      CreateMongoDBFrame(3, mongodb::Type::kOPMsg, 4, 3, false),
      CreateMongoDBFrame(5, mongodb::Type::kOPMsg, 6, 5, false),
      CreateMongoDBFrame(7, mongodb::Type::kOPMsg, 8, 7, false),
      CreateMongoDBFrame(9, mongodb::Type::kOPMsg, 10, 9, false),
      CreateMongoDBFrame(11, mongodb::Type::kOPMsg, 12, 11, false),
      CreateMongoDBFrame(13, mongodb::Type::kOPMsg, 14, 13, false),
      CreateMongoDBFrame(15, mongodb::Type::kOPMsg, 16, 15, false),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_THAT(result.records, SizeIs(8));
  EXPECT_THAT(reqs, IsEmpty());
  EXPECT_THAT(resps, IsEmpty());
}

TEST_F(MongoDBStitchFramesTest, VerifyOnetoNStitching) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 0, false),
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 0, false),

      // Request frame for multi frame response message
      CreateMongoDBFrame(4, mongodb::Type::kOPMsg, 5, 0, false),

      CreateMongoDBFrame(8, mongodb::Type::kOPMsg, 9, 0, false),
      CreateMongoDBFrame(10, mongodb::Type::kOPMsg, 11, 0, false),
      CreateMongoDBFrame(12, mongodb::Type::kOPMsg, 13, 0, false),
      CreateMongoDBFrame(14, mongodb::Type::kOPMsg, 15, 0, false),
      CreateMongoDBFrame(16, mongodb::Type::kOPMsg, 17, 0, false),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 1, false),
      CreateMongoDBFrame(3, mongodb::Type::kOPMsg, 4, 3, false),

      // Multi frame response message
      CreateMongoDBFrame(5, mongodb::Type::kOPMsg, 6, 5, true, "1"),
      CreateMongoDBFrame(6, mongodb::Type::kOPMsg, 7, 6, true, "2"),
      CreateMongoDBFrame(7, mongodb::Type::kOPMsg, 8, 7, false, "3"),

      CreateMongoDBFrame(9, mongodb::Type::kOPMsg, 10, 9, false),
      CreateMongoDBFrame(11, mongodb::Type::kOPMsg, 12, 11, false),
      CreateMongoDBFrame(13, mongodb::Type::kOPMsg, 14, 13, false),
      CreateMongoDBFrame(15, mongodb::Type::kOPMsg, 16, 15, false),
      CreateMongoDBFrame(17, mongodb::Type::kOPMsg, 18, 17, false),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_EQ(result.records[2].resp.sections[0].documents[0], "1");
  EXPECT_EQ(result.records[2].resp.sections[1].documents[0], "2");
  EXPECT_EQ(result.records[2].resp.sections[2].documents[0], "3");
  EXPECT_THAT(result.records, SizeIs(8));

  EXPECT_THAT(reqs, IsEmpty());
  EXPECT_THAT(resps, IsEmpty());
}

// Add test case for handling kReserved

TEST_F(MongoDBStitchFramesTest, UnmatchedResponsesAreHandled) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 0, false),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 10, false),
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 2, false),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);

  EXPECT_EQ(result.error_count, 1);
  EXPECT_EQ(result.records.size(), 1);

  EXPECT_THAT(reqs, IsEmpty());
  EXPECT_THAT(resps, IsEmpty());
}

TEST_F(MongoDBStitchFramesTest, UnmatchedRequestsAreNotCleanedUp) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 0, false),
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 0, false),
      CreateMongoDBFrame(3, mongodb::Type::kOPMsg, 4, 0, false),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 2, false),
      CreateMongoDBFrame(4, mongodb::Type::kOPMsg, 5, 4, false),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);

  EXPECT_EQ(result.error_count, 0);
  EXPECT_THAT(result.records, SizeIs(2));
  EXPECT_EQ(result.records[0].req.request_id, 2);
  EXPECT_EQ(result.records[1].req.request_id, 4);
  
  // Stale requests are not yet cleaned up.
  EXPECT_THAT(reqs, SizeIs(3));
  EXPECT_THAT(reqs[0].consumed, false);
  EXPECT_THAT(reqs[1].consumed, true);
  EXPECT_THAT(reqs[2].consumed, true);
  EXPECT_THAT(resps, IsEmpty());
}

}  // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
