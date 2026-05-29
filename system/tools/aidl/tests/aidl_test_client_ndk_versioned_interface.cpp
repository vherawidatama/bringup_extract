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

#include "aidl/android/aidl/tests/BackendType.h"
#include "aidl/android/aidl/tests/ITestService.h"
#include "aidl/android/aidl/versioned/tests/IFooInterface.h"

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {

using ::aidl::android::aidl::tests::BackendType;
using ::aidl::android::aidl::tests::ITestService;
using ::aidl::android::aidl::versioned::tests::BazUnion;
using ::aidl::android::aidl::versioned::tests::Foo;
using ::aidl::android::aidl::versioned::tests::IFooInterface;

class AidlNdkVersionedInterfaceTest : public ::testing::Test {
 protected:
  void SetUp() override {
    android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
    android::ProcessState::self()->startThreadPool();
    ndk::SpAIBinder binder =
        ndk::SpAIBinder(AServiceManager_waitForService(IFooInterface::descriptor));
    versioned_ = IFooInterface::fromBinder(binder);
    ASSERT_NE(nullptr, versioned_);

    ndk::SpAIBinder testServiceBinder =
        ndk::SpAIBinder(AServiceManager_waitForService(ITestService::descriptor));
    auto service = ITestService::fromBinder(testServiceBinder);
    auto status = service->getBackendType(&backend_type_);
    EXPECT_TRUE(status.isOk()) << status.getDescription();
  }

  std::shared_ptr<IFooInterface> versioned_;
  BackendType backend_type_;
};

TEST_F(AidlNdkVersionedInterfaceTest, GetInterfaceVersion) {
  int32_t version = 0;
  auto status = versioned_->getInterfaceVersion(&version);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(version, 1);
}

TEST_F(AidlNdkVersionedInterfaceTest, GetInterfaceHash) {
  std::string hash;
  auto status = versioned_->getInterfaceHash(&hash);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(hash, "9e7be1859820c59d9d55dd133e71a3687b5d2e5b");
}

TEST_F(AidlNdkVersionedInterfaceTest, UnionWithOldField) {
  std::string result;
  auto status =
      versioned_->acceptUnionAndReturnString(BazUnion::make<BazUnion::intNum>(42), &result);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(result, "42");
}

TEST_F(AidlNdkVersionedInterfaceTest, UnionWithNewField) {
  std::string result;
  auto status =
      versioned_->acceptUnionAndReturnString(BazUnion::make<BazUnion::longNum>(42L), &result);
  // b/173458620 - Java and C++ return different errors
  if (backend_type_ == BackendType::JAVA) {
    EXPECT_EQ(status.getExceptionCode(), EX_ILLEGAL_ARGUMENT);
  } else {
    EXPECT_EQ(status.getStatus(), STATUS_BAD_VALUE);
  }
}

TEST_F(AidlNdkVersionedInterfaceTest, ArrayOfParcelableWithNewField) {
  std::vector<Foo> foos(42);
  int32_t length;
  auto status = versioned_->returnsLengthOfFooArray(foos, &length);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(42, length);
}

TEST_F(AidlNdkVersionedInterfaceTest, ReadDataCorrectlyAfterParcelableWithNewField) {
  Foo inFoo, inoutFoo, outFoo;
  inoutFoo.intDefault42 = 0;
  outFoo.intDefault42 = 0;
  int32_t ret;
  auto status = versioned_->ignoreParcelablesAndRepeatInt(inFoo, &inoutFoo, &outFoo, 43, &ret);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(ret, 43);
  EXPECT_EQ(inoutFoo.intDefault42, 0);
  EXPECT_EQ(outFoo.intDefault42, 0);
}

TEST_F(AidlNdkVersionedInterfaceTest, ErrorWhenCallingV2Api) {
  auto status = versioned_->newApi();
  EXPECT_EQ(status.getStatus(), STATUS_UNKNOWN_TRANSACTION);
}

}  // namespace
