/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <string>

const int time_zone_areas_count = 13;

int index_for_keyboard_layout(std::string &layout);

const char *name_for_keyboard_layout(std::string &layout);

int index_for_tz_area(std::string& tz);

int index_for_tz_city_region(std::string &tz);

