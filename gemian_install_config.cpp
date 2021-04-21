/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gemian_install_config.h"

const char* keyboard_layout_codes[] = { "",   "ara", "at", "be", "cat",       "hr",    "cz",
                                        "dk", "gb",  "us", "ie", "dvorak-us", "fr",    "fi",
                                        "de", "gr",  "hu", "it", "jp",        "no",    "pl",
                                        "pt", "ru",  "sk", "es", "se",        "ch-de", nullptr };

const char* keyboard_layout_items[] = {
  "Back",         "Arabic",       "Austrian",        "Belgian",
  "Catalan",      "Croatian",     "Czech",           "Danish",
  "English (UK)", "English (US)", "English (Eire)",  "English (Dvorak-US)",
  "French",       "Finnish",      "German",          "Greek",
  "Hungarian",    "Italian",      "Japanese (Kana)", "Norwegian",
  "Polish",       "Portuguese",   "Russian",         "Slovak",
  "Spanish",      "Swedish",      "Swiss German",    nullptr
};

const char* time_zone_africa[] = {
  "Africa",     "Abidjan",    "Accra",    "Addis_Ababa", "Algiers",  "Asmara",        "Asmera",
  "Bamako",     "Bangui",     "Banjul",   "Bissau",      "Blantyre", "Brazzaville",   "Bujumbura",
  "Cairo",      "Casablanca", "Ceuta",    "Conakry",     "Dakar",    "Dar_es_Salaam", "Djibouti",
  "Douala",     "El_Aaiun",   "Freetown", "Gaborone",    "Harare",   "Johannesburg",  "Juba",
  "Kampala",    "Khartoum",   "Kigali",   "Kinshasa",    "Lagos",    "Libreville",    "Lome",
  "Luanda",     "Lubumbashi", "Lusaka",   "Malabo",      "Maputo",   "Maseru",        "Mbabane",
  "Mogadishu",  "Monrovia",   "Nairobi",  "Ndjamena",    "Niamey",   "Nouakchott",    "Ouagadougou",
  "Porto-Novo", "Sao_Tome",   "Timbuktu", "Tripoli",     "Tunis",    "Windhoek",      nullptr
};

const char* time_zone_america[] = {
  "America",       "Adak",           "Anchorage",      "Anguilla",      "Antigua",
  "Araguaina",     "Argentina",      "Aruba",          "Asuncion",      "Atikokan",
  "Atka",          "Bahia",          "Bahia_Banderas", "Barbados",      "Belem",
  "Belize",        "Blanc-Sablon",   "Boa_Vista",      "Bogota",        "Boise",
  "Buenos_Aires",  "Cambridge_Bay",  "Campo_Grande",   "Cancun",        "Caracas",
  "Catamarca",     "Cayenne",        "Cayman",         "Chicago",       "Chihuahua",
  "Coral_Harbour", "Cordoba",        "Costa_Rica",     "Creston",       "Cuiaba",
  "Curacao",       "Danmarkshavn",   "Dawson",         "Dawson_Creek",  "Denver",
  "Detroit",       "Dominica",       "Edmonton",       "Eirunepe",      "El_Salvador",
  "Ensenada",      "Fortaleza",      "Fort_Nelson",    "Fort_Wayne",    "Glace_Bay",
  "Godthab",       "Goose_Bay",      "Grand_Turk",     "Grenada",       "Guadeloupe",
  "Guatemala",     "Guayaquil",      "Guyana",         "Halifax",       "Havana",
  "Hermosillo",    "Indiana",        "Indianapolis",   "Inuvik",        "Iqaluit",
  "Jamaica",       "Jujuy",          "Juneau",         "Kentucky",      "Knox_IN",
  "Kralendijk",    "La_Paz",         "Lima",           "Los_Angeles",   "Louisville",
  "Lower_Princes", "Maceio",         "Managua",        "Manaus",        "Marigot",
  "Martinique",    "Matamoros",      "Mazatlan",       "Mendoza",       "Menominee",
  "Merida",        "Metlakatla",     "Mexico_City",    "Miquelon",      "Moncton",
  "Monterrey",     "Montevideo",     "Montreal",       "Montserrat",    "Nassau",
  "New_York",      "Nipigon",        "Nome",           "Noronha",       "North_Dakota",
  "Nuuk",          "Ojinaga",        "Panama",         "Pangnirtung",   "Paramaribo",
  "Phoenix",       "Port-au-Prince", "Porto_Acre",     "Port_of_Spain", "Porto_Velho",
  "Puerto_Rico",   "Punta_Arenas",   "Rainy_River",    "Rankin_Inlet",  "Recife",
  "Regina",        "Resolute",       "Rio_Branco",     "Rosario",       "Santa_Isabel",
  "Santarem",      "Santiago",       "Santo_Domingo",  "Sao_Paulo",     "Scoresbysund",
  "Shiprock",      "Sitka",          "St_Barthelemy",  "St_Johns",      "St_Kitts",
  "St_Lucia",      "St_Thomas",      "St_Vincent",     "Swift_Current", "Tegucigalpa",
  "Thule",         "Thunder_Bay",    "Tijuana",        "Toronto",       "Tortola",
  "Vancouver",     "Virgin",         "Whitehorse",     "Winnipeg",      "Yakutat",
  "Yellowknife",   nullptr
};

