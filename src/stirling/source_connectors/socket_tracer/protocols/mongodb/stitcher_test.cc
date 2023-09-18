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

// using ::testing::ElementsAre;
// using ::testing::ElementsAreArray;
// using ::testing::Field;
using ::testing::IsEmpty;
using ::testing::SizeIs;
// using ::testing::StrEq;

class MongoDBStitchFramesTest : public ::testing::Test {};

Frame CreateMongoDBFrame(uint64_t ts_ns, mongodb::Type type, int32_t request_id,
                                  int32_t response_to) {
  mongodb::Frame frame;
  frame.timestamp_ns = ts_ns;

  frame.op_code = static_cast<int32_t>(type);
  frame.request_id = request_id;
  frame.response_to = response_to;
  return frame;

}

TEST_F(MongoDBStitchFramesTest, VerifyOnetoOneMatching) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 0),
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 0),
      CreateMongoDBFrame(4, mongodb::Type::kOPMsg, 5, 0),
      CreateMongoDBFrame(6, mongodb::Type::kOPMsg, 7, 0),
      CreateMongoDBFrame(8, mongodb::Type::kOPMsg, 9, 0),
      CreateMongoDBFrame(10, mongodb::Type::kOPMsg, 11, 0),
      CreateMongoDBFrame(12, mongodb::Type::kOPMsg, 13, 0),
      CreateMongoDBFrame(14, mongodb::Type::kOPMsg, 15, 0),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 1),
      CreateMongoDBFrame(3, mongodb::Type::kOPMsg, 4, 3),
      CreateMongoDBFrame(5, mongodb::Type::kOPMsg, 6, 5),
      CreateMongoDBFrame(7, mongodb::Type::kOPMsg, 8, 7),
      CreateMongoDBFrame(9, mongodb::Type::kOPMsg, 10, 9),
      CreateMongoDBFrame(11, mongodb::Type::kOPMsg, 12, 11),
      CreateMongoDBFrame(13, mongodb::Type::kOPMsg, 14, 13),
      CreateMongoDBFrame(15, mongodb::Type::kOPMsg, 16, 15),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_THAT(result.records, SizeIs(8));
  EXPECT_THAT(reqs, IsEmpty());
  EXPECT_THAT(resps, IsEmpty());
}

TEST_F(MongoDBStitchFramesTest, VerifyOnetoNStitching) {
  std::deque<mongodb::Frame> reqs = {
      CreateMongoDBFrame(0, mongodb::Type::kOPMsg, 1, 0),
      CreateMongoDBFrame(2, mongodb::Type::kOPMsg, 3, 0),
      CreateMongoDBFrame(4, mongodb::Type::kOPMsg, 5, 0),
      CreateMongoDBFrame(8, mongodb::Type::kOPMsg, 9, 0),
      CreateMongoDBFrame(10, mongodb::Type::kOPMsg, 11, 0),
      CreateMongoDBFrame(12, mongodb::Type::kOPMsg, 13, 0),
      CreateMongoDBFrame(14, mongodb::Type::kOPMsg, 15, 0),
      CreateMongoDBFrame(16, mongodb::Type::kOPMsg, 17, 0),
  };
  std::deque<mongodb::Frame> resps = {
      CreateMongoDBFrame(1, mongodb::Type::kOPMsg, 2, 1),
      CreateMongoDBFrame(3, mongodb::Type::kOPMsg, 4, 3),

      CreateMongoDBFrame(5, mongodb::Type::kOPMsg, 6, 5),
      CreateMongoDBFrame(6, mongodb::Type::kOPMsg, 7, 6),
      CreateMongoDBFrame(7, mongodb::Type::kOPMsg, 8, 7),

      CreateMongoDBFrame(9, mongodb::Type::kOPMsg, 10, 9),
      CreateMongoDBFrame(11, mongodb::Type::kOPMsg, 12, 11),
      CreateMongoDBFrame(13, mongodb::Type::kOPMsg, 14, 13),
      CreateMongoDBFrame(15, mongodb::Type::kOPMsg, 16, 15),
      CreateMongoDBFrame(17, mongodb::Type::kOPMsg, 18, 17),
  };

  RecordsWithErrorCount<mongodb::Record> result = mongodb::StitchFrames(&reqs, &resps);
  EXPECT_EQ(result.error_count, 0);
  EXPECT_THAT(result.records, SizeIs(8));
  EXPECT_THAT(reqs, IsEmpty());
  EXPECT_THAT(resps, IsEmpty());
}

} // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
