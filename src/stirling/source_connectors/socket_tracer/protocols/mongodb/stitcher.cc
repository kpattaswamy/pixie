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

#include <set>
#include <string>
#include <utility>
#include <variant>

#include <absl/strings/str_replace.h>

#include "src/stirling/source_connectors/socket_tracer/protocols/common/interface.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/types.h"
#include "src/stirling/utils/binary_decoder.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

RecordsWithErrorCount<mongodb::Record> StitchFrames(std::deque<mongodb::Frame>* reqs, std::deque<mongodb::Frame>* resps) {
  std::vector<mongodb::Record> records;
  int error_count = 0;

  // Previous request frame's requestID.
  int32_t prev_req_req_id = 1;
  // Previous response frame's requestID.
  int32_t prev_resp_req_id = 0;

  for (auto& req : *reqs) {
    for (auto resp_it = resps->begin(); resp_it != resps->end(); resp_it++) {
      // Skip the current request frame if it is an extension of the previous request 
      // frame. This is symbolized by the more_to_come flag bit in the previous frame.
      if (req.response_to == prev_req_req_id) {
        req.consumed = true;
        break;
      }
      prev_req_req_id = req.request_id;

      auto& resp = *resp_it;

      if (resp.consumed) continue;

      // Skip the current response frame if it is an extension of the previous response
      // frame. 
      if (resp.response_to == prev_resp_req_id) {
        LOG(INFO) << absl::Substitute("resp ext $0 $1", resp.response_to, prev_resp_req_id);
        prev_resp_req_id = resp.request_id;
        resp.consumed = true;
        continue;
      }
      prev_resp_req_id = resp.request_id;
      Type req_type = static_cast<Type>(req.op_code);

      // kReserved messages do not have a response pair.
      if (req_type == Type::kReserved) {
        req.consumed = true;
        records.push_back({std::move(req), {}});
        break;
      }

      if (req.timestamp_ns > resp.timestamp_ns) {
        resp.consumed = true;
        error_count++;
        VLOG(1) << absl::Substitute(
            "Did not find a request matching the response. Response ID = $0 Type = $1", int32_t(resp.response_to),
            resp.op_code);
        continue;
      }

      // A request of type kOPMsg/kOPCompressed can be matched with a response
      // of either type kOPMsg or kOPCompressed.
      if (req.request_id != resp.response_to) {
        continue;
      }

      req.consumed = true;
      resp.consumed = true;
      records.push_back({std::move(req), std::move(resp)});
      break;
    }
  }

  auto it = reqs->begin();
  while (it != reqs->end()) {
    if (!(*it).consumed) {
      break;
    }
    it++;
  }
  reqs->erase(reqs->begin(), it);
  resps->clear();

  return {records, error_count};
}

}  // namespace mongodb
}  // namespace protocols
}  // namespace stirling
}  // namespace px
