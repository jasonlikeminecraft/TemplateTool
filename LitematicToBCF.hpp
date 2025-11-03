#pragma once  
#include "BCFCachedWriter.hpp"  
#include <nbt_tags.h>  
#include <io/stream_reader.h>  
#include <io/izlibstream.h>  
#include <cmath>  
#include <vector>  
#include <string>  
  
class LitematicToBCF {  
private:  
    const std::string m_filename;  
    const std::string m_outputFilename;  
  
public:  
    LitematicToBCF(const std::string& filename, const std::string& outputFilename)  
        : m_filename(filename), m_outputFilename(outputFilename) {  
        convert();  
    }  
  
    void convert() {  
        // 1. 读取 GZIP 压缩的 NBT  
        nbt::io::izlibstream gzFile(m_filename);  
        auto [rootName, litematic] = nbt::io::read_compound(gzFile);  
          
        // 2. 创建 BCFCachedWriter,使用合理的内存阈值  
        BCFCachedWriter writer(m_outputFilename, "./temp_bcf_cache", 50000);  
          
        // 3. 访问 Regions compound  
        auto& regions = litematic->at("Regions").as<nbt::tag_compound>();  
          
        // 4. 遍历每个区域  
        for (const auto& [regionName, regionTag] : regions) {  
            processRegion(regionTag.as<nbt::tag_compound>(), writer);  
        }  
          
        // 5. 完成写入  
        writer.finalize();  
    }  
  
private:  
    void processRegion(const nbt::tag_compound& region, BCFCachedWriter& writer) {  
        // 读取区域尺寸  
        auto& size = region.at("Size").as<nbt::tag_compound>();  
        int sizeX = static_cast<int32_t>(size.at("x"));  
        int sizeY = static_cast<int32_t>(size.at("y"));  
        int sizeZ = static_cast<int32_t>(size.at("z"));  
          
        // 读取区域位置  
        auto& position = region.at("Position").as<nbt::tag_compound>();  
        int posX = static_cast<int32_t>(position.at("x"));  
        int posY = static_cast<int32_t>(position.at("y"));  
        int posZ = static_cast<int32_t>(position.at("z"));  
          
        // 读取调色板  
        auto& palette = region.at("BlockStatePalette").as<nbt::tag_list>();  
        int paletteSize = static_cast<int>(palette.size());  
          
        // 优化 1: 预先标记空气方块,避免重复字符串查找  
        std::vector<bool> isAirPalette = buildAirFilter(palette);  
          
        // 读取位压缩的方块索引  
        auto& blockStates = region.at("BlockStates").as<nbt::tag_long_array>();  
          
        // 计算位宽  
        int bitsPerBlock = paletteSize > 1 ?   
            static_cast<int>(std::ceil(std::log2(paletteSize))) : 1;  
          
        // 优化 2: 解码所有方块索引(批量处理,CPU 缓存友好)  
        int totalBlocks = sizeX * sizeY * sizeZ;  
        std::vector<int> blockIndices = decodeBlockStates(  
            blockStates, totalBlocks, bitsPerBlock  
        );  
          
        // 优化 3: 按 (y, z, x) 顺序遍历,提高空间局部性  
        for (int y = 0; y < sizeY; y++) {  
            for (int z = 0; z < sizeZ; z++) {  
                for (int x = 0; x < sizeX; x++) {  
                    // Litematic 索引顺序: x + (z * sizeX) + (y * sizeX * sizeZ)  
                    int index = x + (z * sizeX) + (y * sizeX * sizeZ);  
                    int paletteIndex = blockIndices[index];  
                      
                    // 边界检查和空气过滤  
                    if (paletteIndex < 0 || paletteIndex >= paletteSize   
                        || isAirPalette[paletteIndex]) {  
                        continue;  
                    }  
                      
                    // 获取方块定义  
                    auto& block = palette.at(paletteIndex).as<nbt::tag_compound>();  
                    std::string blockName = static_cast<std::string>(block.at("Name"));  
                      
                    // 优化 4: 处理方块状态(使用 move 语义)  
                    std::vector<std::pair<std::string, std::string>> statesVec;  
                    if (block.has_key("Properties")) {  
                        statesVec = extractBlockStates(  
                            block.at("Properties").as<nbt::tag_compound>()  
                        );  
                    }  
                      
                    // 添加方块(使用全局坐标)  
                    writer.addBlock(posX + x, posY + y, posZ + z,   
                                  blockName, statesVec);  
                }  
            }  
        }  
    }  
      
    // 优化 1: 预先构建空气方块过滤器  
    std::vector<bool> buildAirFilter(const nbt::tag_list& palette) {  
        std::vector<bool> isAir(palette.size(), false);  
          
        for (size_t i = 0; i < palette.size(); i++) {  
            auto& block = palette.at(i).as<nbt::tag_compound>();  
            std::string name = static_cast<std::string>(block.at("Name"));  
              
            // 检查是否为空气方块  
            if (name.find("air") != std::string::npos) {  
                isAir[i] = true;  
            }  
        }  
          
        return isAir;  
    }  
      
    // 优化 2: 批量解码方块索引  
    std::vector<int> decodeBlockStates(  
        const nbt::tag_long_array& blockStates,  
        int totalBlocks,  
        int bitsPerBlock  
    ) {  
        std::vector<int> indices;  
        indices.reserve(totalBlocks);  // 预分配内存  
          
        int blocksPerLong = 64 / bitsPerBlock;  
        uint64_t mask = (1ULL << bitsPerBlock) - 1;  
          
        for (int i = 0; i < totalBlocks; i++) {  
            int longIndex = i / blocksPerLong;  
            int bitOffset = (i % blocksPerLong) * bitsPerBlock;  
              
            // 从 long 数组中提取索引  
            uint64_t longValue = static_cast<uint64_t>(blockStates[longIndex]);  
            int paletteIndex = static_cast<int>((longValue >> bitOffset) & mask);  
              
            indices.push_back(paletteIndex);  
        }  
          
        return indices;  
    }  
      
    // 优化 4: 提取方块状态(使用 move 语义)  
    std::vector<std::pair<std::string, std::string>> extractBlockStates(  
        const nbt::tag_compound& properties  
    ) {  
        std::vector<std::pair<std::string, std::string>> states;  
        states.reserve(properties.size());  // 预分配内存  
          
        for (const auto& [propName, propValue] : properties) {  
            std::string valueStr = static_cast<std::string>(propValue);  
            states.emplace_back(propName, std::move(valueStr));  
        }  
          
        return states;  
    }  
};
