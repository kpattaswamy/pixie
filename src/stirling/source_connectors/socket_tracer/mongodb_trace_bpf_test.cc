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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include <absl/strings/str_replace.h>

#include "src/common/base/base.h"
#include "src/common/exec/exec.h"
#include "src/common/testing/testing.h"
#include "src/shared/types/column_wrapper.h"
#include "src/shared/types/types.h"
#include "src/stirling/core/data_table.h"
#include "src/stirling/core/output.h"
#include "src/stirling/source_connectors/socket_tracer/mongodb_table.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/types.h"
#include "src/stirling/source_connectors/socket_tracer/testing/container_images/mongodb_container.h"
#include "src/stirling/source_connectors/socket_tracer/testing/container_images/mongodb_client_container.h"
#include "src/stirling/source_connectors/socket_tracer/testing/protocol_checkers.h"
#include "src/stirling/source_connectors/socket_tracer/testing/socket_trace_bpf_test_fixture.h"
#include "src/stirling/testing/common.h"
#include "src/stirling/utils/linux_headers.h"

namespace px {
namespace stirling {

namespace mongodb = protocols::mongodb;

using ::px::stirling::testing::FindRecordIdxMatchesPID;
using ::px::stirling::testing::FindRecordsMatchingPID;
using ::px::stirling::testing::GetTargetRecords;
using ::px::stirling::testing::SocketTraceBPFTestFixture;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

using ::testing::Each;
using ::testing::Field;
using ::testing::MatchesRegex;

void Init() {
  FLAGS_stirling_enable_mongodb_tracing = true;
}

class MongoDBTraceTest : public SocketTraceBPFTestFixture</* TClientSideTracing */ true> {
 protected:
   MongoDBTraceTest() {
    Init();
    PX_CHECK_OK(mongodb_server_.Run(std::chrono::seconds{120}));
   }

  void RunMongoDBClient() {
    mongodb_client_.Run(
        std::chrono::seconds{120},
        {absl::Substitute("--network=container:$0", mongodb_server_.container_name())});
  }

  ::px::stirling::testing::MongoDBClientContainer mongodb_client_;
  ::px::stirling::testing::MongoDBContainer mongodb_server_;
};

auto EqMongoDB(const protocols::mongodb::Frame& f) {
  return Field(&protocols::mongodb::Frame::op_msg_type, Eq(f.op_msg_type));
}

auto EqMongoDBRecord(const protocols::mongodb::Record& r) {
  return AllOf(Field(&protocols::mongodb::Record::req, EqMongoDB(r.req)),
               Field(&protocols::mongodb::Record::resp, EqMongoDB(r.resp)));
}

std::vector<mongodb::Record> ToRecordVector(const types::ColumnWrapperRecordBatch& rb,
                                        const std::vector<size_t>& indices) {
  std::vector<mongodb::Record> result;
  for (const auto& idx : indices) {
    mongodb::Record r;
    r.req.op_msg_type = std::string(rb[kMongoDBReqCmdIdx]->Get<types::StringValue>(idx));
    r.resp.op_msg_type = std::string(rb[kMongoDBRespStatusIdx]->Get<types::StringValue>(idx));
    result.push_back(r);
  }
  return result;
}

mongodb::Record RecordOpMsg(std::string req_cmd, std::string resp_status) {
  mongodb::Record r = {};
  r.req.op_msg_type = req_cmd;
  r.resp.op_msg_type = resp_status;
  return r;
}

//-----------------------------------------------------------------------------
// Test Scenarios
//-----------------------------------------------------------------------------

TEST_F(MongoDBTraceTest, Capture) {
  // Initiate the mongo transactions.
  StartTransferDataThread();
  RunMongoDBClient();
  LOG(INFO) << "client finished running";
  StopTransferDataThread();

  // Grab the data from Stirling.
  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kMongoDBTableNum);
  ASSERT_NOT_EMPTY_AND_GET_RECORDS(const types::ColumnWrapperRecordBatch& record_batch, tablets);

  std::vector<mongodb::Record> server_records =
      GetTargetRecords<mongodb::Record>(record_batch, mongodb_server_.process_pid());

  mongodb::Record opMsgInsert = RecordOpMsg("insert", "ok: {$numberDouble: 1.0}");

  EXPECT_THAT(server_records, Contains(EqMongoDBRecord(opMsgInsert)));
}

}  // namespace stirling
}  // namespace px
