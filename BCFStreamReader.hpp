#pragma once    
#include <fstream>    
#include <vector>    
#include <unordered_map>  // 优化 2: 改用 unordered_map  
#include "bcf_structs.hpp"    
#include "bcf_io.hpp"    
#include "SubChunkUtils.hpp"    
    
class BCFStreamReader {    
private:    
    std::string filename;    
    BCFHeader header;    
    std::vector<FilePos> subChunkOffsets;    
    std::vector<PaletteKey> paletteList;    
      
    // 优化 2: 使用 unordered_map 替代 map (O(1) 查找)  
    std::unordered_map<BlockTypeID, std::string> typeMap;    
    std::unordered_map<BlockStateID, std::string> stateMap;    
      
    // 优化 1: 缓存文件流,避免重复打开  
    mutable std::ifstream cachedStream;  
    
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
        typeMap.reserve(typeCount);  // 预分配容量  
        for (uint32_t i = 0; i < typeCount; i++) {    
            BlockTypeID id = read_u16(ifs);    
            std::string name = readString16(ifs);    
            typeMap[id] = std::move(name);  // 使用 move 避免拷贝  
        }    
    
        // 读取状态名映射    
        ifs.seekg(header.stateNameMapOffset, std::ios::beg);    
        uint32_t stateCount = read_u32(ifs);    
        stateMap.reserve(stateCount);  // 预分配容量  
        for (uint32_t i = 0; i < stateCount; i++) {    
            BlockStateID id = read_u8(ifs);    
            std::string name = readString16(ifs);    
            stateMap[id] = std::move(name);  // 使用 move 避免拷贝  
        }    
          
        // 构造完成后关闭临时文件流  
        ifs.close();  
    }    
    
    // 流式读取指定子区块    
    std::vector<BlockInfo> getBlocks(size_t subChunkIndex) {    
        std::vector<BlockInfo> result;    
        if (subChunkIndex >= subChunkOffsets.size()) return result;    
    
        // 优化 1: 复用文件流,避免重复打开  
        if (!cachedStream.is_open()) {    
            cachedStream.open(filename, std::ios::binary);    
            if (!cachedStream) {  
                throw std::runtime_error("Failed to reopen file for streaming");  
            }  
        }    
          
        // 跳转到目标子区块  
        cachedStream.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);    
    
        // 读取子区块    
        SubChunkSize sz;    
        Coord oy;    
        auto groups = SubChunkUtils::readSubChunk(cachedStream, sz, oy);    
    
        // 优化 3: 预分配 result 容量  
        size_t totalBlocks = 0;  
        for (const auto& bg : groups) {  
            totalBlocks += bg.count;  
        }  
        result.reserve(totalBlocks);  
    
        // 转换为 BlockInfo    
        for (auto& bg : groups) {    
            const PaletteKey& pk = paletteList[bg.paletteId];    
              
            // 优化 2: unordered_map 的 O(1) 查找  
            const std::string& typeStr = typeMap.at(pk.typeId);    
              
            for (size_t i = 0; i < bg.count; i++) {    
                BlockInfo bi;    
                bi.x = bg.x[i]; bi.y = bg.y[i]; bi.z = bg.z[i];    
                bi.type = typeStr;    
                  
                // 预分配状态 vector 容量  
                bi.states.reserve(pk.states.size());  
                  
                for (auto& s : pk.states) {    
                    // 优化 2: unordered_map 的 O(1) 查找  
                    const std::string& stateName = stateMap.at(s.first);    
                    bi.states.push_back({ stateName, std::to_string(s.second) });    
                }    
                result.push_back(std::move(bi));  // 使用 move 避免拷贝  
            }    
        }    
        return result;    
    }    
    
    size_t getSubChunkCount() const { return subChunkOffsets.size(); }    
      
    // 析构函数确保文件流关闭  
    ~BCFStreamReader() {  
        if (cachedStream.is_open()) {  
            cachedStream.close();  
        }  
    }  
};
