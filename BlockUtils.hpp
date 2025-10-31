#pragma once
#include "bcf_structs.hpp"
#include "bcf_io.hpp"
#include <fstream>
#include <unordered_map>
// -------------------- BlockGroup 工具 --------------------
struct BlockUtils {

    // 添加一个方块到 BlockGroup
    static void addBlock(BlockGroup& bg, Coord x, Coord y, Coord z) {
        bg.x.push_back(x); bg.y.push_back(y); bg.z.push_back(z);
        bg.count = static_cast<BlockCount>(bg.x.size());
    }

    // 读取 BlockGroup 的第 idx 个方块
    static void getBlock(const BlockGroup& bg, size_t idx, Coord& x, Coord& y, Coord& z) {
        x = bg.x[idx]; y = bg.y[idx]; z = bg.z[idx];
    }

    // 写 BlockGroup 到文件
    static void writeBlockGroup(std::ofstream& ofs, const BlockGroup& bg) {
        write_u32(ofs, bg.paletteId);
        write_u32(ofs, bg.count);
        for (size_t i = 0; i < bg.count; i++) {
            write_i16(ofs, bg.x[i]);
            write_i16(ofs, bg.y[i]);
            write_i16(ofs, bg.z[i]);
        }
    }

    // 从文件读取 BlockGroup
    static BlockGroup readBlockGroup(std::ifstream& ifs) {
        BlockGroup bg;
        bg.paletteId = read_u32(ifs);
        bg.count = read_u32(ifs);
        bg.x.resize(bg.count); bg.y.resize(bg.count); bg.z.resize(bg.count);
        for (size_t i = 0; i < bg.count; i++) {
            bg.x[i] = read_i16(ifs);
            bg.y[i] = read_i16(ifs);
            bg.z[i] = read_i16(ifs);
        }
        return bg;
    }
};


// -------------------- 一次性合并子区块内 BlockGroup --------------------
struct MergeUtils {

    // 注意：PaletteID 表示 type+state 的组合，如果你在内存里只有 PaletteID，可以直接用
    static std::vector<BlockGroup> mergeBlockGroups(const std::vector<BlockGroup>& groups) {
        std::unordered_map<PaletteID, BlockGroup> merged;

        for (const auto& bg : groups) {
            auto& target = merged[bg.paletteId];  // 自动创建
            target.paletteId = bg.paletteId;
            target.x.insert(target.x.end(), bg.x.begin(), bg.x.end());
            target.y.insert(target.y.end(), bg.y.begin(), bg.y.end());
            target.z.insert(target.z.end(), bg.z.begin(), bg.z.end());
            target.count = static_cast<BlockCount>(target.x.size());
        }

        // 转成 vector 返回
        std::vector<BlockGroup> result;
        result.reserve(merged.size());
        for (auto& kv : merged) result.push_back(kv.second);
        return result;
    }
};