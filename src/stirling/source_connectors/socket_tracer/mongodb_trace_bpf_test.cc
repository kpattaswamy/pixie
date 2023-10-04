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
#include "src/stirling/source_connectors/socket_tracer/testing/container_images/mongodb_server_container.h"
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
using ::testing::UnorderedElementsAre;
using ::testing::UnorderedElementsAreArray;

using ::testing::Each;
using ::testing::Field;
using ::testing::MatchesRegex;

bool Init() {
  FLAGS_stirling_enable_mongodb_tracing = true;
  return true;
}

class MongoDBTraceTest : public SocketTraceBPFTestFixture</* TClientSideTracing */ true> {
 protected:
  MongoDBTraceTest() {
    Init();

    PX_CHECK_OK(server_.Run(std::chrono::seconds{60}));
  }

  std::string mongodb_client_output = "StringString";

  std::string classpath =
      "@/app/px/src/stirling/source_connectors/socket_tracer/testing/containers/mongodb/"
      "server_image.classpath";

  StatusOr<int32_t> RunMongoDBClient() {
    std::string cmd =
        absl::StrFormat("podman exec %s /usr/bin/java -cp %s Client & echo $! && wait",
                        server_.container_name(), classpath);
    PX_ASSIGN_OR_RETURN(std::string out, px::Exec(cmd));

    LOG(INFO) << absl::StrFormat("mongodb client command output: '%s'", out);

    std::vector<std::string> tokens = absl::StrSplit(out, "\n");

    int32_t client_pid;
    if (!absl::SimpleAtoi(tokens[0], &client_pid)) {
      return error::Internal("Could not extract PID.");
    }

    LOG(INFO) << absl::StrFormat("Client PID: %d", client_pid);

    if (!absl::StrContains(out, mongodb_client_output)) {
      return error::Internal(
          absl::StrFormat("Expected output from mongodb to include '%s', received '%s' instead",
                          mongodb_client_output, out));
    }
    return client_pid;
  }

  ::px::stirling::testing::MongoDBServerContainer server_;
};

struct MongoDBTraceRecord {
  int64_t ts_ns = 0;
  mongodb::Type op_code;
  std::string req_cmd;
  std::string req_body;
  std::string resp_status;
  std::string resp_body;

  std::string ToString() const {
    return absl::Substitute("ts_ns=$0 req_cmd=$1 req_body=$2 resp_status=$3 resp_body=$4", ts_ns,
                            req_cmd, req_body, resp_status, resp_body);
  }
};

auto EqMongoDBTraceRecord(const MongoDBTraceRecord& r) {
  return AllOf(Field(&MongoDBTraceRecord::op_code, Eq(r.op_code)),
               Field(&MongoDBTraceRecord::req_cmd, Eq(r.op_msg_type)),
               Field(&MongoDBTraceRecord::resp_status, HasSubstr("ok")));
}

MongoDBTraceRecord kOPMsgInsert = {
    .op_code = mongodb::Type::kOPMsg, .req_cmd = "insert", .resp_status = "ok: {$numberDouble: 1.0}"};

std::vector<mongodb::Record> ToRecordVector(const types::ColumnWrapperRecordBatch& rb,
                                        const std::vector<size_t>& indices) {
  std::vector<mongodb::Record> result;

  for (const auto& idx : indices) {
    mongodb::Record r;
    r.req.op_msg_type = rb[kMongoDBReqCmdIdx]->Get<types::StringValue>(idx).val;
    result.push_back(r);
  }
  return result;
}

// mongodb::Record RecordOpMsgReq(mongodb::Type op_code, std::string op_msg_type) {
//   mongodb::Record r = {};
//   r.req.op_code = static_cast<int32_t>(op_code);
//   r.req.op_msg_type = op_msg_type;
//   return r;
// }

// mongodb::Record RecordOpMsgResp(mongodb::Type op_code, std::string op_msg_type) {
//   mongodb::Record r = {};
//   r.resp.op_code = static_cast<int32_t>(op_code);
//   r.resp.op_msg_type = op_msg_type;
//   return r;
// }

mongodb::Record RecordOpMsg(mongodb::Type op_code, std::string req_cmd, std::string resp_status) {
  mongodb::Record r = {};
  r.req.op_code = static_cast<int32_t>(op_code);
  r.req.op_msg_type = req_cmd;
  r.resp.op_code = static_cast<int32_t>(op_code);
  r.resp.op_msg_type = resp_status;
  return r;
}

//-----------------------------------------------------------------------------
// Test Scenarios
//-----------------------------------------------------------------------------

TEST_F(MongoDBTraceTest, Capture) {
  StartTransferDataThread();

  ASSERT_OK(RunMongoDBClient());

  StopTransferDataThread();

  // Grab the data from Stirling.
  std::vector<TaggedRecordBatch> tablets = ConsumeRecords(SocketTraceConnector::kMongoDBTableNum);
  ASSERT_NOT_EMPTY_AND_GET_RECORDS(const types::ColumnWrapperRecordBatch& record_batch, tablets);

  std::vector<mongodb::Record> server_records =
      ToRecordVector<mongodb::Record>(record_batch, server_.process_pid());

  mongodb::Record opMsg = RecordOpMsg(mongodb::Type::kOPMsg, "insert", "insert", "ok: {$numberDouble: 1.0}");

  EXPECT_THAT(server_records, Contains(EqMongoDBTraceRecord(opMsg)));
}

}  // namespace stirling
}  // namespace px
