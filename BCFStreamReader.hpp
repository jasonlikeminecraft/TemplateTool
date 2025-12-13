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
    std::unordered_map<StateValueID, std::string> stateValueMap;
      
    // 优化 1: 缓存文件流,避免重复打开  
    mutable std::ifstream cachedStream;  
    
public:    BCFStreamReader(const std::string& filename) : filename(filename) {  
    std::ifstream ifs(filename, std::ios::binary);  
    if (!ifs) throw std::runtime_error("Failed to open file");  
  
    // 读取文件头  
    read_le<BCFHeader>(ifs, header);  
  
    // 检查版本 (支持版本 2, 3, 4)  
    if (header.version < 2) {  
        throw std::runtime_error("File version does not support streaming (version < 2)");  
    }  
  
    // 读取子区块偏移量表  
    ifs.seekg(header.subChunkOffsetsTableOffset, std::ios::beg);  
    FilePos subChunkCount = read_u64(ifs);  
    subChunkOffsets.reserve(subChunkCount);  
    for (FilePos i = 0; i < subChunkCount; i++) {  
        subChunkOffsets.push_back(read_u64(ifs));  
    }  
  
    // 读取 Palette 映射并反序列化NBT数据  
    ifs.seekg(header.paletteOffset, std::ios::beg);  
    uint32_t paletteCount = read_u32(ifs);  
    paletteList.reserve(paletteCount);  
    for (uint32_t i = 0; i < paletteCount; i++) {  
        uint32_t pid = read_u32(ifs);  
        BlockTypeID typeId = read_u16(ifs);  
        uint16_t stateCount = read_u16(ifs);  
  
        PaletteKey pk;   
        pk.typeId = typeId;  
        for (uint16_t j = 0; j < stateCount; j++) {  
            BlockStateID sid = read_u8(ifs);  
            BlockStateID val = read_u8(ifs);  
            pk.states.push_back({ sid,val });  
        }  
          
        // 读取并反序列化 NBT 数据（版本 4+）  
        if (header.version >= 4) {
            std::string nbtStr = readString32(ifs);
            if (!nbtStr.empty()) {
                std::istringstream iss(nbtStr);
                try {
                    nbt::io::stream_reader reader(iss,endian::little);

                    // 读取完整 root tag
                    auto root = reader.read_tag();

                    // root 是 pair<string, shared_ptr<tag>>
                    // 我们期望它是 compound
                    if (root.second &&
                        root.second->get_type() == nbt::tag_type::Compound) {
                        // 将 unique_ptr 转为 shared_ptr
                        pk.nbtData = std::shared_ptr<nbt::tag_compound>(
                            static_cast<nbt::tag_compound*>(root.second.release())
                        );
                    }
                    else {
                        // 非 compound，直接丢弃
                        pk.nbtData = nullptr;
                    }
                }
                catch (const std::exception& e) {
                    pk.nbtData = nullptr;
                }
            }
            else {
                pk.nbtData = nullptr;
            }
        }
          
        paletteList.push_back(pk);  
    }  
  
    // 读取类型名映射  
    ifs.seekg(header.blockTypeMapOffset, std::ios::beg);  
    uint32_t typeCount = read_u32(ifs);  
    for (uint32_t i = 0; i < typeCount; i++) {  
        BlockTypeID typeId = read_u16(ifs);  
        std::string typeName = readString16(ifs);  
        typeMap[typeId] = typeName;  
    }  
  
    // 读取状态名映射  
    ifs.seekg(header.stateNameMapOffset, std::ios::beg);  
    uint32_t stateCount = read_u32(ifs);  
    for (uint32_t i = 0; i < stateCount; i++) {  
        BlockStateID stateId = read_u8(ifs);  
        std::string stateName = readString16(ifs);  
        stateMap[stateId] = stateName;  
    }  
  
    // 读取状态值映射  
    ifs.seekg(header.stateValueMapOffset, std::ios::beg);  
    uint32_t valueCount = read_u32(ifs);  
    for (uint32_t i = 0; i < valueCount; i++) {  
        StateValueID valueId = read_u8(ifs);  
        std::string valueName = readString16(ifs);  
        stateValueMap[valueId] = valueName;  
    }  
}

    // 流式读取指定子区块    
    //std::vector<BlockInfo> getBlocks(size_t subChunkIndex) {    
    //    std::vector<BlockInfo> result;    
    //    if (subChunkIndex >= subChunkOffsets.size()) return result;    
    //
    //    // 优化 1: 复用文件流,避免重复打开  
    //    if (!cachedStream.is_open()) {    
    //        cachedStream.open(filename, std::ios::binary);    
    //        if (!cachedStream) {  
    //            throw std::runtime_error("Failed to reopen file for streaming");  
    //        }  
    //    }    
    //      
    //    // 跳转到目标子区块  
    //    cachedStream.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);    
    //
    //    // 读取子区块    
    //    SubChunkSize sz;    
    //    Coord oy;    
    //    auto groups = SubChunkUtils::readSubChunk(cachedStream, sz, oy);    
    //
    //    // 优化 3: 预分配 result 容量  
    //    size_t totalBlocks = 0;  
    //    for (const auto& bg : groups) {  
    //        totalBlocks += bg.count;  
    //    }  
    //    result.reserve(totalBlocks);  
    //
    //    // 转换为 BlockInfo    
    //    for (auto& bg : groups) {    
    //        const PaletteKey& pk = paletteList[bg.paletteId];    
    //          
    //        // 优化 2: unordered_map 的 O(1) 查找  
    //        const std::string& typeStr = typeMap.at(pk.typeId);    
    //          
    //        for (size_t i = 0; i < bg.count; i++) {    
    //            BlockInfo bi;    
    //            bi.x = bg.x[i]; bi.y = bg.y[i]; bi.z = bg.z[i];    
    //            bi.type = typeStr;    
    //              
    //            // 预分配状态 vector 容量  
    //            bi.states.reserve(pk.states.size());  
    //              
    //            for (auto& s : pk.states) {    
    //                // 优化 2: unordered_map 的 O(1) 查找  
    //                const std::string& stateName = stateMap.at(s.first);    
    //                bi.states.push_back({ stateName, std::to_string(s.second) });    
    //            }    
    //            result.push_back(std::move(bi));  // 使用 move 避免拷贝  
    //        }    
    //    }    
    //    return result;    
    //}    
    
    size_t getSubChunkCount() const { return subChunkOffsets.size(); }    
    // 在 BCFStreamReader 中添加:  
