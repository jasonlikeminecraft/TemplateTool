#pragma once  
#include <fstream>  
#include <vector>  
#include <map>  
#include "bcf_structs.hpp"  
#include "bcf_io.hpp"  
#include "SubChunkUtils.hpp"  
  
class BCFStreamReader {  
private:  
    std::string filename;  
    BCFHeader header;  
    std::vector<FilePos> subChunkOffsets;  
    std::vector<PaletteKey> paletteList;  
    std::map<BlockTypeID, std::string> typeMap;  
    std::map<BlockStateID, std::string> stateMap;  
  
public:  
    BCFStreamReader(const std::string& filename) : filename(filename) {  
        std::ifstream ifs(filename, std::ios::binary);  
        if (!ifs) throw std::runtime_error("Failed to open file");  
  
        // 读取文件头  
        read_le<BCFHeader>(ifs, header);  
  
        // 检查版本  
        if (header.version < 2) {  
            throw std::runtime_error("File version does not support streaming (version < 2)");  
        }  
  
        // 读取子区块偏移量表  
        ifs.seekg(header.subChunkOffsetsTableOffset, std::ios::beg);  
        uint64_t offsetCount = read_u64(ifs);  
        subChunkOffsets.reserve(offsetCount);  
        for (uint64_t i = 0; i < offsetCount; i++) {  
            subChunkOffsets.push_back(read_u64(ifs));  
        }  
  
        // 读取 Palette 映射  
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
  
        // 读取类型名映射  
        ifs.seekg(header.blockTypeMapOffset, std::ios::beg);  
        uint32_t typeCount = read_u32(ifs);  
        for (uint32_t i = 0; i < typeCount; i++) {  
            BlockTypeID id = read_u16(ifs);  
            std::string name = readString16(ifs);  
            typeMap[id] = name;  
        }  
  
        // 读取状态名映射  
        ifs.seekg(header.stateNameMapOffset, std::ios::beg);  
        uint32_t stateCount = read_u32(ifs);  
        for (uint32_t i = 0; i < stateCount; i++) {  
            BlockStateID id = read_u8(ifs);  
            std::string name = readString16(ifs);  
            stateMap[id] = name;  
        }  
    }  
  
    // 流式读取指定子区块  
    std::vector<BlockInfo> getBlocks(size_t subChunkIndex) {  
        std::vector<BlockInfo> result;  
        if (subChunkIndex >= subChunkOffsets.size()) return result;  
  
        // 打开文件并跳转到目标子区块  
        std::ifstream ifs(filename, std::ios::binary);  
        ifs.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);  
  
        // 读取子区块  
        SubChunkSize sz;  
        Coord oy;  
        auto groups = SubChunkUtils::readSubChunk(ifs, sz, oy);  
  
        // 转换为 BlockInfo  
        for (auto& bg : groups) {  
            const PaletteKey& pk = paletteList[bg.paletteId];  
            std::string typeStr = typeMap.at(pk.typeId);  
            for (size_t i = 0; i < bg.count; i++) {  
                BlockInfo bi;  
                bi.x = bg.x[i]; bi.y = bg.y[i]; bi.z = bg.z[i];  
                bi.type = typeStr;  
                for (auto& s : pk.states) {  
                    std::string stateName = stateMap.at(s.first);  
                    bi.states.push_back({ stateName, std::to_string(s.second) });  
                }  
                result.push_back(bi);  
            }  
        }  
        return result;  
    }  
  
    size_t getSubChunkCount() const { return subChunkOffsets.size(); }  
};