const char* time_zone_antarctica[] = { "Antarctica", "Casey",      "Davis",   "DumontDUrville",
                                       "Mawson",     "Macquarie",  "McMurdo", "Palmer",
                                       "Rothera",    "South_Pole", "Syowa",   "Troll",
                                       "Vostok",     nullptr };

const char* time_zone_australia[] = {
  "Australia",  "Adelaide", "Brisbane", "Broken_Hill", "Canberra",  "Currie", "Darwin",     "Eucla",
  "Hobart",     "LHI",      "Lindeman", "Lord_Howe",   "Melbourne", "North",  "NSW",        "Perth",
  "Queensland", "South",    "Sydney",   "Tasmania",    "Victoria",  "West",   "Yancowinna", nullptr
};

const char* time_zone_arctic[] = { "Arctic", "Longyearbyen", nullptr };

const char* time_zone_asia[] = {
  "Asia",        "Aden",          "Almaty",        "Amman",        "Anadyr",      "Aqtau",
  "Aqtobe",      "Ashgabat",      "Ashkhabad",     "Atyrau",       "Baghdad",     "Bahrain",
  "Baku",        "Bangkok",       "Barnaul",       "Beirut",       "Bishkek",     "Brunei",
  "Calcutta",    "Chita",         "Choibalsan",    "Chongqing",    "Chungking",   "Colombo",
  "Dacca",       "Damascus",      "Dhaka",         "Dili",         "Dubai",       "Dushanbe",
  "Famagusta",   "Gaza",          "Harbin",        "Hebron",       "Ho_Chi_Minh", "Hong_Kong",
  "Hovd",        "Irkutsk",       "Istanbul",      "Jakarta",      "Jayapura",    "Jerusalem",
  "Kabul",       "Kamchatka",     "Karachi",       "Kashgar",      "Kathmandu",   "Katmandu",
  "Khandyga",    "Kolkata",       "Krasnoyarsk",   "Kuala_Lumpur", "Kuching",     "Kuwait",
  "Macao",       "Macau",         "Magadan",       "Makassar",     "Manila",      "Muscat",
  "Nicosia",     "Novokuznetsk",  "Novosibirsk",   "Omsk",         "Oral",        "Phnom_Penh",
  "Pontianak",   "Pyongyang",     "Qatar",         "Qostanay",     "Qyzylorda",   "Rangoon",
  "Riyadh",      "Saigon",        "Sakhalin",      "Samarkand",    "Seoul",       "Shanghai",
  "Singapore",   "Srednekolymsk", "Taipei",        "Tashkent",     "Tbilisi",     "Tehran",
  "Tel_Aviv",    "Thimbu",        "Thimphu",       "Tokyo",        "Tomsk",       "Ujung_Pandang",
  "Ulaanbaatar", "Ulan_Bator",    "Urumqi",        "Ust-Nera",     "Vientiane",   "Vladivostok",
  "Yakutsk",     "Yangon",        "Yekaterinburg", "Yerevan",      nullptr
};

const char* time_zone_atlantic[] = { "Atlantic",   "Azores",        "Bermuda",   "Canary",
                                     "Cape_Verde", "Faroe",         "Jan_Mayen", "Madeira",
                                     "Reykjavik",  "South_Georgia", "St_Helena", "Stanley",
                                     nullptr };

const char* time_zone_europe[] = {
  "Europe",     "Amsterdam",   "Andorra",   "Astrakhan", "Athens",     "Belfast",     "Belgrade",
  "Berlin",     "Bratislava",  "Brussels",  "Bucharest", "Budapest",   "Busingen",    "Chisinau",
  "Copenhagen", "Dublin",      "Gibraltar", "Guernsey",  "Helsinki",   "Isle_of_Man", "Istanbul",
  "Jersey",     "Kaliningrad", "Kiev",      "Kirov",     "Lisbon",     "Ljubljana",   "London",
  "Luxembourg", "Madrid",      "Malta",     "Mariehamn", "Minsk",      "Monaco",      "Moscow",
  "Nicosia",    "Oslo",        "Paris",     "Podgorica", "Prague",     "Riga",        "Rome",
  "Samara",     "San_Marino",  "Sarajevo",  "Saratov",   "Simferopol", "Skopje",      "Sofia",
  "Stockholm",  "Tallinn",     "Tirane",    "Tiraspol",  "Ulyanovsk",  "Uzhgorod",    "Vaduz",
  "Vatican",    "Vienna",      "Vilnius",   "Volgograd", "Warsaw",     "Zagreb",      "Zaporozhye",
  "Zurich",     nullptr
};