std::vector<BlockRegion> getBlockRegions(size_t subChunkIndex) {  
    std::vector<BlockRegion> result;  
    if (subChunkIndex >= subChunkOffsets.size()) return result;  
  
    if (!cachedStream.is_open()) {  
        cachedStream.open(filename, std::ios::binary);  
        if (!cachedStream) {  
            throw std::runtime_error("Failed to reopen file for streaming");  
        }  
    }  
  
    cachedStream.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);  
    SubChunkSize sz;  
    Coord ox, oy, oz;  
    return SubChunkUtils::readSubChunk(cachedStream, sz, ox, oy, oz);  
}
std::vector<BlockRegion> getBlockRegions(size_t subChunkIndex) const {
    std::vector<BlockRegion> result;
    if (subChunkIndex >= subChunkOffsets.size()) return result;

    if (!cachedStream.is_open()) {
        cachedStream.open(filename, std::ios::binary);
        if (!cachedStream) {
            throw std::runtime_error("Failed to reopen file for streaming");
        }
    }

    cachedStream.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);
    SubChunkSize sz;
    Coord ox, oy, oz;
    return SubChunkUtils::readSubChunk(cachedStream, sz, ox, oy, oz);
}

    const PaletteKey& getPaletteKey(PaletteID paletteId) const {
        if (paletteId >= paletteList.size()) {
            throw std::out_of_range("Invalid paletteId");
        }
        return paletteList[paletteId];
    }

    std::string getBlockType(BlockTypeID typeId) const {
        auto it = typeMap.find(typeId);
        if (it == typeMap.end()) {
            return "minecraft:air";
        }
        return it->second;
    }

    std::string getStateName(BlockStateID stateId) const {
        auto it = stateMap.find(stateId);
        if (it == stateMap.end()) {
            return "unknown";
        }
        return it->second;
    }
    std::string getStateValue(StateValueID valueId) const {
        auto it = stateValueMap.find(valueId);
        if (it == stateValueMap.end()) {
            return "0";  // 默认值  
        }
        return it->second;
    }
// 获取指定PaletteID的NBT数据  
std::shared_ptr<nbt::tag_compound> getBlockNBTData(PaletteID paletteId) const {
    if (paletteId >= paletteList.size()) return nullptr;  
    return paletteList[paletteId].nbtData;  
}  
  
// 获取指定方块的完整NBT数据  
std::shared_ptr<nbt::tag_compound> getBlockNBTData(int x, int y, int z, size_t subChunkIndex) const {
    auto regions = getBlockRegions(subChunkIndex);  
    for (const auto& region : regions) {  
        if (x >= region.x1 && x <= region.x2 &&  
            y >= region.y1 && y <= region.y2 &&  
            z >= region.z1 && z <= region.z2) {  
            return getBlockNBTData(region.paletteId);  
        }  
    }  
    return nullptr;  
}

// 获取指定 subchunk 的起始坐标  

  
SubChunkOrigin getSubChunkOrigin(size_t subChunkIndex) {  
    SubChunkOrigin origin;  
    if (subChunkIndex >= subChunkOffsets.size()) {  
        origin.originX = 0;  
        origin.originY = 0;  
        origin.originZ = 0;  
        return origin;  
    }  
  
    if (!cachedStream.is_open()) {  
        cachedStream.open(filename, std::ios::binary);  
        if (!cachedStream) {  
            throw std::runtime_error("Failed to reopen file for streaming");  
        }  
    }  
  
    // 跳转到 subchunk 位置  
    cachedStream.seekg(subChunkOffsets[subChunkIndex], std::ios::beg);  
      
    // 读取 subChunkSize (跳过)  
    read_u64(cachedStream);  
      
    // 读取三个坐标  
    origin.originX = read_i16(cachedStream);  
    origin.originY = read_i16(cachedStream);  
    origin.originZ = read_i16(cachedStream);  
      
    return origin;  
}
    // 析构函数确保文件流关闭  
    ~BCFStreamReader() {  
        if (cachedStream.is_open()) {  
            cachedStream.close();  
        }  
    }  
};
