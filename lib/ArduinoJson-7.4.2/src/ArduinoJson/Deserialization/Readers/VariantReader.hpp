// ArduinoJson - https://arduinojson.org
// Copyright © 2014-2025, Benoit BLANCHON
// MIT License

#pragma once

#include "../../Object/MemberProxy.hpp"
#include "../../Variant/JsonVariantConst.hpp"

ARDUINOJSON_BEGIN_PRIVATE_NAMESPACE

template <typename TVariant>
struct Reader<TVariant, enable_if_t<IsVariant<TVariant>::value>>
    : Reader<char*, void> {
  explicit Reader(const TVariant& x)
      : Reader<char*, void>(x.template as<const char*>()) {}
};

ARDUINOJSON_END_PRIVATE_NAMESPACE
