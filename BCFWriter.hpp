#pragma once  
#include "bcf_structs.hpp"  
#include "BCFUtils.hpp"  
#include "BlockUtils.hpp"  
#include "MergeUtils.hpp"  
#include <unordered_map>  
#include <string>  
  
class BCFWriter {  
private:  
    std::vector<std::vector<BlockGroup>> subChunks;  
    std::vector<PaletteKey> paletteList;  
    std::map<BlockTypeID, std::string> typeMap;  
    std::map<BlockStateID, std::string> stateMap;  
      
    // 用于快速查找已存在的 palette  
    std::unordered_map<PaletteKey, PaletteID, PaletteKeyHash> paletteCache;  
      
    // 下一个可用的 ID  
    BlockTypeID nextTypeId = 0;  
    BlockStateID nextStateId = 0;  
  
public:  
    BCFWriter() = default;  
      
    // 添加方块,自动处理所有映射  
    void addBlock(int subChunkIndex, int x, int y, int z,   
                  const std::string& blockType,  
                  const std::vector<std::pair<std::string, uint8_t>>& states = {}) {  
          
        // 确保子区块存在  
        while (subChunks.size() <= subChunkIndex) {  
            subChunks.push_back({});  
        }  
          
        // 获取或创建 typeId  
        BlockTypeID typeId = getOrCreateTypeId(blockType);  
          
        // 构建 PaletteKey  
        PaletteKey key;  
        key.typeId = typeId;  
        for (const auto& [stateName, stateValue] : states) {  
            BlockStateID stateId = getOrCreateStateId(stateName);  
            key.states.push_back({stateId, stateValue});  
        }  
          
        // 获取或创建 paletteId  
        PaletteID paletteId = getOrCreatePaletteId(key);  
          
        // 添加方块到对应的 BlockGroup  
        addBlockToGroup(subChunkIndex, paletteId, x, y, z);  
    }  
      
    // 写入文件  
    void write(const std::string& filename) {  
        // 合并每个子区块中的重复 BlockGroup  
        for (auto& subChunk : subChunks) {  
            subChunk = MergeUtils::mergeBlockGroups(subChunk);  
        }  
          
        BCFUtils::writeBCF(filename, subChunks, paletteList, typeMap, stateMap);  
    }  
      
private:  
    BlockTypeID getOrCreateTypeId(const std::string& blockType) {  
        // 查找是否已存在  
        for (const auto& [id, name] : typeMap) {  
            if (name == blockType) return id;  
        }  
          
        // 创建新的  
        BlockTypeID newId = nextTypeId++;  
        typeMap[newId] = blockType;  
        return newId;  
    }  
      
    BlockStateID getOrCreateStateId(const std::string& stateName) {  
        // 查找是否已存在  
        for (const auto& [id, name] : stateMap) {  
            if (name == stateName) return id;  
        }  
          
        // 创建新的  
        BlockStateID newId = nextStateId++;  
        stateMap[newId] = stateName;  
        return newId;  
    }  
      
    PaletteID getOrCreatePaletteId(const PaletteKey& key) {  
        // 查找缓存  
        auto it = paletteCache.find(key);  
        if (it != paletteCache.end()) {  
            return it->second;  
        }  
          
        // 创建新的  
        PaletteID newId = static_cast<PaletteID>(paletteList.size());  
        paletteList.push_back(key);  
        paletteCache[key] = newId;  
        return newId;  
    }  
      
    void addBlockToGroup(int subChunkIndex, PaletteID paletteId, int x, int y, int z) {  
        auto& subChunk = subChunks[subChunkIndex];  
          
        // 查找是否已有相同 paletteId 的 BlockGroup  
        for (auto& group : subChunk) {  
            if (group.paletteId == paletteId) {  
                BlockUtils::addBlock(group, x, y, z);  
                return;  
            }  
        }  
          
        // 创建新的 BlockGroup  
        BlockGroup newGroup;  
        newGroup.paletteId = paletteId;  
        BlockUtils::addBlock(newGroup, x, y, z);  
        subChunk.push_back(newGroup);  
    }  
};
