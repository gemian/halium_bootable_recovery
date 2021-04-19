/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdarg>
#include <iostream>
#include <string>
#include <map>
#include "para_variables.h"

TEST_CASE("Read JP as Block", "[keyboard_selection]")
{
    std::string filename = "para-jp.img";
    std::ifstream paraStream(filename, std::ios::binary);
    REQUIRE(paraStream.is_open());

    ParaVariables paraVariables;
    paraVariables.Clear();
    ParaVarErrors err = paraVariables.ReadFromStream(paraStream);
    CHECK(err == ParaVarErrorNone);
    std::string &kbLayout = paraVariables["keyboard_layout"];
    CHECK(kbLayout.compare("jp") == 0);
    paraStream.close();
}

TEST_CASE("Read GB as Block", "[keyboard_selection]")
{
    std::string filename = "para-gb.img";
    std::ifstream paraStream(filename, std::ios::binary);
    REQUIRE(paraStream.is_open());

    ParaVariables paraVariables;
    REQUIRE(paraVariables.ReadFromStream(paraStream) == ParaVarErrorNone);
    REQUIRE(paraVariables["keyboard_layout"].compare("gb") == 0);
    paraStream.close();
}

TEST_CASE("Read Dvorak as Block", "[keyboard_selection]")
{
    std::string filename = "para-dvorak.img";
    std::ifstream paraStream(filename, std::ios::binary);
    REQUIRE(paraStream.is_open());

    ParaVariables paraVariables;
    REQUIRE(paraVariables.ReadFromStream(paraStream) == ParaVarErrorNone);
    REQUIRE(paraVariables["keyboard_layout"].compare("dvorak") == 0);
    paraStream.close();
}

TEST_CASE("Read null as bad block", "[keyboard_selection]")
{
    std::string filename = "/dev/null";
    std::ifstream paraStream(filename, std::ios::binary);
    REQUIRE(paraStream.is_open());

    ParaVariables paraVariables;
    REQUIRE(paraVariables.ReadFromStream(paraStream) == ParaVarErrorReadBlock);
    REQUIRE(paraVariables["keyboard_layout"].compare("gb") != 0);
    paraStream.close();
}

TEST_CASE("Read no sig", "[keyboard_selection]")
{
    std::string filename1 = "para-nosig1.img";
    std::ifstream paraStream1(filename1, std::ios::binary);
    REQUIRE(paraStream1.is_open());

    ParaVariables paraVariables;
    REQUIRE(paraVariables.ReadFromStream(paraStream1) == ParaVarErrorBadSig);
    REQUIRE(paraVariables["keyboard_layout"].compare("gb") != 0);
    paraStream1.close();

    std::string filename2 = "para-nosig2.img";
    std::ifstream paraStream2(filename2, std::ios::binary);
    REQUIRE(paraStream2.is_open());

    paraVariables.Clear();
    REQUIRE(paraVariables.ReadFromStream(paraStream2) == ParaVarErrorBadSig);
    REQUIRE(paraVariables["keyboard_layout"].compare("gb") != 0);
    paraStream2.close();
}

TEST_CASE("Read bad checksum", "[keyboard_selection]")
{
    std::string filename = "para-broken.img";
    std::ifstream paraStream(filename, std::ios::binary);
    REQUIRE(paraStream.is_open());

    ParaVariables paraVariables;
    REQUIRE(paraVariables.ReadFromStream(paraStream) == ParaVarErrorBadCheckSum);
    std::string &string = paraVariables["keyboard_layout"];
    REQUIRE(string.compare("gb") != 0);
    paraStream.close();
}

TEST_CASE("Write GB", "[keyboard_selection]")
{
    ParaVariables paraVariables;
    paraVariables["keyboard_layout"]="gb";
    paraVariables["off-mode-charge"]="1";
    REQUIRE(paraVariables["keyboard_layout"].compare("gb") == 0);

    std::string filenameOut = "para-gb-out.img";
    std::ofstream paraStream(filenameOut, std::ios::binary);
    REQUIRE(paraStream.is_open());

    paraVariables.WriteToStream(paraStream);
    paraStream.close();

    std::string filename = "para-gb.img";
    std::ifstream knownGood(filename, std::ios::binary);
    REQUIRE(knownGood.is_open());
    knownGood.seekg(ParaEnvBlockRWStart);
    std::ifstream justWritten(filenameOut, std::ios::binary);
    REQUIRE(justWritten.is_open());
    justWritten.seekg(ParaEnvBlockRWStart);

    std::string knownGoodBlock(ParaEnvBlockSize, ParaEnvNullChar);
    REQUIRE (knownGood.read(&knownGoodBlock[0], ParaEnvBlockSize));
    std::string justWrittenBlock(ParaEnvBlockSize, ParaEnvNullChar);
    REQUIRE (justWritten.read(&justWrittenBlock[0], ParaEnvBlockSize));

    REQUIRE (knownGoodBlock.compare(justWrittenBlock) == 0);
}
