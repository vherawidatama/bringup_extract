/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include <android-base/logging.h>
#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "aidl/android/aidl/tests/BnTestService.h"

namespace {

using ::aidl::android::aidl::tests::ITestService;
using ::aidl::android::aidl::tests::ITestServiceDelegator;

constexpr int8_t kCustomByte = 8;

static_assert(std::is_same<ITestService::DefaultDelegator, ITestServiceDelegator>::value);

struct CustomDelegator : public ITestServiceDelegator {
 public:
  CustomDelegator(std::shared_ptr<ITestService>& impl) : ITestServiceDelegator(impl) {}

  // Change RepeatByte to always return the same byte.
  ndk::ScopedAStatus RepeatByte(int8_t /* token */, int8_t* aidl_return) override {
    *aidl_return = kCustomByte;
    return ndk::ScopedAStatus::ok();
  }
};

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

class AidlNdkDelegateTest : public ::testing::Test {
 protected:
  std::shared_ptr<ITestService> service = getService<ITestService>();
};

TEST_F(AidlNdkDelegateTest, SimpleDelegator) {
  auto delegator = ndk::SharedRefBase::make<ITestServiceDelegator>(service);
  int8_t returned_value = 0;
  auto status = delegator->RepeatByte(12, &returned_value);
  ASSERT_TRUE(status.isOk()) << status.getMessage();
  EXPECT_EQ(returned_value, 12);
}

TEST_F(AidlNdkDelegateTest, CustomDelegator) {
  auto delegator = ndk::SharedRefBase::make<CustomDelegator>(service);
  int8_t returned_value = 0;
  auto status = delegator->RepeatByte(12, &returned_value);
  ASSERT_TRUE(status.isOk()) << status.getMessage();
  EXPECT_EQ(returned_value, kCustomByte);
}

TEST_F(AidlNdkDelegateTest, SendDelegator) {
  auto delegator = ndk::SharedRefBase::make<ITestServiceDelegator>(service);
  auto fromAsBinder = ITestServiceDelegator::fromBinder(delegator->asBinder());
  // Make sure the delegator works after asBinder -> fromBinder conversions
  int8_t returned_value = 0;
  auto status = fromAsBinder->RepeatByte(12, &returned_value);
  ASSERT_TRUE(status.isOk()) << status.getDescription();
  EXPECT_EQ(returned_value, 12);
}

}  // namespace
