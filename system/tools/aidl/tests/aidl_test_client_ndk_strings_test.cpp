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
#include "gtest/gtest.h"

namespace {

using ::aidl::android::aidl::tests::BackendType;
using ::aidl::android::aidl::tests::ITestService;
using ::testing::ContainerEq;
using ::testing::Optional;
using ::testing::Values;

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

class AidlNdkStringsTest : public ::testing::Test {
 protected:
  void SetUp() override {
    service_ = getService<ITestService>();
    auto status = service_->getBackendType(&backend_type_);
    ASSERT_TRUE(status.isOk()) << status.getDescription();
  }
  std::shared_ptr<ITestService> service_;
  BackendType backend_type_;
};

const std::string_view kUtf8Inputs[] = {
    "Deliver us from evil.",
    "",
    std::string_view("\0\0", 2),
    // The utf8 encodings of the small letter yee and euro sign.
    "\xF0\x90\x90\xB7\xE2\x82\xAC",
    ITestService::STRING_CONSTANT_UTF8,
};

TEST_F(AidlNdkStringsTest, RepeatUtf8String) {
  for (const auto& input : kUtf8Inputs) {
    std::string reply;
    auto status = service_->RepeatUtf8CppString(std::string(input), &reply);
    EXPECT_TRUE(status.isOk());
    EXPECT_EQ(reply, input);
  }
}

TEST_F(AidlNdkStringsTest, RepeatNullableUtf8String) {
  for (const auto& input : kUtf8Inputs) {
    std::optional<std::string> reply;
    auto status = service_->RepeatNullableUtf8CppString(std::string(input), &reply);
    EXPECT_TRUE(status.isOk());
    EXPECT_THAT(reply, Optional(input));
  }
}

TEST_F(AidlNdkStringsTest, RepeatNullableUtf8StringEmpty) {
  std::optional<std::string> reply;
  auto status = service_->RepeatNullableUtf8CppString(std::nullopt, &reply);
  EXPECT_TRUE(status.isOk());
  EXPECT_EQ(reply, std::nullopt);
}

TEST_F(AidlNdkStringsTest, ReverseUtf8StringArray) {
  std::vector<std::string> input = {"a", "", "\xc3\xb8"};
  std::vector<std::string> repeated;
  if (backend_type_ == BackendType::JAVA) {
    repeated = decltype(input)(input.size());
  }
  std::vector<std::string> reversed;

  auto status = service_->ReverseUtf8CppString(input, &repeated, &reversed);
  ASSERT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_THAT(repeated, ContainerEq(input));

  std::vector<std::string> reversed_input{input.crbegin(), input.crend()};
  EXPECT_THAT(reversed, ContainerEq(reversed_input));
}

using StringMethodPtr = decltype(&ITestService::ReverseNullableUtf8CppString);
using OptStringVector = std::optional<std::vector<std::optional<std::string>>>;

struct AidlNdkStringArrayTest : public AidlNdkStringsTest,
                                public ::testing::WithParamInterface<StringMethodPtr> {};

TEST_P(AidlNdkStringArrayTest, RepeatEmpty) {
  OptStringVector input, repeated, reversed;

  auto status = (*service_.*GetParam())(input, &repeated, &reversed);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  if (GetParam() == &ITestService::ReverseUtf8CppStringList && backend_type_ == BackendType::JAVA) {
    // Java cannot clear the input variable to return a null value. It can
    // only ever fill out a list.
    EXPECT_TRUE(repeated.has_value());
  } else {
    EXPECT_FALSE(repeated.has_value());
  }
  EXPECT_FALSE(reversed.has_value());
}

TEST_P(AidlNdkStringArrayTest, RepeatNonempty) {
  OptStringVector input;
  OptStringVector repeated;
  OptStringVector reversed;
  input.emplace();
  input->push_back("Deliver us from evil.");
  input->push_back(std::nullopt);
  input->push_back("\xF0\x90\x90\xB7\xE2\x82\xAC");

  // usable size needs to be initialized for Java
  repeated.emplace(input->size());

  auto status = (*service_.*GetParam())(input, &repeated, &reversed);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  EXPECT_THAT(repeated, Optional(ContainerEq(*input)));
  EXPECT_TRUE(repeated.has_value());
  OptStringVector reversed_input;
  reversed_input.emplace(input->crbegin(), input->crend());
  EXPECT_THAT(reversed, Optional(ContainerEq(*reversed_input)));
}

INSTANTIATE_TEST_SUITE_P(Methods, AidlNdkStringArrayTest,
                         Values(&ITestService::ReverseUtf8CppStringList,
                                &ITestService::ReverseNullableUtf8CppString));

}  // namespace
