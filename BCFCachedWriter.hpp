#pragma once  
#include "bcf_structs.hpp"  
#include "bcf_io.hpp"  
#include "SubChunkUtils.hpp"  
#include "BlockUtils.hpp"  
#include "RegionMergeUtils.hpp"
#include <fstream>  
#include <string>  
#include <map>  
#include <vector>  
#include <filesystem>  
#include <unordered_map>  
#include <algorithm>  
#include <io/stream_writer.h>
#include <iostream>
struct BlockData {
    int x, y, z;
    std::string blockType;
    std::vector<std::pair<std::string, std::string>> states;
};


class BCFCachedWriter {  
private:  
    std::string outputFilename;  
    std::string tempDir;  
      
    // 新增: 世界尺寸  
    uint16_t width = 144;   // x 方向尺寸  
    uint16_t length = 144;  // z 方向尺寸  
    uint16_t height = 376; // y 方向尺寸 (320 - (-56) = 376)  
    int minY = -56;        // 最小 y 坐标  
// 新增: 动态边界追踪  
int minX = std::numeric_limits<int>::max();  
int maxX = std::numeric_limits<int>::min();  
int minZ = std::numeric_limits<int>::max();  
int maxZ = std::numeric_limits<int>::min();  
bool hasBounds = false;  // 标记是否已有方块数据

    // 元数据  
    std::vector<PaletteKey> paletteList;  
    std::map<BlockTypeID, std::string> typeMap;  
    std::map<BlockStateID, std::string> stateMap;  
      
    // 优化: 反向映射表,O(1) 查找  
    std::unordered_map<std::string, BlockTypeID> typeNameToId;  
    std::unordered_map<std::string, BlockStateID> stateNameToId;  
      

    std::map<StateValueID, std::string> stateValueMap;  // ID -> 状态值字符串  
    std::unordered_map<std::string, StateValueID> stateValueToId;  // 反向映射,O(1) 查找  
    StateValueID nextStateValueId = 0;

    // 跟踪缓存文件  
    std::map<int, std::string> subChunkCacheFiles;  
std::unordered_map<int, std::ofstream> tempFileHandles;
      
    // 当前活跃的 sub-chunk  
    std::map<int, std::vector<BlockGroup>> activeSubChunks;  
    size_t maxBlocksInMemory = 25000;  
      
