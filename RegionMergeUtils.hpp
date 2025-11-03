#pragma once
#include "bcf_structs.hpp"
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <vector>
#include <cstdint>

struct RegionMergeUtils {

    // 坐标键类型
    using Key = std::tuple<Coord, Coord, Coord>;

    // 高效三维坐标哈希
    struct KeyHash {
        std::size_t operator()(const Key& k) const noexcept {
            auto [x, y, z] = k;
            // 使用质数混合避免坐标聚集导致哈希碰撞
            return static_cast<std::size_t>(
                (x * 73856093) ^ (y * 19349663) ^ (z * 83492791)
            );
        }
    };

    static std::vector<BlockRegion> mergeToRegions(
        const std::vector<BlockGroup>& groups)
    {
        std::vector<BlockRegion> regions;

        // paletteId → 该类型的方块坐标集合
        std::unordered_map<PaletteID, std::unordered_set<Key, KeyHash>> grids;
        grids.reserve(groups.size());

        // 构建坐标哈希表
        for (const auto& bg : groups) {
            auto& grid = grids[bg.paletteId];
            grid.reserve(bg.count);
            for (size_t i = 0; i < bg.count; i++) {
                grid.emplace(bg.x[i], bg.y[i], bg.z[i]);
            }
        }

        // 遍历每个 paletteId 的方块集合
        for (auto& [paletteId, grid] : grids) {
            while (!grid.empty()) {
                // 取第一个方块作为起点
                auto it = grid.begin();
                auto [x, y, z] = *it;

                // X 向扩展
                Coord maxX = x;
                while (grid.count({maxX + 1, y, z})) maxX++;

                // Z 向扩展
                Coord maxZ = z;
                bool canExpandZ = true;
                while (canExpandZ) {
                    for (Coord cx = x; cx <= maxX; cx++) {
                        if (!grid.count({cx, y, maxZ + 1})) {
                            canExpandZ = false;
                            break;
                        }
                    }
                    if (canExpandZ) maxZ++;
                }

                // Y 向扩展
                Coord maxY = y;
                bool canExpandY = true;
                while (canExpandY) {
                    for (Coord cx = x; cx <= maxX; cx++) {
                        for (Coord cz = z; cz <= maxZ; cz++) {
                            if (!grid.count({cx, maxY + 1, cz})) {
                                canExpandY = false;
                                break;
                            }
                        }
                        if (!canExpandY) break;
                    }
                    if (canExpandY) maxY++;
                }

                // 添加合并区域
                BlockRegion region;
                region.paletteId = paletteId;
                region.x1 = x; region.y1 = y; region.z1 = z;
                region.x2 = maxX; region.y2 = maxY; region.z2 = maxZ;
                regions.push_back(region);

                // 移除已合并方块
                for (Coord cy = y; cy <= maxY; cy++) {
                    for (Coord cx = x; cx <= maxX; cx++) {
                        for (Coord cz = z; cz <= maxZ; cz++) {
                            grid.erase({cx, cy, cz});
                        }
                    }
                }
            }
        }

        return regions;
    }
};
