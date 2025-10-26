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
using Version = uint8_t;    // 文件版本号

using StatePair = std::pair<BlockStateID, BlockStateID>;
#pragma pack(push,1)
// -------------------- 文件头 --------------------
struct BCFHeader {
    char magic[3];           // "BCF"
    Version version;
    uint16_t width, length, height;
    uint8_t subChunkBaseSize;
    FilePos subChunkCount;
    FilePos paletteOffset;
    FilePos blockTypeMapOffset;
    FilePos stateNameMapOffset;

    BCFHeader()
        : version(1), width(64), length(64), height(64),
        subChunkBaseSize(16), subChunkCount(0),
        paletteOffset(0), blockTypeMapOffset(0), stateNameMapOffset(0)
    {
        magic[0] = 'B'; magic[1] = 'C'; magic[2] = 'F';
    }
};

// -------------------- 子区块头 --------------------
struct SubChunkHeader {
    SubChunkSize subChunkSize;   // 子区块字节数
    Coord originY;
    BlockCount blockGroupCount;

    SubChunkHeader() : subChunkSize(0), originY(0), blockGroupCount(0) {}
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
    std::vector<std::pair<std::string, int>> states; // <状态名,状态值>
};
#pragma pack(pop)