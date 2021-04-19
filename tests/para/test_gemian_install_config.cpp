/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <string>
#include <map>
#include "gemian_install_config.h"

TEST_CASE("Test Keyboard Layout", "[gemian_install_config]") {
  std::string emptyString;
  REQUIRE(index_for_keyboard_layout(emptyString) == 9);

  std::string dvorakUSLayout("dvorak-us");
  REQUIRE(index_for_keyboard_layout(dvorakUSLayout) == 11);

  std::string usLayout("us");
  const char* nameOfUSLayout = name_for_keyboard_layout(usLayout);
  std::string usLayoutName("English (US)");
  REQUIRE(nameOfUSLayout == usLayoutName);

  std::string swissGermanLayout("ch-de");
  const char* nameOfSwissGermanLayout = name_for_keyboard_layout(swissGermanLayout);
  std::string swissGermanLayoutName("Swiss German");
  REQUIRE(nameOfSwissGermanLayout == swissGermanLayoutName);
}

TEST_CASE("Test Geo Index", "[gemian_install_config]") {
  std::string tz("Europe/London");
  REQUIRE(index_for_tz_area(tz) == 8);
  REQUIRE(index_for_tz_city_region(tz) == 27);
}