    // ID 管理  
    std::unordered_map<PaletteKey, PaletteID, PaletteKeyHash> paletteCache;  
    BlockTypeID nextTypeId = 0;  
    BlockStateID nextStateId = 0;  

private:  
    size_t blockCounter = 0;  
    static constexpr size_t FLUSH_CHECK_INTERVAL = 200;
  
public:  
    BCFCachedWriter(const std::string& filename,
        const std::string& tempDir = "./temp_bcf_cache",
        size_t maxBlocks = 5000,
        uint16_t worldWidth = 144,
        uint16_t worldLength = 144,
        uint16_t worldHeight = 376,
        int worldMinY = -56)
        : outputFilename(filename), tempDir(tempDir),
        maxBlocksInMemory(maxBlocks),
        width(worldWidth), length(worldLength),
        height(worldHeight), minY(worldMinY) {
        std::filesystem::create_directories(tempDir);
    }
void addBlock(int x, int y, int z,       
    const std::string& blockType,  
    const std::vector<std::pair<std::string, std::string>>& states = {},  
    std::shared_ptr<nbt::tag_compound> nbtData = nullptr) {  // 使用libnbt++类型  
  
    // 动态更新边界    
    if (!hasBounds) {    
        minX = maxX = x;    
        minZ = maxZ = z;    
        hasBounds = true;    
    } else {    
        minX = std::min(minX, x);    
        maxX = std::max(maxX, x);    
        minZ = std::min(minZ, z);    
        maxZ = std::max(maxZ, z);    
    }    
  
    // 计算相对于边界的坐标    
    int relativeX = x - minX;    
    int relativeZ = z - minZ;    
  
    // 计算当前的世界尺寸 (向上取整到 144 的倍数)    
    int currentWidth = maxX - minX + 1;    
    int currentLength = maxZ - minZ + 1;    
    int subChunkCountX = (currentWidth + 143) / 144;    
  
    // 使用相对坐标计算 sub-chunk 索引    
    int subChunkIndexX = relativeX / 144;    
    int subChunkIndexZ = relativeZ / 144;    
    int subChunkIndex = subChunkIndexZ * subChunkCountX + subChunkIndexX;    
  
    // 计算局部坐标 (相对于 sub-chunk)    
    int localX = relativeX % 144;    
    int localZ = relativeZ % 144;    
    int localY = y + 56;  // 将 y 从 [-56, 320] 映射到 [0, 376]    
  
    // 获取或创建 IDs (使用优化的 O(1) 查找)      
    BlockTypeID typeId = getOrCreateTypeId(blockType);      
  
    // 构建PaletteKey  
    PaletteKey key;      
    key.typeId = typeId;      
    for (const auto& [stateName, stateValue] : states) {    
        BlockStateID stateId = getOrCreateStateId(stateName);    
        StateValueID valueId = getOrCreateStateValue(stateValue);    
        key.states.push_back({ stateId, valueId });    
    }    
    key.nbtData = nbtData;  // 直接存储tag_compound*  
    PaletteID paletteId = getOrCreatePaletteId(key);      
  
    // 添加到对应的 sub-chunk      
    auto& subChunk = activeSubChunks[subChunkIndex];      
    addBlockToGroup(subChunk, paletteId, localX, localY, localZ);      
      // 优化：批量检查flush  
    if (++blockCounter >= FLUSH_CHECK_INTERVAL) {  
        checkAndFlush();  
        blockCounter = 0;  
    }  
}  
    // 完成写入 
void finalize() {  
    // 1. 关闭所有临时文件句柄（优化4：文件句柄缓存）  
    for (auto& [index, ofs] : tempFileHandles) {  
        ofs.close();  
    }  
    tempFileHandles.clear();  
      
    // 2. Flush 所有剩余的 sub-chunk  
    for (auto& [index, groups] : activeSubChunks) {  
        flushSubChunkToCache(index, std::move(groups));  
    }  
    activeSubChunks.clear();  
      
    // 3. 重置计数器（优化2：减少flush检查频率）  
    blockCounter = 0;  
      
    // 4. 合并所有缓存文件并写入最终BCF  
    mergeAllCacheFiles();  
      
    // 5. 清理临时文件  
    cleanup();  
}
      

//void addBlocks(std::vector<BlockData>& blocks) {  
//    // 更新BlockData结构体包含NBT数据  
//    struct BlockData {  
//        int x, y, z;  
//        std::string blockType;  
//        std::vector<std::pair<std::string, std::string>> states;  
//        nbt::tag_compound* nbtData;  // 新增NBT字段  
//    };  
//      
//    for (auto& block : blocks) {  
//         // 内联 addBlock 逻辑以避免函数调用开销  
//          if (!hasBounds) {
//             minX = maxX = block.x;
//              minZ = maxZ = block.z;
//              hasBounds = true;
//          }
//         else {
//           minX = std::min(minX, block.x);
//               maxX = std::max(maxX, block.x);
//               minZ = std::min(minZ, block.z);
//                maxZ = std::max(maxZ, block.z);
//           }
//
//           int relativeX = block.x - minX;
//           int relativeZ = block.z - minZ;
//           int currentWidth = maxX - minX + 1;
//          int subChunkCountX = (currentWidth + 143) / 144;
//          int subChunkIndexX = relativeX / 144;
//        int subChunkIndexZ = relativeZ / 144;
//           int subChunkIndex = subChunkIndexZ * subChunkCountX + subChunkIndexX;
//          int localX = relativeX % 144;
//        int localZ = relativeZ % 144;
//          int localY = block.z + 56;
//        BlockTypeID typeId = getOrCreateTypeId(block.blockType);  
//        PaletteKey key;  
//        key.typeId = typeId;  
//        for (const auto& [stateName, stateValue] : block.states) {  
//            BlockStateID stateId = getOrCreateStateId(stateName);  
//            StateValueID valueId = getOrCreateStateValue(stateValue);  
//            key.states.push_back({ stateId, valueId });  
//        }  
//        key.nbtData = block.nbtData;  // 新增：设置NBT数据  
//        PaletteID paletteId = getOrCreatePaletteId(key);  
//          
//        auto& subChunk = activeSubChunks[subChunkIndex];  
//        addBlockToGroup(subChunk, paletteId, localX, localY, localZ);  
//    }  
//    checkAndFlush();  
//}

