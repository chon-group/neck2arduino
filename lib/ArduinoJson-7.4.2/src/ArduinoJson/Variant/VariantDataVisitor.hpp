// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include "../Array/ArrayData.hpp"
#include "../Numbers/JsonFloat.hpp"
#include "../Numbers/JsonInteger.hpp"
#include "../Object/ObjectData.hpp"

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TResult>
struct VariantDataVisitor {
  using result_type = TResult;

  template <typename T>
  TResult visit(const T&) {
    return TResult();
  }
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