const char* time_zone_indian[] = { "Indian",  "Antananarivo", "Chagos", "Christmas", "Cocos",
                                   "Comoro",  "Kerguelen",    "Mahe",   "Maldives",  "Mauritius",
                                   "Mayotte", "Reunion",      nullptr };

const char* time_zone_pacific[] = {
  "Pacific",     "Apia",      "Auckland", "Bougainville", "Chatham",   "Chuuk",        "Easter",
  "Efate",       "Enderbury", "Fakaofo",  "Fiji",         "Funafuti",  "Galapagos",    "Gambier",
  "Guadalcanal", "Guam",      "Honolulu", "Johnston",     "Marquesas", "Kiritimati",   "Kosrae",
  "Kwajalein",   "Majuro",    "Midway",   "Nauru",        "Niue",      "Norfolk",      "Noumea",
  "Pago_Pago",   "Palau",     "Pitcairn", "Pohnpei",      "Ponape",    "Port_Moresby", "Rarotonga",
  "Saipan",      "Samoa",     "Tahiti",   "Tarawa",       "Tongatapu", "Truk",         "Wake",
  "Wallis",      "Yap",       nullptr
};

const char* time_zone_systemv[] = { "SystemV", "AST4",    "AST4ADT", "CST6",    "CST6CDT",
                                    "EST5",    "EST5EDT", "HST10",   "MST7",    "MST7MDT",
                                    "PST8",    "PST8PDT", "YST9",    "YST9YDT", nullptr };
const char* time_zone_us[] = { "US",      "Alaska", "Aleutian",       "Arizona",  "Central",
                               "Eastern", "Hawaii", "Indiana-Starke", "Michigan", "Mountain",
                               "Pacific", "Samoa",  nullptr };
const char* time_zone_etc[] = { "Etc",    "GMT",       "GMT+0",  "GMT+1",     "GMT+2",  "GMT+3",
                                "GMT+4",  "GMT+5",     "GMT+6",  "GMT+7",     "GMT+8",  "GMT+9",
                                "GMT+10", "GMT+11",    "GMT+12", "GMT0",      "GMT-0",  "GMT-1",
                                "GMT-2",  "GMT-3",     "GMT-4",  "GMT-5",     "GMT-6",  "GMT-7",
                                "GMT-8",  "GMT-9",     "GMT-10", "GMT-11",    "GMT-12", "GMT-13",
                                "GMT-14", "Greenwich", "UTC",    "Universal", "Zulu",   "UCT",
                                nullptr };

const char* time_zone_back[] = { "Back", nullptr };

const char** time_zone_items[] = { time_zone_back,       time_zone_africa,    time_zone_america,
                                   time_zone_antarctica, time_zone_australia, time_zone_arctic,
                                   time_zone_asia,       time_zone_atlantic,  time_zone_europe,
                                   time_zone_indian,     time_zone_pacific,   time_zone_systemv,
                                   time_zone_us,         time_zone_etc };

int index_for_keyboard_layout(std::string& layout) {
  for (int i = 0; !layout.empty() && keyboard_layout_codes[i] != nullptr; i++) {
    if (layout.compare(keyboard_layout_codes[i]) == 0) {
      return i;
    }
  }
  return 9;  // us
}

const char* name_for_keyboard_layout(std::string& layout) {
  for (int i = 0; !layout.empty() && keyboard_layout_codes[i] != nullptr; i++) {
    if (layout.compare(keyboard_layout_codes[i]) == 0) {
      return keyboard_layout_items[i];
    }
  }
  return keyboard_layout_items[9];  // us
}

int index_for_tz_area(std::string& tz) {
  unsigned long pos = tz.find('/');
  std::string area(tz.substr(0, pos));
  for (int i = 0; i <= time_zone_areas_count; i++) {
    std::string area_name(time_zone_items[i][0]);
    if (area_name == area) {
      return i;
    }
  }
  return 0;
}

int index_for_tz_city_region(std::string& tz) {
  unsigned long pos = tz.find('/');
  std::string city_region(tz.substr(pos + 1));
  int area_index = index_for_tz_area(tz);
  for (int i = 1; time_zone_items[area_index][i] != nullptr; i++) {
    std::string city_region_name(time_zone_items[area_index][i]);
    if (city_region_name == city_region) {
      return i;
    }
  }
  return 0;
}