    ~BCFCachedWriter() {  
        if (!activeSubChunks.empty() || !subChunkCacheFiles.empty()) {  
            cleanup();  
        }  
    }  
  
private:  
    // 优化: O(1) 查找,使用反向映射  
    BlockTypeID getOrCreateTypeId(const std::string& blockType) {  
        auto it = typeNameToId.find(blockType);  
        if (it != typeNameToId.end()) {  
            return it->second;  
        }  
          
        BlockTypeID newId = nextTypeId++;  
        typeMap[newId] = blockType;  
        typeNameToId[blockType] = newId;  
        return newId;  
    }  
      
    BlockStateID getOrCreateStateId(const std::string& stateName) {  
        auto it = stateNameToId.find(stateName);  
        if (it != stateNameToId.end()) {  
            return it->second;  
        }  
          
        BlockStateID newId = nextStateId++;  
        stateMap[newId] = stateName;  
        stateNameToId[stateName] = newId;  
        return newId;  
    }  
      
    StateValueID getOrCreateStateValue(const std::string& stateValue) {
        auto it = stateValueToId.find(stateValue);
        if (it != stateValueToId.end()) {
            return it->second;
        }

        StateValueID newId = nextStateValueId++;
        stateValueMap[newId] = stateValue;
        stateValueToId[stateValue] = newId;
        return newId;
    }
    PaletteID getOrCreatePaletteId(const PaletteKey& key) {  
        auto it = paletteCache.find(key);  
        if (it != paletteCache.end()) {  
            return it->second;  
        }  
          
        PaletteID newId = static_cast<PaletteID>(paletteList.size());  
        paletteList.push_back(key);  
        paletteCache[key] = newId;  
        return newId;  
    }  
    void addBlockToGroup(std::vector<BlockGroup>& subChunk,
        PaletteID paletteId, int x, int y, int z)
    {
        // 1️⃣ 查找已有 BlockGroup
        for (auto& group : subChunk) {
            if (group.paletteId == paletteId) {
                // 2️⃣ 确保 vector 有足够容量
                size_t sz = group.x.size();
                if (group.x.capacity() < sz + 1) {
                    size_t newCap = std::max(sz + 1, sz * 2);
                    group.x.reserve(newCap);
                    group.y.reserve(newCap);
                    group.z.reserve(newCap);
                }
                // 3️⃣ 添加方块
                BlockUtils::addBlock(group, x, y, z);
                return;
            }
        }

        // 4️⃣ 没有找到对应 paletteId，新建 BlockGroup
        BlockGroup newGroup;
        newGroup.paletteId = paletteId;

        constexpr size_t initialReserve = 1000;
        newGroup.x.reserve(initialReserve);
        newGroup.y.reserve(initialReserve);
        newGroup.z.reserve(initialReserve);

        BlockUtils::addBlock(newGroup, x, y, z);

        // 5️⃣ 安全 push 到 subChunk
        try {
            subChunk.push_back(std::move(newGroup));
        }
        catch (const std::exception& e) {
            std::cerr << "Error pushing new BlockGroup: " << e.what() << std::endl;
        }
    }

    // ==========================

