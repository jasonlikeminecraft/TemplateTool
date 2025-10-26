#pragma once
#include <fstream>
#include <vector>
#include <map>
#include "bcf_structs.hpp"
#include "bcf_io.hpp"
#include "SubChunkUtils.hpp"
class BCFReader {
public:
    BCFReader(const std::string& filename,
        const std::map<BlockTypeID, std::string>& typeMap,
        const std::map<BlockStateID, std::string>& stateMap)
        : typeMap(typeMap), stateMap(stateMap)
    {
        std::ifstream ifs(filename, std::ios::binary);
        if (!ifs) throw std::runtime_error("Failed to open file");

        // 读取文件头
        read_le<BCFHeader>(ifs, header);

        // 读取子区块
        ifs.seekg(sizeof(BCFHeader), std::ios::beg); // 假设 header 后是子区块
        for (size_t i = 0; i < header.subChunkCount; i++) {
            SubChunkSize sz; Coord oy;
            subChunks.push_back(SubChunkUtils::readSubChunk(ifs, sz, oy));
        }

        // 读取 Palette 映射表
        ifs.seekg(header.paletteOffset, std::ios::beg);
        uint32_t paletteCount = read_u32(ifs);
        paletteList.reserve(paletteCount);
        for (uint32_t i = 0; i < paletteCount; i++) {
            uint32_t pid = read_u32(ifs);
            BlockTypeID typeId = read_u16(ifs);
            uint16_t stateCount = read_u16(ifs);

            PaletteKey pk; pk.typeId = typeId;
            for (uint16_t j = 0; j < stateCount; j++) {
                BlockStateID sid = read_u8(ifs);
                BlockStateID val = read_u8(ifs);
                pk.states.push_back({ sid,val });
            }
            paletteList.push_back(pk);
        }
    }

    // 获取指定子区块的所有方块信息（字符串化）
    std::vector<BlockInfo> getBlocks(size_t subChunkIndex) {
        std::vector<BlockInfo> result;
        if (subChunkIndex >= subChunks.size()) return result;

        const auto& groups = subChunks[subChunkIndex];
        for (auto& bg : groups) {
            const PaletteKey& pk = paletteList[bg.paletteId];
            std::string typeStr = typeMap.at(pk.typeId);
            for (size_t i = 0; i < bg.count; i++) {
                BlockInfo bi;
                bi.x = bg.x[i]; bi.y = bg.y[i]; bi.z = bg.z[i];
                bi.type = typeStr;
                for (auto& s : pk.states) {
                    std::string stateName = stateMap.at(s.first);
                    bi.states.push_back({ stateName,s.second });
                }
                result.push_back(bi);
            }
        }
        return result;
    }

    size_t getSubChunkCount() const { return subChunks.size(); }

private:
    BCFHeader header;
    std::vector<std::vector<BlockGroup>> subChunks;
    std::vector<PaletteKey> paletteList;
    const std::map<BlockTypeID, std::string>& typeMap;
    const std::map<BlockStateID, std::string>& stateMap;
};
