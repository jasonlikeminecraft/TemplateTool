#pragma once
#include <mcnbt/mcnbt.hpp>
#include "BCFCachedWriter.hpp"
#include <iostream>  
#include <sstream>  
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <regex>

class McstructureToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;
public:

    McstructureToBCF(const std::string& filename, const std::string& outputFilename)
        : m_filename(filename)
        , m_outputFilename(outputFilename)
    {
    }
    void convert() {
        nbt::Tag structure = nbt::Tag::fromFile(m_filename, false);
        BCFCachedWriter writer(m_outputFilename);
        int sizeX = structure["size"][0].getInt();
        int sizeY = structure["size"][1].getInt();
        int sizeZ = structure["size"][2].getInt();
        int totalBlocks = sizeX * sizeY * sizeZ;
        int yzSize = sizeY * sizeZ;

        nbt::Tag& structureData = structure["structure"];
        nbt::Tag& blockPalette = structureData["palette"]["default"]["block_palette"];
        nbt::Tag& blockIndicesTag = structureData["block_indices"][0];
        std::vector<int> blockIndices(totalBlocks);
        for (int i = 0; i < totalBlocks; ++i) {
            blockIndices[i] = blockIndicesTag[i].getInt();
        }

        std::vector<bool> isAirPalette(blockPalette.size(), false);
        for (size_t i = 0; i < blockPalette.size(); ++i) {
            std::string name = blockPalette[i]["name"].getString();
            if (name.find("air") != std::string::npos) {
                isAirPalette[i] = true;
            }
        }



        for (int x = 0; x < sizeX; ++x) {
            for (int y = 0; y < sizeY; ++y) {
                for (int z = 0; z < sizeZ; ++z) {
                    int index = z + y * sizeZ + x * yzSize;

                    int paletteIndex = blockIndices[index];
                    if (paletteIndex < 0 || paletteIndex >= static_cast<int>(blockPalette.size()) || isAirPalette[paletteIndex]) {
                        continue;
                    }

                    nbt::Tag& block = blockPalette[paletteIndex];
                    std::string blockName = block["name"].getString();
                    if (block.hasTag("states") && !block["states"].isEmpty()) {
                        nbt::Tag& states = block["states"];
                        bool first = true;
                        std::pair<std::string, std::string> state;
                        std::vector<std::pair<std::string, std::string>> statesVec;
                        for (size_t i = 0; i < states.size(); ++i) {
                            first = false;

                            std::string stateName = states[i].name();
                            std::string stateValue;

                            switch (states[i].type()) {
                            case nbt::TT_BYTE:
                                stateValue = states[i].getByte() ? "true" : "false";
                                break;
                            case nbt::TT_INT:
                                stateValue = std::to_string(states[i].getInt());
                                break;
                            case nbt::TT_STRING:
                                stateValue = "\"" + states[i].getString() + "\"";
                                break;
                            default:
                                stateValue = states[i].getString();
                                break;
                            }
                            state.first = stateName;
                            state.second = stateValue;
                            statesVec.push_back(state);
                        }
                        writer.addBlock(x, y, z, blockName, statesVec);
                    }
                    writer.addBlock(x, y, z, blockName, {});
                }
            }
        }
        writer.finalize();
    }

};
static std::vector<std::string> generateSetblockCommands(const std::string& filename) {

    std::vector<std::string> commands;

    nbt::Tag structure = nbt::Tag::fromFile(filename, false);


    int sizeX = structure["size"][0].getInt();
    int sizeY = structure["size"][1].getInt();
    int sizeZ = structure["size"][2].getInt();
    int totalBlocks = sizeX * sizeY * sizeZ;
    int yzSize = sizeY * sizeZ;


    commands.reserve(totalBlocks);


    nbt::Tag& structureData = structure["structure"];
    nbt::Tag& blockPalette = structureData["palette"]["default"]["block_palette"];
    nbt::Tag& blockIndicesTag = structureData["block_indices"][0];


    std::vector<int> blockIndices(totalBlocks);
    for (int i = 0; i < totalBlocks; ++i) {
        blockIndices[i] = blockIndicesTag[i].getInt();
    }


    std::vector<bool> isAirPalette(blockPalette.size(), false);
    for (size_t i = 0; i < blockPalette.size(); ++i) {
        std::string name = blockPalette[i]["name"].getString();
        if (name.find("air") != std::string::npos) {
            isAirPalette[i] = true;
        }
    }


    for (int x = 0; x < sizeX; ++x) {
        for (int y = 0; y < sizeY; ++y) {
            for (int z = 0; z < sizeZ; ++z) {
                int index = z + y * sizeZ + x * yzSize;

                int paletteIndex = blockIndices[index];
                if (paletteIndex < 0 || paletteIndex >= static_cast<int>(blockPalette.size()) || isAirPalette[paletteIndex]) {
                    continue;
                }

                nbt::Tag& block = blockPalette[paletteIndex];
                std::string blockName = block["name"].getString();


                std::string cmd = "setblock ~" + std::to_string(x) + " ~" + std::to_string(y) + " ~" + std::to_string(z) + " " + blockName;


                if (block.hasTag("states") && !block["states"].isEmpty()) {
                    nbt::Tag& states = block["states"];
                    cmd += "[";
                    bool first = true;

                    for (size_t i = 0; i < states.size(); ++i) {
                        if (!first) cmd += ",";
                        first = false;

                        std::string stateName = states[i].name();
                        std::string stateValue;

                        switch (states[i].type()) {
                        case nbt::TT_BYTE:
                            stateValue = states[i].getByte() ? "true" : "false";
                            break;
                        case nbt::TT_INT:
                            stateValue = std::to_string(states[i].getInt());
                            break;
                        case nbt::TT_STRING:
                            stateValue = "\"" + states[i].getString() + "\"";
                            break;
                        default:
                            stateValue = states[i].getString();
                            break;
                        }

                        cmd += "\"" + stateName + "\"=" + stateValue;
                    }

                    cmd += "]";
                }


                commands.push_back(std::move(cmd));
            }
        }
    }

    return commands;
}


