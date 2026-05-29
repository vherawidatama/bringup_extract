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

#include <functional>

#include <android/binder_auto_utils.h>
#include <android/binder_manager.h>
#include <binder/ProcessState.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "aidl/android/aidl/fixedsizearray/FixedSizeArrayExample.h"
#include "aidl/android/aidl/tests/ITestService.h"
#include "aidl/android/aidl/tests/RecursiveList.h"
#include "aidl/android/aidl/tests/Union.h"
#include "aidl/android/aidl/tests/extension/MyExt.h"
#include "aidl/android/aidl/tests/vintf/VintfExtendableParcelable.h"
#include "aidl/android/aidl/tests/vintf/VintfParcelable.h"

namespace {

using ::aidl::android::aidl::fixedsizearray::FixedSizeArrayExample;
using ::aidl::android::aidl::tests::BackendType;
using ::aidl::android::aidl::tests::ITestService;
using ::aidl::android::aidl::tests::RecursiveList;
using ::aidl::android::aidl::tests::SimpleParcelable;
using ::aidl::android::aidl::tests::Union;
using ::aidl::android::aidl::tests::extension::ExtendableParcelable;
using ::aidl::android::aidl::tests::extension::MyExt;
using ::aidl::android::aidl::tests::vintf::VintfExtendableParcelable;
using ::aidl::android::aidl::tests::vintf::VintfParcelable;
using ::ndk::AParcel_readData;
using ::ndk::AParcel_writeData;
using ::testing::ElementsAre;

using BnRepeatFixedSizeArray =
    aidl::android::aidl::fixedsizearray::FixedSizeArrayExample::BnRepeatFixedSizeArray;
using BpRepeatFixedSizeArray =
    aidl::android::aidl::fixedsizearray::FixedSizeArrayExample::BpRepeatFixedSizeArray;
using IntParcelable = aidl::android::aidl::fixedsizearray::FixedSizeArrayExample::IntParcelable;
using IRepeatFixedSizeArray =
    aidl::android::aidl::fixedsizearray::FixedSizeArrayExample::IRepeatFixedSizeArray;
using BnEmptyInterface =
    aidl::android::aidl::fixedsizearray::FixedSizeArrayExample::BnEmptyInterface;

template <typename T>
std::shared_ptr<T> getService() {
  android::ProcessState::self()->setThreadPoolMaxThreadCount(1);
  android::ProcessState::self()->startThreadPool();
  ndk::SpAIBinder binder = ndk::SpAIBinder(AServiceManager_waitForService(T::descriptor));
  return T::fromBinder(binder);
}

// TODO(b/196454897): copy more tests from aidl_test_client

TEST(AidlNdkParcelablesTest, RepeatSimpleParcelable) {
  SimpleParcelable input("foo", 42);
  SimpleParcelable out_param, returned;
  auto status = getService<ITestService>()->RepeatSimpleParcelable(input, &out_param, &returned);
  ASSERT_TRUE(status.isOk());
  EXPECT_EQ(input, out_param) << input.toString() << " " << out_param.toString();
  EXPECT_EQ(input, returned) << input.toString() << " " << returned.toString();
}

TEST(AidlNdkParcelablesTest, ReverseSimpleParcelable) {
  BackendType backend;
  auto status = getService<ITestService>()->getBackendType(&backend);
  ASSERT_TRUE(status.isOk());

  const std::vector<SimpleParcelable> original{SimpleParcelable("a", 1), SimpleParcelable("b", 2),
                                               SimpleParcelable("c", 3)};
  std::vector<SimpleParcelable> repeated;
  if (backend == BackendType::JAVA) {
    repeated = std::vector<SimpleParcelable>(original.size());
  }
  std::vector<SimpleParcelable> reversed;
  status = getService<ITestService>()->ReverseSimpleParcelables(original, &repeated, &reversed);
  ASSERT_TRUE(status.isOk());

  EXPECT_EQ(repeated, original);

  std::reverse(reversed.begin(), reversed.end());
  EXPECT_EQ(reversed, original);
}

TEST(AidlNdkParcelablesTest, ReverseRecursiveList) {
  std::unique_ptr<RecursiveList> head;
  for (int i = 0; i < 10; i++) {
    auto node = std::make_unique<RecursiveList>();
    node->value = i;
    node->next = std::move(head);
    head = std::move(node);
  }
  // head: [9, 8, ... 0]

  RecursiveList reversed;
  auto status = getService<ITestService>()->ReverseList(*head, &reversed);
  ASSERT_TRUE(status.isOk());

  // reversed should be [0, 1, .. 9]
  RecursiveList* cur = &reversed;
  for (int i = 0; i < 10; i++) {
    EXPECT_EQ(i, cur->value);
    cur = cur->next.get();
  }
  EXPECT_EQ(nullptr, cur);
}

TEST(AidlNdkParcelablesTest, RepeatExtendableParcelable) {
  MyExt ext;
  ext.a = 42;
  ext.b = "EXT";
  ExtendableParcelable ep;
  ep.a = 1;
  ep.b = "a";
  ep.ext.setParcelable(ext);

  ExtendableParcelable ep2;
  auto status = getService<ITestService>()->RepeatExtendableParcelable(ep, &ep2);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  EXPECT_EQ(ep2.a, ep.a);
  EXPECT_EQ(ep2.b, ep.b);

  std::optional<MyExt> ret_ext;
  ASSERT_EQ(ep2.ext.getParcelable(&ret_ext), android::OK);
  ASSERT_TRUE(ret_ext.has_value());

  EXPECT_EQ(ret_ext->a, ext.a);
  EXPECT_EQ(ret_ext->b, ext.b);
}

TEST(AidlNdkParcelablesTest, RepeatExtendableParcelableVintf) {
  VintfParcelable inner;
  inner.a = 5;

  VintfExtendableParcelable ext;
  ext.ext.setParcelable(inner);

  ExtendableParcelable ep;
  ep.a = 1;
  ep.b = "a";
  ep.ext.setParcelable(ext);

  ExtendableParcelable ep2;
  auto status = getService<ITestService>()->RepeatExtendableParcelableVintf(ep, &ep2);
  ASSERT_TRUE(status.isOk()) << status.getDescription();

  EXPECT_EQ(ep2.a, ep.a);
  EXPECT_EQ(ep2.b, ep.b);

  std::optional<VintfExtendableParcelable> ret_ext;
  ASSERT_EQ(ep2.ext.getParcelable(&ret_ext), android::OK);
  ASSERT_TRUE(ret_ext.has_value());

  std::optional<VintfParcelable> ret_inner;
  ASSERT_EQ(ret_ext->ext.getParcelable(&ret_inner), android::OK);
  ASSERT_TRUE(ret_inner.has_value());
  EXPECT_EQ(ret_inner->a, inner.a);
}

TEST(AidlNdkParcelablesTest, GetUnionTags) {
  std::vector<Union> unions;
  std::vector<Union::Tag> tags;
  // test empty
  auto status = getService<ITestService>()->GetUnionTags(unions, &tags);
  ASSERT_TRUE(status.isOk());
  EXPECT_EQ(tags, (std::vector<Union::Tag>{}));
  // test non-empty
  unions.push_back(Union::make<Union::n>());
  unions.push_back(Union::make<Union::ns>());
  status = getService<ITestService>()->GetUnionTags(unions, &tags);
  ASSERT_TRUE(status.isOk());
  EXPECT_EQ(tags, (std::vector<Union::Tag>{Union::n, Union::ns}));
}

TEST(AidlNdkParcelablesTest, FixedSizeArray) {
  AParcel* parcel = AParcel_create();

  FixedSizeArrayExample p;
  p.byteMatrix[0][0] = 0;
  p.byteMatrix[0][1] = 1;
  p.byteMatrix[1][0] = 2;
  p.byteMatrix[1][1] = 3;
  p.floatMatrix[0][0] = 0.f;
  p.floatMatrix[0][1] = 1.f;
  p.floatMatrix[1][0] = 2.f;
  p.floatMatrix[1][1] = 3.f;
  EXPECT_EQ(p.writeToParcel(parcel), android::OK);

  AParcel_setDataPosition(parcel, 0);

  FixedSizeArrayExample q;
  EXPECT_EQ(q.readFromParcel(parcel), android::OK);
  EXPECT_EQ(p, q);

  AParcel_delete(parcel);
}

TEST(AidlNdkParcelablesTest, FixedSizeArrayWithValuesAtNullableFields) {
  AParcel* parcel = AParcel_create();

  FixedSizeArrayExample p;
  p.boolNullableArray = std::array<bool, 2>{true, false};
  p.byteNullableArray = std::array<uint8_t, 2>{42, 0};
  p.stringNullableArray = std::array<std::optional<std::string>, 2>{"hello", "world"};

  p.boolNullableMatrix.emplace();
  p.boolNullableMatrix->at(0) = std::array<bool, 2>{true, false};
  p.byteNullableMatrix.emplace();
  p.byteNullableMatrix->at(0) = std::array<uint8_t, 2>{42, 0};
  p.stringNullableMatrix.emplace();
  p.stringNullableMatrix->at(0) = std::array<std::optional<std::string>, 2>{"hello", "world"};

  EXPECT_EQ(p.writeToParcel(parcel), android::OK);

  AParcel_setDataPosition(parcel, 0);

  FixedSizeArrayExample q;
  EXPECT_EQ(q.readFromParcel(parcel), android::OK);
  EXPECT_EQ(q, p);

  AParcel_delete(parcel);
}

TEST(AidlNdkParcelablesTest, FixedSizeArrayOfBytesShouldBePacked) {
  AParcel* parcel = AParcel_create();

  std::array<std::array<uint8_t, 3>, 2> byte_array;
  byte_array[0] = {1, 2, 3};
  byte_array[1] = {4, 5, 6};
  EXPECT_EQ(AParcel_writeData(parcel, byte_array), android::OK);

  AParcel_setDataPosition(parcel, 0);

  int32_t len;
  EXPECT_EQ(AParcel_readData(parcel, &len), android::OK);
  EXPECT_EQ(2, len);
  std::vector<uint8_t> byte_vector;
  EXPECT_EQ(AParcel_readData(parcel, &byte_vector), android::OK);
  EXPECT_THAT(byte_vector, ElementsAre(1, 2, 3));
  EXPECT_EQ(AParcel_readData(parcel, &byte_vector), android::OK);
  EXPECT_THAT(byte_vector, ElementsAre(4, 5, 6));

  AParcel_delete(parcel);
}

template <typename Service, typename MemFn, typename Input>
void CheckRepeat(Service service, MemFn fn, Input input) {
  Input out1, out2;
  EXPECT_TRUE(std::invoke(fn, service, input, &out1, &out2).isOk());
  EXPECT_EQ(input, out1);
  EXPECT_EQ(input, out2);
}

template <typename T>
std::array<std::array<T, 3>, 2> Make2dArray(std::initializer_list<T> values) {
  std::array<std::array<T, 3>, 2> arr = {};
  auto it = std::begin(values);
  for (auto& row : arr) {
    for (auto& el : row) {
      if (it == std::end(values)) break;
      el = *it++;
    }
  }
  return arr;
}

TEST(AidlNdkParcelablesTest, FixedSizeArrayOverBinder) {
  auto service = getService<IRepeatFixedSizeArray>();

  CheckRepeat(service, &IRepeatFixedSizeArray::RepeatBytes, (std::array<uint8_t, 3>{1, 2, 3}));

  CheckRepeat(service, &IRepeatFixedSizeArray::RepeatInts, (std::array<int32_t, 3>{1, 2, 3}));

  auto binder1 = ndk::SharedRefBase::make<BnEmptyInterface>()->asBinder();
  auto binder2 = ndk::SharedRefBase::make<BnEmptyInterface>()->asBinder();
  auto binder3 = ndk::SharedRefBase::make<BnEmptyInterface>()->asBinder();
  CheckRepeat(service, &IRepeatFixedSizeArray::RepeatBinders,
              (std::array<ndk::SpAIBinder, 3>{binder1, binder2, binder3}));

  IntParcelable p1, p2, p3;
  p1.value = 1;
  p2.value = 2;
  p3.value = 3;
  CheckRepeat(service, &IRepeatFixedSizeArray::RepeatParcelables,
              (std::array<IntParcelable, 3>{p1, p2, p3}));

  CheckRepeat(service, &IRepeatFixedSizeArray::Repeat2dBytes, Make2dArray<uint8_t>({1, 2, 3}));

  CheckRepeat(service, &IRepeatFixedSizeArray::Repeat2dInts, Make2dArray<int32_t>({1, 2, 3}));

  // Not-nullable
  CheckRepeat(service, &IRepeatFixedSizeArray::Repeat2dBinders,
              Make2dArray<ndk::SpAIBinder>({binder1, binder2, binder3, binder1, binder2, binder3}));

  CheckRepeat(service, &IRepeatFixedSizeArray::Repeat2dParcelables,
              Make2dArray<IntParcelable>({p1, p2, p3}));
}

}  // namespace
