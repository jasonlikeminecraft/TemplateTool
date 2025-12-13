#pragma once  
#include "bcf_structs.hpp"  
#include <map>  
#include <tuple>  
#include <vector>  

struct RegionMergeUtils {
    static constexpr int64_t MAX_BLOCK_COUNT = 32767;

    // 将 BlockGroup 合并成 BlockRegion    
    static std::vector<BlockRegion> mergeToRegions(
        const std::vector<BlockGroup>& groups) {

        std::vector<BlockRegion> regions;

        // 按 paletteId 分组构建 3D 网格    
        std::map<PaletteID, std::map<std::tuple<Coord, Coord, Coord>, bool>> grids;

        for (const auto& bg : groups) {
            for (size_t i = 0; i < bg.count; i++) {
                grids[bg.paletteId][{bg.x[i], bg.y[i], bg.z[i]}] = true;
            }
        }

        // 对每个 paletteId 进行贪心合并    
        for (auto& [paletteId, grid] : grids) {
            while (!grid.empty()) {
                auto it = grid.begin();
                auto [x, y, z] = it->first;

                // 向 X 方向扩展    
                Coord maxX = x;
                while (grid.count({ maxX + 1, y, z })) maxX++;

                // 向 Z 方向扩展    
                Coord maxZ = z;
                bool canExpandZ = true;
                while (canExpandZ) {
                    // 检查扩展后的方块数量  
                    int64_t newCount = (int64_t)(maxX - x + 1) * (maxZ - z + 2);
                    if (newCount > MAX_BLOCK_COUNT) {
                        canExpandZ = false;
                        break;
                    }

                    for (Coord cx = x; cx <= maxX; cx++) {
                        if (!grid.count({ cx, y, maxZ + 1 })) {
                            canExpandZ = false;
                            break;
                        }
                    }
                    if (canExpandZ) maxZ++;
                }

                // 向 Y 方向扩展    
                Coord maxY = y;
                bool canExpandY = true;
                while (canExpandY) {
                    // 检查扩展后的方块数量  
                    int64_t newCount = (int64_t)(maxX - x + 1) * (maxZ - z + 1) * (maxY - y + 2);
                    if (newCount > MAX_BLOCK_COUNT) {
                        canExpandY = false;
                        break;
                    }

                    for (Coord cx = x; cx <= maxX; cx++) {
                        for (Coord cz = z; cz <= maxZ; cz++) {
                            if (!grid.count({ cx, maxY + 1, cz })) {
                                canExpandY = false;
                                break;
                            }
                        }
                        if (!canExpandY) break;
                    }
                    if (canExpandY) maxY++;
                }

                // 创建区域    
                BlockRegion region;
                region.paletteId = paletteId;
                region.x1 = x; region.y1 = y; region.z1 = z;
                region.x2 = maxX; region.y2 = maxY; region.z2 = maxZ;
                regions.push_back(region);

                // 移除已访问的方块    
                for (Coord cy = y; cy <= maxY; cy++) {
                    for (Coord cx = x; cx <= maxX; cx++) {
                        for (Coord cz = z; cz <= maxZ; cz++) {
                            grid.erase({ cx, cy, cz });
                        }
                    }
                }
            }
        }

        return regions;
    }
};