    void checkAndFlush() {
        size_t total = getTotalBlocksInMemory();
        if (total < maxBlocksInMemory) return;

        // 1️⃣ 统计各 sub-chunk 大小
        std::vector<std::pair<int, size_t>> sizes;
        for (const auto& [idx, groups] : activeSubChunks) {
            size_t count = 0;
            for (const auto& g : groups) count += g.count;
            sizes.push_back({ idx, count });
        }

        // 2️⃣ 按大小降序排序
        std::sort(sizes.begin(), sizes.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

        // 3️⃣ Flush 直到低于阈值
        size_t target = static_cast<size_t>(maxBlocksInMemory * 0.8);
        for (const auto& [idx, size] : sizes) {
            if (total <= target) break;

            auto it = activeSubChunks.find(idx);
            if (it != activeSubChunks.end()) {
                // move 出去
                std::vector<BlockGroup> groups;
                groups.swap(it->second);

                // 删除原条目，确保不再访问
                activeSubChunks.erase(it);

                // flush
                flushSubChunkToCache(idx, std::move(groups));
            }
            total -= size;
        }
    }

    size_t getTotalBlocksInMemory() {  
        size_t total = 0;  
        for (const auto& [idx, groups] : activeSubChunks) {  
            for (const auto& group : groups) {  
                total += group.count;  
            }  
        }  
        return total;  
    }  
      


    void flushSubChunkToCache(int subChunkIndex, std::vector<BlockGroup>&& groups) {
        try {
            auto& ofs = tempFileHandles[subChunkIndex];
            if (!ofs.is_open()) {
                std::string cacheFile = tempDir + "/subchunk_" + std::to_string(subChunkIndex) + ".tmp";
                ofs.open(cacheFile, std::ios::binary | std::ios::app);
                if (!ofs) throw std::runtime_error("Failed to open cache file");
            }

            write_u32(ofs, static_cast<uint32_t>(groups.size()));
            for (auto& bg : groups) {
                BlockUtils::writeBlockGroup(ofs, bg);
            }

            subChunkCacheFiles[subChunkIndex] =
                tempDir + "/subchunk_" + std::to_string(subChunkIndex) + ".tmp";
        }
        catch (const std::exception& e) {
            std::cerr << "flushSubChunkToCache error: " << e.what() << std::endl;
        }
    }
void mergeAllCacheFiles() {  
    std::ofstream ofs(outputFilename, std::ios::binary);  
    if (!ofs) {  
        throw std::runtime_error("Failed to create output file: " + outputFilename);  
    }  
  
    // 计算最终的世界尺寸  
    int finalWidth = hasBounds ? (maxX - minX + 1) : 144;  
    int finalLength = hasBounds ? (maxZ - minZ + 1) : 144;  
    int subChunkCountX = (finalWidth + 143) / 144;  
  
    // 写入占位符 header      
    BCFHeader header;  
    header.width = static_cast<uint16_t>(finalWidth);  
    header.length = static_cast<uint16_t>(finalLength);  
    header.height = height;  
    write_le<BCFHeader>(ofs, header);  
  
    // 按顺序处理所有 sub-chunk      
    std::vector<FilePos> subChunkOffsets;  
  
    for (const auto& [index, cacheFile] : subChunkCacheFiles) {  
        subChunkOffsets.push_back(ofs.tellp());  
  
        // 计算当前 subchunk 的起始坐标 (使用动态边界)  
        int subChunkX = index % subChunkCountX;  
        int subChunkZ = index / subChunkCountX;  
        Coord originX = static_cast<Coord>(minX + subChunkX * 144);  
        Coord originY = static_cast<Coord>(minY);  
        Coord originZ = static_cast<Coord>(minZ + subChunkZ * 144);  
  
        // 读取所有片段并合并  
        std::ifstream ifs(cacheFile, std::ios::binary);  
        if (!ifs) {  
            throw std::runtime_error("Failed to read cache file: " + cacheFile);  
        }  
  
        std::vector<BlockGroup> allGroups;  
  
        // 读取所有片段并收集      
        while (ifs.peek() != EOF) {  
            uint32_t groupCount = read_u32(ifs);  
            for (uint32_t i = 0; i < groupCount; i++) {  
                allGroups.push_back(BlockUtils::readBlockGroup(ifs));  
            }  
        }  
        ifs.close();  
  
        // 先合并相同 paletteId 的 BlockGroup      
        allGroups = MergeUtils::mergeBlockGroups(allGroups);  
  
        // 使用 RegionMergeUtils 将 BlockGroup 转换为 BlockRegion      
        auto mergedRegions = RegionMergeUtils::mergeToRegions(allGroups);  
  
        // 传递三个坐标参数      
        SubChunkUtils::writeSubChunk(ofs, mergedRegions, originX, originY, originZ);  
    }  
  
    // 写入子区块偏移量表  
    FilePos offsetTablePos = ofs.tellp();  
    write_u64(ofs, subChunkOffsets.size());  
    for (auto offset : subChunkOffsets) {  
        write_u64(ofs, offset);  
    }  
  
    // 写入 palette 时序列化 NBT 数据  
    FilePos palettePos = ofs.tellp();  
    write_u32(ofs, static_cast<uint32_t>(paletteList.size()));  
    for (size_t pid = 0; pid < paletteList.size(); pid++) {  
        const auto& k = paletteList[pid];  
        write_u32(ofs, static_cast<uint32_t>(pid));  
        write_u16(ofs, k.typeId);  
        write_u16(ofs, static_cast<uint16_t>(k.states.size()));  
        for (const auto& s : k.states) {  
            write_u8(ofs, s.first);  
            write_u8(ofs, s.second);  
        }  
          
        // 序列化 NBT 数据  
        if (k.nbtData) {  
            std::ostringstream oss;  
            nbt::io::stream_writer writer(oss,endian::little);
            writer.write_tag("" ,* k.nbtData);
            std::string nbtStr = oss.str();  
            writeString32(ofs, nbtStr);
        } else {  
            writeString32(ofs, "");  // 空NBT数据  
        }  
    }  
  
    // 写入类型名映射  
    FilePos typeMapPos = ofs.tellp();  
    write_u32(ofs, static_cast<uint32_t>(typeMap.size()));  
    for (const auto& kv : typeMap) {  
        write_u16(ofs, kv.first);  
        writeString16(ofs, kv.second);  
    }  
  
    // 写入状态映射  
    FilePos stateMapPos = ofs.tellp();  
    write_u32(ofs, static_cast<uint32_t>(stateMap.size()));  
    for (const auto& kv : stateMap) {  
        write_u8(ofs, kv.first);  
        writeString16(ofs, kv.second);  
    }  
  
    // 写入 state value map      
    FilePos stateValueMapPos = ofs.tellp();  
    write_u32(ofs, static_cast<uint32_t>(stateValueMap.size()));  
    for (const auto& kv : stateValueMap) {  
        write_u8(ofs, kv.first);  
        writeString16(ofs, kv.second);  
    }  
  
    // 更新 header (版本升级到 4)      
    header.version = 4;  
    header.subChunkCount = subChunkOffsets.size();  
    header.subChunkOffsetsTableOffset = offsetTablePos;  
    header.paletteOffset = palettePos;  
    header.blockTypeMapOffset = typeMapPos;  
    header.stateNameMapOffset = stateMapPos;  
    header.stateValueMapOffset = stateValueMapPos;  
    header.nbtDataOffset = 0;  // NBT 数据嵌入在 palette 中  
  
    ofs.seekp(0);  
    write_le<BCFHeader>(ofs, header);  
    ofs.close();  
}
    void cleanup() {  
        // 删除所有临时文件  
            // 1. 强制关闭所有文件句柄  
        for (auto& [index, ofs] : tempFileHandles) {
            if (ofs.is_open()) {
                ofs.flush();      // 确保数据写入  
                ofs.close();      // 关闭文件  
            }
        }
        for (const auto& [index, cacheFile] : subChunkCacheFiles) {  
            try {  
                std::filesystem::remove(cacheFile);  
            } catch (std::exception& e) {
               std::cerr << "Failed to remove cache file: " << e.what() << std::endl;
            }  
        }  
          
        // 删除临时目录  
        try {  
            if (std::filesystem::exists(tempDir) &&   
                std::filesystem::is_empty(tempDir)) {  
                std::filesystem::remove(tempDir);  
            }  
        } catch (std::exception& e) {
            std::cerr << "Failed to remove temp directory: " << e.what() << std::endl;
        }  
          
        subChunkCacheFiles.clear();  
    }  



};

