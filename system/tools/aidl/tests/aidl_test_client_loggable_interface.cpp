/*
 * Copyright (C) 2020 The Android Open Source Project
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

#include <android/aidl/loggable/ILoggableInterface.h>

#include "aidl_test_client.h"

#include <android/aidl/loggable/BpLoggableInterface.h>
#include <android/aidl/tests/BackendType.h>
#include <binder/IServiceManager.h>
#include <binder/ParcelFileDescriptor.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <utils/String16.h>

namespace {

using ::android::String16;
using ::android::aidl::loggable::BpLoggableInterface;
using ::android::aidl::loggable::Data;
using ::android::aidl::loggable::Enum;
using ::android::aidl::loggable::ILoggableInterface;
using ::android::aidl::tests::BackendType;
using ::testing::ElementsAre;
using ::testing::Pair;

TEST_F(AidlTest, LoggableInterface) {
  BackendType backendType;
  auto status = service->getBackendType(&backendType);
  EXPECT_TRUE(status.isOk());
  if (backendType != BackendType::CPP) GTEST_SKIP();

  android::sp<ILoggableInterface> loggable =
      android::waitForService<ILoggableInterface>(ILoggableInterface::descriptor);
  ASSERT_NE(nullptr, loggable);

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
  String16 stringValue("def");
  std::vector<String16> stringArray = {String16("ghi"), String16("jkl")};
  std::vector<String16> listValue = {String16("mno")};
  Data dataValue;
  dataValue.num = 42;
  dataValue.str = "abc";
  dataValue.nestedUnion = "def";
  dataValue.nestedEnum = Enum::FOO;
  android::sp<android::IBinder> binderValue;
  std::optional<android::os::ParcelFileDescriptor> pfdValue;
  std::vector<android::os::ParcelFileDescriptor> pfdArray;
  std::vector<String16> aidl_return;

  status = loggable->LogThis(boolValue, &boolArray, byteValue, &byteArray, charValue, &charArray,
                             intValue, &intArray, longValue, &longArray, floatValue, &floatArray,
                             doubleValue, &doubleArray, stringValue, &stringArray, &listValue,
                             dataValue, binderValue, &pfdValue, &pfdArray, &aidl_return);
  EXPECT_TRUE(status.isOk());
  EXPECT_THAT(aidl_return, ElementsAre(String16("loggable")));

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
          Pair("boolValue", "true"),
          Pair("boolArray", "[false, true]"),
          Pair("byteValue", "41"),
          Pair("byteArray", "[42, 43]"),
          Pair("charValue", "120"),
          Pair("charArray", "[97, 98, 99]"),
          Pair("intValue", "44"),
          Pair("intArray", "[45, 46]"),
          Pair("longValue", "47"),
          Pair("longArray", "[48, 49]"),
          Pair("floatValue", "50.000000"),
          Pair("floatArray", "[51.000000, 52.000000]"),
          Pair("doubleValue", "52.000000"),
          Pair("doubleArray", "[53.000000, 54.000000]"),
          Pair("stringValue", "def"),
          Pair("stringArray", "[ghi, jkl]"),
          Pair("listValue", "[mno]"),
          Pair("dataValue",
               "Data{num: 42, str: abc, nestedUnion: Union{str: def}, nestedEnum: FOO}"),
          Pair("binderValue", "(null)"),
          Pair("pfdValue", "(null)"),
          Pair("pfdArray", "[]")));
  EXPECT_THAT(log.output_args, ElementsAre(Pair("boolArray", "[false, true]"),
                                           Pair("byteArray", "[42, 43]"),
                                           Pair("charArray", "[97, 98, 99]"),
                                           Pair("intArray", "[45, 46]"),
                                           Pair("longArray", "[48, 49]"),
                                           Pair("floatArray", "[51.000000, 52.000000]"),
                                           Pair("doubleArray", "[53.000000, 54.000000]"),
                                           Pair("stringArray", "[ghi, jkl]"),
                                           Pair("listValue", "[mno]"),
                                           Pair("pfdValue", "(null)"),
                                           Pair("pfdArray", "[]")));
  // clang-format on
}

}  // namespace
