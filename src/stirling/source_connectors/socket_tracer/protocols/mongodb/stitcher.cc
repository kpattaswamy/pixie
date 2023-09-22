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

  int32_t prev_resp_req_id = 0;
  bool more_to_come = false;
  mongodb::Frame* head_resp_frame;

  for (auto& req : *reqs) {
    for (auto resp_it = resps->begin(); resp_it != resps->end(); resp_it++) {
      auto& resp = *resp_it;
      if (resp.consumed) continue;

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
            "Did not find a request matching the response. RequestID = $0 ResponseTo = $1 Type = $2", resp.request_id, resp.response_to,
            resp.op_code);
        continue;
      }

      if (resp.more_to_come) {
        more_to_come = true;
      } 

      // Make sure the request's requestID matches with the response's responseTo for 1:1 stitching.
      if (req.request_id != resp.response_to && !more_to_come) {
        continue;
      } 

      // Handle the case of 1:N request to response frame stitching.
      if (more_to_come) {
        // Check if either the request pairs with the response or if the response extends prior response.
        if (req.request_id != resp.response_to && prev_resp_req_id != resp.response_to) {
          resp.consumed = true;
          error_count++;
          continue;
        }

        prev_resp_req_id = resp.request_id;

        // Find the head response frame in the series of more to come frames. 
        if (req.request_id == resp.response_to) {
          head_resp_frame = &resp;
          continue;
        }

        // Append the current response frame's sections to the head response frame.
        head_resp_frame->sections.insert(std::end(head_resp_frame->sections), std::begin(resp.sections), std::end(resp.sections));
        resp.consumed = true;

        // Continue to append the next response to the head frame if there's more to come.
        if (resp.more_to_come) {
          continue;
        }
      }

      // Add the request and response pair to records.
      req.consumed = true;
      if (more_to_come) {
        head_resp_frame->consumed = true;
        records.push_back({std::move(req), std::move(*head_resp_frame)});
        more_to_come = false;
        break;
      }
      
      resp.consumed = true;
      records.push_back({std::move(req), std::move(resp)});
      break;
    }
  }

  // TODO(kpattaswamy) Cleanup stale requests.
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
