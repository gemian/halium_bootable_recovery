/*
 * Copyright (c) 2021, Adam Boardman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <iostream>
#include <numeric>
#include "para_variables.h"

static const std::string signature("ENV_v1");

ParaVariables::ParaVariables() = default;

ParaVarErrors ParaVariables::ReadFromStream(std::ifstream &stream) {
    stream.seekg(ParaEnvBlockRWStart);
    std::string para_env_block(ParaEnvBlockSize, ParaEnvNullChar);
    if (!stream.read(&para_env_block[0], ParaEnvBlockSize)) {
        return ParaVarErrorReadBlock;
    }

    if (para_env_block.compare(ParaEnvBlockSig1Pos, signature.length(), signature) != 0) {
        return ParaVarErrorBadSig;
    }

    size_t start;
    size_t end = signature.length();
    while ((start = para_env_block.find_first_not_of(ParaEnvNullChar, end)) != std::string::npos) {
        end = para_env_block.find(ParaEnvNullChar, start);
        std::string variable_block(para_env_block.data() + start, end - start);
        size_t mid = variable_block.find('=');
        if (mid != std::string::npos) {
            std::string variable_name(variable_block.substr(0, mid));
            std::string variable_value(variable_block.substr(mid + 1));
            variables.insert(std::pair<std::string,std::string>(variable_name,variable_value));
        }
    }

    if (para_env_block.compare(ParaEnvBlockSig2Pos, signature.length(), signature) != 0) {
        return ParaVarErrorBadSig;
    }

    uint32_t sum = std::accumulate(&para_env_block[signature.length()], &para_env_block[ParaEnvBlockSig2Pos], 0);
    std::string checksum(reinterpret_cast<char*>(&sum), sizeof sum);
    if (para_env_block.compare(ParaEnvBlockSize-sizeof sum, sizeof sum, checksum) != 0) {
        return ParaVarErrorBadCheckSum;
    }

    return ParaVarErrorNone;
}

std::string &ParaVariables::operator[](const std::string &key) {
    return variables[key];
}

void ParaVariables::Clear() {
    variables.clear();
}

void ParaVariables::WriteToStream(std::ofstream &stream) {
    stream.seekp(ParaEnvBlockRWStart);
    stream << signature;

    stream.seekp(ParaEnvBlockRWStart+ParaEnvBlockSigSize);
    for (auto const& x : variables) {
        stream << x.first << "=" << x.second;
        stream.write(reinterpret_cast<char*>(&ParaEnvNullChar), sizeof(char));
    }
    while (stream.tellp() < ParaEnvBlockRWStart+ParaEnvBlockSigSize) {
        stream.write(reinterpret_cast<char*>(&ParaEnvNullChar), sizeof(char));
    }

    stream.seekp(ParaEnvBlockRWStart+ParaEnvBlockSig2Pos);
    stream << signature;

    uint32_t sum = 0;
    for (const auto& x : variables) {
        sum += std::accumulate(&x.first[0], &x.first[x.first.length()], 0);
        sum += '=';
        sum += std::accumulate(&x.second[0], &x.second[x.second.length()], 0);
    }
    stream.seekp(ParaEnvBlockRWStart+ParaEnvBlockSize-sizeof sum);
    stream.write(reinterpret_cast<char*>(&sum), sizeof sum);
}

