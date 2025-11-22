#pragma once    
#include "BCFCachedWriter.hpp"  
#include "BlockStateConverter.hpp"    
#include <nbt_tags.h>    
#include <io/stream_reader.h>    
#include <io/izlibstream.h>    
#include <iostream>    
#include <fstream>    
#include <vector>    
#include <string>    
#include <unordered_map>    

class SchemToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;
    BlockStateConverter m_converter;  // 转换器成员  

public:
    // 构造函数:支持可选的转换表文件  
    SchemToBCF(const std::string& filename, const std::string& outputFilename,
        const std::string& conversionTableFile = "D:\\Projects\\TemplateTool\\snbt_convert.txt")
        : m_filename(filename), m_outputFilename(outputFilename) {

        // 如果提供了转换表文件,加载它  
        if (!conversionTableFile.empty()) {
            if (!m_converter.loadFromFile(conversionTableFile)) {
                //std::cout << "警告: 转换表加载失败,将使用原始方块数据" << std::endl;

            }
        }

        convert();
    }

    void convert() {
        std::ifstream file(m_filename, std::ios::binary);
        if (!file) {
            std::cerr << "无法打开文件" << std::endl;
            return;
        }

        BCFCachedWriter writer(m_outputFilename, "./temp_bcf_cache", 50000);

        try {
            // 读取并解压缩 GZIP 文件    
            zlib::izlibstream gzstream(file);
            auto [name, schematic] = nbt::io::read_compound(gzstream);

            // 获取尺寸    
            int16_t width = static_cast<int16_t>(schematic->at("Width"));
            int16_t height = static_cast<int16_t>(schematic->at("Height"));
            int16_t length = static_cast<int16_t>(schematic->at("Length"));

            std::cout << "尺寸: " << width << "x" << height << "x" << length << std::endl;

            // 读取调色板 (Palette 是 tag_compound)    
            auto& palette = schematic->at("Palette").as<nbt::tag_compound>();

            // 构建反向映射: 调色板ID -> 方块信息    
            std::vector<std::pair<std::string, nbt::tag_compound*>> paletteMap;
            int maxPaletteId = -1;

            for (const auto& [blockName, idTag] : palette) {
                int paletteId = static_cast<int32_t>(idTag);
                if (paletteId > maxPaletteId) {
                    maxPaletteId = paletteId;
                }
            }

            paletteMap.resize(maxPaletteId + 1);

            for (const auto& [blockName, idTag] : palette) {
                int paletteId = static_cast<int32_t>(idTag);
                paletteMap[paletteId] = { blockName, nullptr };
            }

            // 读取 BlockData (变长整数编码的调色板索引)    
            auto& blockData = schematic->at("BlockData").as<nbt::tag_byte_array>();
            std::cout << "方块数据大小: " << blockData.size() << " 字节" << std::endl;

            // 预构建空气方块过滤器    
            std::vector<bool> isAirPalette(paletteMap.size(), false);
            for (size_t i = 0; i < paletteMap.size(); i++) {
                if (paletteMap[i].first.find("air") != std::string::npos) {
                    isAirPalette[i] = true;
                }
            }

            // 解码变长整数    
            std::vector<int> blockIndices = decodeVarIntArray(blockData, width * height * length);

            // 遍历所有方块 (YZX 顺序)    
            for (int y = 0; y < height; ++y) {
                for (int z = 0; z < length; ++z) {
                    for (int x = 0; x < width; ++x) {
                        int index = x + (z * width) + (y * width * length);
                        int paletteId = blockIndices[index];

                        // 过滤空气方块    
                        if (paletteId < 0 || paletteId >= (int)paletteMap.size()
                            || isAirPalette[paletteId]) {
                            continue;
                        }

                        // 解析方块名称和状态    
                        auto [blockFullName, _] = paletteMap[paletteId];
                        auto [blockName, states] = parseBlockNameAndStates(blockFullName);

                        // 应用 Java 到 Bedrock 转换  
                        auto [beBlockName, beStates] = m_converter.convert(blockName, states);

                        // 添加方块    
                        writer.addBlock(x, y, z, beBlockName, beStates);
                    }
                }
            }

            std::cout << "转换完成!" << std::endl;
            writer.finalize();
            std::cout << "写入文件完成!" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "转换错误: " << e.what() << std::endl;
            return;
        }
    }

private:
    // 解码变长整数数组    
    std::vector<int> decodeVarIntArray(const nbt::tag_byte_array& data, int totalBlocks) {
        std::vector<int> result;
        result.reserve(totalBlocks);

        size_t byteIndex = 0;
        while (result.size() < (size_t)totalBlocks && byteIndex < data.size()) {
            int value = 0;
            int shift = 0;
            uint8_t byte;

            // 读取变长整数    
            do {
                if (byteIndex >= data.size()) break;
                byte = static_cast<uint8_t>(data[byteIndex++]);
                value |= (byte & 0x7F) << shift;
                shift += 7;
            } while (byte & 0x80);

            result.push_back(value);
        }

        return result;
    }

    // 解析方块名称和状态    
    // 例如: "minecraft:stone[variant=granite,smooth=true]"     
    // -> ("minecraft:stone", [("variant","granite"), ("smooth","true")])    
    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>
        parseBlockNameAndStates(const std::string& fullName) {
        size_t bracketPos = fullName.find('[');

        if (bracketPos == std::string::npos) {
            // 没有状态    
            return { fullName, {} };
        }

        std::string blockName = fullName.substr(0, bracketPos);
        std::vector<std::pair<std::string, std::string>> states;

        // 解析状态: "key1=value1,key2=value2"    
        size_t start = bracketPos + 1;
        size_t end = fullName.find(']', start);

        if (end == std::string::npos) {
            return { blockName, {} };
        }

        std::string statesStr = fullName.substr(start, end - start);

        // 分割逗号    
        size_t pos = 0;
        while (pos < statesStr.size()) {
            size_t commaPos = statesStr.find(',', pos);
            if (commaPos == std::string::npos) {
                commaPos = statesStr.size();
            }

            std::string pair = statesStr.substr(pos, commaPos - pos);
            size_t equalPos = pair.find('=');

            if (equalPos != std::string::npos) {
                std::string key = pair.substr(0, equalPos);
                std::string value = pair.substr(equalPos + 1);
                if (key != "waterlogged")
                {
                    states.emplace_back(key, value);
                }
                
            }

            pos = commaPos + 1;
        }

        return { blockName, states };
    }
};