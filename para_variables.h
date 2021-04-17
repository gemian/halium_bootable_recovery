/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <map>
#include <fstream>

typedef enum {
    ParaVarErrorNone,
    ParaVarErrorReadBlock,
    ParaVarErrorBadSig,
    ParaVarErrorBadCheckSum
} ParaVarErrors;

static const int ParaEnvBlockRWStart = 0x20000;
static const int ParaEnvBlockSize = 0x4000;
static char ParaEnvNullChar = '\0';
static const int ParaEnvBlockSigSize = 8;
static const int ParaEnvBlockSig2Pos = ParaEnvBlockSize - ParaEnvBlockSigSize - 4;
static const int ParaEnvBlockSig1Pos = 0;

class ParaVariables {
public:
    ParaVariables();

    ParaVarErrors ReadFromStream(std::ifstream &stream);

    std::string& operator[](const std::string& key);


    void Clear();

    void WriteToStream(std::ofstream &stream);

private:
    std::map<std::string, std::string> variables;
};
