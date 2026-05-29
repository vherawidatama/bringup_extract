/*
 * Copyright (C) 2023 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aidl/transaction_ids.h>

#include "aidl_to_common.h"
#include "code_writer.h"
#include "options.h"

namespace android {
namespace aidl {

static constexpr size_t kMaxSkip = 10;

bool ShouldForceDowngradeFor(CommunicationSide e) {
  return kDowngradeCommunicationBitmap & static_cast<int>(e);
}

void GenerateAutoGenHeader(CodeWriter& out, const Options& options) {
  out << "/*\n";
  out << " * This file is auto-generated.  DO NOT MODIFY.\n";
  out << " * Using: " << MultilineCommentEscape(options.RawArgs()) << "\n";
  out << " *\n";
  out << " * DO NOT CHECK THIS FILE INTO A CODE TREE (e.g. git, etc..).\n";
  out << " * ALWAYS GENERATE THIS FILE FROM UPDATED AIDL COMPILER\n";
  out << " * AS A BUILD INTERMEDIATE ONLY. THIS IS NOT SOURCE CODE.\n";
  out << " */\n";
}

int GetMaxId(const AidlInterface& defined_type) {
  std::vector<int> ids;
  for (const auto& method : defined_type.GetMethods()) ids.push_back(method->GetId());
  std::sort(ids.begin(), ids.end());

  int last_id = -1;
  size_t total_skipped = 0;
  for (int id : ids) {
    if (id > kMaxUserSetMethodId) break;
    AIDL_FATAL_IF(id <= last_id, defined_type) << id << last_id;
    total_skipped += last_id == -1 ? 0 : id - last_id - 1;
    if (total_skipped > kMaxSkip) return last_id;
    last_id = id;
  }
  return last_id;
}

std::vector<std::string> GetFunctionNames(const AidlInterface& defined_type) {
  // Find the maxId used for AIDL method. If methods use skipped ids, only support till kMaxSkip.
  int maxId = GetMaxId(defined_type);
  int functionCount = maxId + 1;
  // If tracing is off, don't populate this array. libbinder_ndk will still add traces based on
  // transaction code
  vector<std::string> functionNames;
  if (maxId != -1) {
    functionNames.resize(functionCount);

    // Assign method names to the proper places in array
    for (const auto& method : defined_type.GetMethods()) {
      if (method->GetId() >= functionCount) {
        continue;
      }
      functionNames[method->GetId()] = method->GetName();
    }
  }
  return functionNames;
}

}  // namespace aidl
}  // namespace android
