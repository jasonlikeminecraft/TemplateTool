#pragma once
#include "BCFCachedWriter.hpp"
#include <iostream>  
#include <sstream>  
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <regex>
#include <nbt_tags.h>

class McstructureToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;
public:

    McstructureToBCF(const std::string& filename, const std::string& outputFilename)
        : m_filename(filename)
        , m_outputFilename(outputFilename)
    {
        convert();
    }
    void convert() {
        // 打开文件并读取 NBT 数据(无压缩)  
        std::ifstream file(m_filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("无法打开文件");
        }
        auto [rootName, structure] = nbt::io::read_compound(file,endian::little);

        BCFCachedWriter writer(m_outputFilename);

        // 读取尺寸信息  
        auto& sizeList = structure->at("size").as<nbt::tag_list>();
        int sizeX = static_cast<int32_t>(sizeList.at(0));
        int sizeY = static_cast<int32_t>(sizeList.at(1));
        int sizeZ = static_cast<int32_t>(sizeList.at(2));
        int totalBlocks = sizeX * sizeY * sizeZ;
        int yzSize = sizeY * sizeZ;

        // 访问结构数据  
        auto& structureData = structure->at("structure").as<nbt::tag_compound>();
        auto& blockPalette = structureData.at("palette")
            .as<nbt::tag_compound>()
            .at("default")
            .as<nbt::tag_compound>()
            .at("block_palette")
            .as<nbt::tag_list>();

        auto& blockIndicesList = structureData.at("block_indices")
            .as<nbt::tag_list>()
            .at(0)
            .as<nbt::tag_list>();
        // 复制方块索引  
        std::vector<int> blockIndices(totalBlocks);
        for (int i = 0; i < totalBlocks; ++i) {
            blockIndices[i] = blockIndicesList[i].as<nbt::tag_int>();
        }

        // 标记空气方块  
        std::vector<bool> isAirPalette(blockPalette.size(), false);
        for (size_t i = 0; i < blockPalette.size(); ++i) {
            auto& paletteEntry = blockPalette.at(i).as<nbt::tag_compound>();
            std::string name = static_cast<std::string>(paletteEntry.at("name"));
            if (name.find("air") != std::string::npos) {
                isAirPalette[i] = true;
            }
        }

        // 遍历所有方块  
        for (int x = 0; x < sizeX; ++x) {
            for (int y = 0; y < sizeY; ++y) {
                for (int z = 0; z < sizeZ; ++z) {
                    int index = z + y * sizeZ + x * yzSize;

                    int paletteIndex = blockIndices[index];
                    if (paletteIndex < 0 || paletteIndex >= static_cast<int>(blockPalette.size())
                        || isAirPalette[paletteIndex]) {
                        continue;
                    }

                    auto& block = blockPalette.at(paletteIndex).as<nbt::tag_compound>();
                    std::string blockName = static_cast<std::string>(block.at("name"));

                    // 处理方块状态  
                    if (block.has_key("states")) {
                        auto& states = block.at("states").as<nbt::tag_compound>();
                        std::vector<std::pair<std::string, std::string>> statesVec;

                        for (const auto& [stateName, stateValue] : states) {
                            std::string valueStr;

                            switch (stateValue.get_type()) {
                            case nbt::tag_type::Byte:
                                valueStr = static_cast<int8_t>(stateValue) ? "true" : "false";
                                break;
                            case nbt::tag_type::Int:
                                valueStr = std::to_string(static_cast<int32_t>(stateValue));
                                break;
                            case nbt::tag_type::String:
                                valueStr = static_cast<std::string>(stateValue);
                                break;
                            default:
                                valueStr = static_cast<std::string>(stateValue);
                                break;
                            }

                            statesVec.push_back({ stateName, valueStr });
                        }
                        writer.addBlock(x, y, z, blockName, statesVec);
                    }
                    else {
                        writer.addBlock(x, y, z, blockName, {});
                    }
                }
            }
        }
        writer.finalize();
    }
};


