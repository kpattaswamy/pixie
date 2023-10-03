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

#pragma once

#include <deque>
#include <map>
#include <vector>

#include "src/common/base/base.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/common/interface.h"
#include "src/stirling/source_connectors/socket_tracer/protocols/mongodb/types.h"

namespace px {
namespace stirling {
namespace protocols {
namespace mongodb {

RecordsWithErrorCount<mongodb::Record> StitchFrames(
    std::map<mongodb::stream_id, std::deque<Frame>>* reqs,
    std::map<mongodb::stream_id, std::deque<Frame>>* resps);
}  // namespace mongodb

// Uncomment this when the upstream stitching interface is done
// template <>
// inline RecordsWithErrorCount<mongodb::Record> StitchFrames(
//     std::map<mongodb::stream_id, std::deque<mongodb::Frame>>* reqs,                                                   
//     std::map<mongodb::stream_id, std::deque<mongodb::Frame>>* resps,
//     NoState* /*state*/) {
//   return mongodb::StitchFrames(reqs, resps);
// }


}  // namespace protocols
}  // namespace stirling
}  // namespace px
