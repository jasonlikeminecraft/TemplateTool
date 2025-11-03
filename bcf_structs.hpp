#pragma once
#include <cstdint>
#include <vector>
#include <string>

// -------------------- 类型别名 --------------------
using FilePos = uint64_t;   // 文件偏移量或大小
using SubChunkSize = uint64_t;   // 子区块大小
using PaletteID = uint32_t;   // paletteId
using BlockCount = uint32_t;   // 方块数量
using Coord = int16_t;    // 坐标 x/y/z 范围 -32767~32767
using BlockTypeID = uint16_t;   // 方块类型 ID
using BlockStateID = uint8_t;    // 方块状态 ID
using StateValueID = uint8_t;    // 状态值 ID  
using Version = uint8_t;    // 文件版本号

using StatePair = std::pair<BlockStateID, StateValueID>;
#pragma pack(push,1)
// -------------------- 文件头 --------------------

struct BCFHeader {  
    char magic[3];           // "BCF"  
    Version version;         // 版本 2 支持偏移量表  
    uint16_t width, length, height;  
    uint8_t subChunkBaseSize;  
    FilePos subChunkCount;  
    FilePos subChunkOffsetsTableOffset;  // 新增:子区块偏移量表位置  
    FilePos paletteOffset;  
    FilePos blockTypeMapOffset;  
    FilePos stateNameMapOffset;  
    FilePos stateValueMapOffset;  // 新增字段  
  
    BCFHeader()  
        : version(2), width(144), length(144), height(376),  // 版本改为 2  
        subChunkBaseSize(376), subChunkCount(0),
        subChunkOffsetsTableOffset(0),  // 新增字段  
        paletteOffset(0), blockTypeMapOffset(0), stateNameMapOffset(0)  , stateValueMapOffset(0)
    {  
        magic[0] = 'B'; magic[1] = 'C'; magic[2] = 'F';  
    }  
};

// -------------------- 子区块头 --------------------
struct SubChunkHeader {
    SubChunkSize subChunkSize;
    Coord originX;  // 新增: X 方向起始坐标  
    Coord originY;
    Coord originZ;  // 新增: Z 方向起始坐标  
    BlockCount blockRegionCount;  // 改名:区域数量而非组数量  

    SubChunkHeader() : subChunkSize(0), originY(0), blockRegionCount(0) {}
};

// -------------------- 同类方块组 --------------------
struct BlockGroup {
    PaletteID paletteId;
    BlockCount count;
    std::vector<Coord> x, y, z;
};

// -------------------- PaletteKey --------------------
struct PaletteKey {
    BlockTypeID typeId;
    std::vector<StatePair> states;

    bool operator==(const PaletteKey& o) const {
        if (typeId != o.typeId || states.size() != o.states.size()) return false;
        for (size_t i = 0; i < states.size(); ++i)
            if (states[i] != o.states[i]) return false;
        return true;
    }
};

// PaletteKey 哈希
struct PaletteKeyHash {
    size_t operator()(const PaletteKey& k) const noexcept {
        size_t h = k.typeId;
        for (auto& s : k.states) {
            h ^= (s.first + 0x9e3779b9 + (h << 6) + (h >> 2));
            h ^= (s.second + 0x9e3779b9 + (h << 6) + (h >> 2));
        }
        return h;
    }
};

struct BlockInfo {
    Coord x, y, z;
    std::string type;                     // 类型字符串
    std::vector<std::pair<std::string, std::string>> states; // <状态名,状态值>
};



struct BlockRegion {
    PaletteID paletteId;
    Coord x1, y1, z1;  // 起始坐标  
    Coord x2, y2, z2;  // 结束坐标  
};
struct SubChunkOrigin {  
    Coord originX;  
    Coord originY;  
    Coord originZ;  
};  

#pragma pack(pop)
