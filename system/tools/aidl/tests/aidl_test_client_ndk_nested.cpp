/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/String16.h>

#include <memory>
#include <optional>
#include <vector>

#include "aidl/android/aidl/tests/nested/INestedService.h"
#include "aidl/android/aidl/tests/nested/ParcelableWithNested.h"

namespace {

using ::aidl::android::aidl::tests::nested::INestedService;
using ::aidl::android::aidl::tests::nested::ParcelableWithNested;
using ::testing::ElementsAre;
using ::testing::Optional;

using NestedResult = ::aidl::android::aidl::tests::nested::INestedService::Result;
using NestedStatus = ::aidl::android::aidl::tests::nested::ParcelableWithNested::Status;

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

TEST(AidlNdkNestedTest, NestedService) {
  auto nestedService = getService<INestedService>();
  ASSERT_NE(nullptr, nestedService);

  ParcelableWithNested p;
  p.status = NestedStatus::OK;
  NestedResult r;
  // OK -> NOT_OK
  auto status = nestedService->flipStatus(p, &r);
  EXPECT_TRUE(status.isOk());
  EXPECT_EQ(r.status, NestedStatus::NOT_OK);

  // NOT_OK -> OK with callback (nested interface)
  struct Callback : INestedService::BnCallback {
    std::optional<ParcelableWithNested::Status> result;
    ndk::ScopedAStatus done(ParcelableWithNested::Status st) override {
      result = st;
      return ndk::ScopedAStatus::ok();
    }
  };
  auto cb = ndk::SharedRefBase::make<Callback>();
  status = nestedService->flipStatusWithCallback(r.status, cb);
  EXPECT_TRUE(status.isOk());
  EXPECT_THAT(cb->result, Optional(NestedStatus::OK));

  // android::enum_ranges<>
  std::vector<NestedStatus> values{ndk::enum_range<NestedStatus>().begin(),
                                   ndk::enum_range<NestedStatus>().end()};
  EXPECT_THAT(values, ElementsAre(NestedStatus::OK, NestedStatus::NOT_OK));

  // toString()
  EXPECT_EQ(toString(NestedStatus::NOT_OK), "NOT_OK");
}

}  // namespace
