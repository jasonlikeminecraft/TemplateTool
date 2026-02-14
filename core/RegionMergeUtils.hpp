#pragma once  
#include "bcf_structs.hpp"  
#include <map>  
#include <tuple>  
#include <vector>  
#include <unordered_set>
#include <unordered_map>




struct RegionMergeUtils {
    static constexpr int64_t MAX_BLOCK_COUNT = 32767;

    // 坐标压缩成 uint64_t
    static inline uint64_t packPos(Coord x, Coord y, Coord z) noexcept {
        return (uint64_t(uint16_t(x)) << 32)
            | (uint64_t(uint16_t(y)) << 16)
            | uint64_t(uint16_t(z));
    }

    static inline void unpackPos(uint64_t key, Coord& x, Coord& y, Coord& z) noexcept {
        x = static_cast<Coord>((key >> 32) & 0xFFFF);
        y = static_cast<Coord>((key >> 16) & 0xFFFF);
        z = static_cast<Coord>(key & 0xFFFF);
    }

    // 将 BlockGroup 合并成 BlockRegion
    static std::vector<BlockRegion> mergeToRegions(const std::vector<BlockGroup>& groups) {
        std::vector<BlockRegion> regions;

        // 构建 grids
        std::unordered_map<PaletteID, std::unordered_set<uint64_t>> grids;

        for (const auto& bg : groups) {
            // 先用临时 vector 收集坐标
            std::vector<uint64_t> temp;
            temp.reserve(bg.count);
            for (size_t i = 0; i < bg.count; i++) {
                temp.push_back(packPos(bg.x[i], bg.y[i], bg.z[i]));
            }

            // 一次性构造 unordered_set，减少 insert 扩容次数
            grids[bg.paletteId].reserve(bg.count); // 先 reserve
            grids[bg.paletteId].insert(temp.begin(), temp.end());
        }

        // 贪心合并
        for (auto& [paletteId, grid] : grids) {
            while (!grid.empty()) {
                auto it = grid.begin();
                uint64_t startKey = *it;
                Coord startX, startY, startZ;
                unpackPos(startKey, startX, startY, startZ);

                // X 方向扩展
                Coord maxX = startX;
                while (grid.find(packPos(maxX + 1, startY, startZ)) != grid.end()) {
                    maxX++;
                }

                // Z 方向扩展
                Coord maxZ = startZ;
                bool canExpandZ = true;
                while (canExpandZ) {
                    for (Coord cx = startX; cx <= maxX; cx++) {
                        if (grid.find(packPos(cx, startY, maxZ + 1)) == grid.end()) {
                            canExpandZ = false;
                            break;
                        }
                    }
                    if (canExpandZ) maxZ++;
                }

                // Y 方向扩展
                Coord maxY = startY;
                bool canExpandY = true;
                while (canExpandY) {
                    for (Coord cx = startX; cx <= maxX; cx++) {
                        for (Coord cz = startZ; cz <= maxZ; cz++) {
                            if (grid.find(packPos(cx, maxY + 1, cz)) == grid.end()) {
                                canExpandY = false;
                                break;
                            }
                        }
                        if (!canExpandY) break;
                    }
                    if (canExpandY) maxY++;
                }

                // 创建区域
                regions.push_back({
                    paletteId,
                    startX, startY, startZ,
                    maxX, maxY, maxZ
                    });

                // 批量删除
                std::vector<uint64_t> eraseList;
                eraseList.reserve((maxX - startX + 1) * (maxY - startY + 1) * (maxZ - startZ + 1));

                for (Coord y = startY; y <= maxY; y++) {
                    for (Coord x = startX; x <= maxX; x++) {
                        for (Coord z = startZ; z <= maxZ; z++) {
                            eraseList.push_back(packPos(x, y, z));
                        }
                    }
                }

                for (uint64_t key : eraseList) {
                    grid.erase(key);
                }
            }
        }

        return regions;
    }
};