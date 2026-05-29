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
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "aidl/android/aidl/tests/ITestService.h"
#include "gmock/gmock.h"

namespace {

using ::aidl::android::aidl::tests::BackendType;
using ::aidl::android::aidl::tests::INamedCallback;
using ::aidl::android::aidl::tests::ITestService;
using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Optional;

using TestServiceVector = std::vector<std::optional<ITestService::Empty>>;

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

class AidlNdkNullablesTest : public ::testing::Test {
 protected:
  void SetUp() override {
    service_ = getService<ITestService>();
    auto status = service_->getBackendType(&backend_type_);
    ASSERT_TRUE(status.isOk()) << status.getDescription();
  }

  std::shared_ptr<ITestService> service_;
  BackendType backend_type_;
};

TEST_F(AidlNdkNullablesTest, ParcelableArray) {
  TestServiceVector input = {ITestService::Empty(), std::nullopt};
  std::optional<TestServiceVector> output;
  ndk::ScopedAStatus status = service_->RepeatNullableParcelableArray(input, &output);
  ASSERT_TRUE(status.isOk());
  EXPECT_THAT(output, Optional(Eq(input)));
}

TEST_F(AidlNdkNullablesTest, ParcelableArrayEmpty) {
  TestServiceVector input = {};
  std::optional<TestServiceVector> output;
  ndk::ScopedAStatus status = service_->RepeatNullableParcelableArray(input, &output);
  ASSERT_TRUE(status.isOk());
  EXPECT_THAT(output, Optional(IsEmpty()));
}

TEST_F(AidlNdkNullablesTest, ParcelableList) {
  TestServiceVector input = {ITestService::Empty(), std::nullopt};
  std::optional<TestServiceVector> output;
  ndk::ScopedAStatus status = service_->RepeatNullableParcelableList(input, &output);
  ASSERT_TRUE(status.isOk());
  EXPECT_THAT(output, Optional(Eq(input)));
}

TEST_F(AidlNdkNullablesTest, ParcelableListEmpty) {
  TestServiceVector input = {};
  std::optional<TestServiceVector> output;
  ndk::ScopedAStatus status = service_->RepeatNullableParcelableList(input, &output);
  ASSERT_TRUE(status.isOk());
  EXPECT_THAT(output, Optional(IsEmpty()));
}

TEST_F(AidlNdkNullablesTest, NullBinder) {
  auto status = service_->TakesAnIBinder(nullptr);
  ASSERT_THAT(status.getStatus(), Eq(STATUS_UNEXPECTED_NULL)) << status.getDescription();
  // Note that NDK backend checks null before transaction while C++ backends doesn't.
}

TEST_F(AidlNdkNullablesTest, BinderListWithNull) {
  std::vector<ndk::SpAIBinder> input{service_->asBinder(), nullptr};
  auto status = service_->TakesAnIBinderList(input);
  ASSERT_THAT(status.getStatus(), Eq(STATUS_UNEXPECTED_NULL));
  // Note that NDK backend checks null before transaction while C++ backends doesn't.
}

TEST_F(AidlNdkNullablesTest, NonNullBinder) {
  auto status = service_->TakesAnIBinder(service_->asBinder());
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlNdkNullablesTest, BinderListWithoutNull) {
  std::vector<ndk::SpAIBinder> input{service_->asBinder()};
  auto status = service_->TakesAnIBinderList(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlNdkNullablesTest, NullBinderToAnnotatedMethod) {
  auto status = service_->TakesANullableIBinder(nullptr);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlNdkNullablesTest, BinderListWithNullToAnnotatedMethod) {
  std::vector<ndk::SpAIBinder> input{service_->asBinder(), nullptr};
  auto status = service_->TakesANullableIBinderList(input);
  ASSERT_TRUE(status.isOk());
}

TEST_F(AidlNdkNullablesTest, BinderArray) {
  std::vector<ndk::SpAIBinder> repeated;
  if (backend_type_ == BackendType::JAVA) {
    // Java can only modify out-argument arrays in-place
    repeated.resize(2);
  }
  // get INamedCallback for "SpAIBinder" object
  std::shared_ptr<INamedCallback> callback;
  auto status = service_->GetCallback(false, &callback);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  std::vector<ndk::SpAIBinder> reversed;
  std::vector<ndk::SpAIBinder> input{service_->asBinder(), callback->asBinder()};
  status = service_->ReverseIBinderArray(input, &repeated, &reversed);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  EXPECT_THAT(input, Eq(repeated));
  std::reverse(std::begin(reversed), std::end(reversed));
  EXPECT_THAT(input, Eq(reversed));
}

TEST_F(AidlNdkNullablesTest, NullableBinderArray) {
  std::optional<std::vector<ndk::SpAIBinder>> repeated;
  if (backend_type_ == BackendType::JAVA) {
    // Java can only modify out-argument arrays in-place
    repeated.emplace(2, ndk::SpAIBinder());
  }

  std::optional<std::vector<ndk::SpAIBinder>> reversed;
  std::optional<std::vector<ndk::SpAIBinder>> input =
      std::vector<ndk::SpAIBinder>{service_->asBinder(), service_->asBinder()};
  auto status = service_->ReverseNullableIBinderArray(input, &repeated, &reversed);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  EXPECT_THAT(input, Eq(repeated));
  ASSERT_TRUE(reversed);
  std::reverse(std::begin(*reversed), std::end(*reversed));
  EXPECT_THAT(input, Eq(reversed));
}

}  // namespace
