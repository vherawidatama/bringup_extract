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

#include <aidl/android/aidl/loggable/ILoggableInterface.h>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <aidl/android/aidl/loggable/BpLoggableInterface.h>
#include <aidl/android/aidl/tests/BackendType.h>
#include <aidl/android/aidl/tests/ITestService.h>

namespace {

using ::aidl::android::aidl::loggable::BpLoggableInterface;
using ::aidl::android::aidl::loggable::Data;
using ::aidl::android::aidl::loggable::Enum;
using ::aidl::android::aidl::loggable::ILoggableInterface;
using ::aidl::android::aidl::tests::BackendType;
using ::aidl::android::aidl::tests::ITestService;
using ::testing::ElementsAre;
using ::testing::Pair;

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

TEST(AidlNdkLoggableInterfaceTest, LogThis) {
  std::shared_ptr<ITestService> service = getService<ITestService>();
  ASSERT_NE(nullptr, service.get());

  BackendType backendType;
  ndk::ScopedAStatus status = service->getBackendType(&backendType);
  EXPECT_TRUE(status.isOk()) << status.getDescription();
  if (backendType != BackendType::CPP) GTEST_SKIP();

  std::shared_ptr<ILoggableInterface> loggable = getService<ILoggableInterface>();
  ASSERT_NE(nullptr, loggable.get());

  BpLoggableInterface::TransactionLog log;
  BpLoggableInterface::logFunc = [&](const BpLoggableInterface::TransactionLog& tx) { log = tx; };

  bool boolValue = true;
  std::vector<bool> boolArray = {false, true};
  int8_t byteValue = 41;
  std::vector<uint8_t> byteArray = {42, 43};
  char16_t charValue = 120;
  std::vector<char16_t> charArray = {97, 98, 99};
  int32_t intValue = 44;
  std::vector<int32_t> intArray = {45, 46};
  int64_t longValue = 47;
  std::vector<int64_t> longArray = {48, 49};
  float floatValue = 50;
  std::vector<float> floatArray = {51, 52};
  double doubleValue = 52;
  std::vector<double> doubleArray = {53, 54};
  std::string stringValue = "def";
  std::vector<std::string> stringArray = {{"ghi"}, {"jkl"}};
  std::vector<std::string> listValue = {{"mno"}};
  Data dataValue;
  dataValue.num = 42;
  dataValue.str = "abc";
  dataValue.nestedUnion = "def";
  dataValue.nestedEnum = Enum::FOO;
  ndk::SpAIBinder binderValue;
  ndk::ScopedFileDescriptor pfdValue;
  std::vector<ndk::ScopedFileDescriptor> pfdArray;
  std::vector<std::string> aidl_return;
  status = loggable->LogThis(boolValue, &boolArray, byteValue, &byteArray, charValue, &charArray,
                             intValue, &intArray, longValue, &longArray, floatValue, &floatArray,
                             doubleValue, &doubleArray, stringValue, &stringArray, &listValue,
                             dataValue, binderValue, &pfdValue, &pfdArray, &aidl_return);
  EXPECT_TRUE(status.isOk());
  EXPECT_THAT(aidl_return, ElementsAre("loggable"));

  // check the captured log
  EXPECT_EQ(log.result, "[loggable]");
  EXPECT_EQ(log.interface_name, "android.aidl.loggable.ILoggableInterface");
  EXPECT_EQ(log.method_name, "LogThis");
  EXPECT_EQ(log.exception_code, 0);
  EXPECT_EQ(log.exception_message, "");
  EXPECT_EQ(log.transaction_error, 0);
  EXPECT_EQ(log.service_specific_error_code, 0);
  // clang-format off
  EXPECT_THAT(
      log.input_args,
      ElementsAre(
          Pair("in_boolValue", "true"),
          Pair("in_boolArray", "[false, true]"),
          Pair("in_byteValue", "41"),
          Pair("in_byteArray", "[42, 43]"),
          Pair("in_charValue", "120"),
          Pair("in_charArray", "[97, 98, 99]"),
          Pair("in_intValue", "44"),
          Pair("in_intArray", "[45, 46]"),
          Pair("in_longValue", "47"),
          Pair("in_longArray", "[48, 49]"),
          Pair("in_floatValue", "50.000000"),
          Pair("in_floatArray", "[51.000000, 52.000000]"),
          Pair("in_doubleValue", "52.000000"),
          Pair("in_doubleArray", "[53.000000, 54.000000]"),
          Pair("in_stringValue", "def"),
          Pair("in_stringArray", "[ghi, jkl]"),
          Pair("in_listValue", "[mno]"),
          Pair("in_dataValue",
               "Data{num: 42, str: abc, nestedUnion: Union{str: def}, nestedEnum: FOO}"),
          Pair("in_binderValue", "binder:0x0"),
          Pair("in_pfdValue", "fd:-1"),
          Pair("in_pfdArray", "[]")));
  EXPECT_THAT(log.output_args, ElementsAre(Pair("in_boolArray", "[false, true]"),
                                           Pair("in_byteArray", "[42, 43]"),
                                           Pair("in_charArray", "[97, 98, 99]"),
                                           Pair("in_intArray", "[45, 46]"),
                                           Pair("in_longArray", "[48, 49]"),
                                           Pair("in_floatArray", "[51.000000, 52.000000]"),
                                           Pair("in_doubleArray", "[53.000000, 54.000000]"),
                                           Pair("in_stringArray", "[ghi, jkl]"),
                                           Pair("in_listValue", "[mno]"),
                                           Pair("in_pfdValue", "fd:-1"),
                                           Pair("in_pfdArray", "[]")));
  // clang-format on
}

}  // namespace
