#pragma once  
#include "bcf_structs.hpp"  
#include <unordered_map>  
#include <unordered_set>  
#include <tuple>  
#include <vector>  
#include <algorithm>  
#include <thread>  
#include <future>  
#include <array>  
  
struct RegionMergeUtils {  
    using Key = std::tuple<Coord, Coord, Coord>;  
      
    struct KeyHash {  
        std::size_t operator()(const Key& k) const noexcept {  
            auto [x, y, z] = k;  
            return static_cast<std::size_t>(  
                (x * 73856093) ^ (y * 19349663) ^ (z * 83492791)  
            );  
        }  
    };  
  
    static std::vector<BlockRegion> mergeToRegions(  
        const std::vector<BlockGroup>& groups)  
    {  
        std::vector<BlockRegion> regions;  
        std::unordered_map<PaletteID, std::unordered_set<Key, KeyHash>> grids;  
        grids.reserve(groups.size());  
  
        // 构建坐标哈希表  
        for (const auto& bg : groups) {  
            auto& grid = grids[bg.paletteId];  
            grid.reserve(bg.count);  
            for (size_t i = 0; i < bg.count; i++) {  
                grid.emplace(bg.x[i], bg.y[i], bg.z[i]);  
            }  
        }  
  
        // 6种扩展顺序  
        static const std::array<std::array<int, 3>, 6> orders = {{  
            {0, 1, 2}, {0, 2, 1}, {1, 0, 2},  
            {1, 2, 0}, {2, 0, 1}, {2, 1, 0}  
        }};  
  
        // 遍历每个 paletteId 的方块集合  
        for (auto& [paletteId, grid] : grids) {  
            while (!grid.empty()) {  
                auto it = grid.begin();  
                auto [x, y, z] = *it;  
  
                BlockRegion best;  
                  
                // 自适应并行策略: 只在大数据集时使用多线程  
                if (grid.size() > 5000) {  
                    // 并行版本  
                    std::array<std::future<BlockRegion>, 6> futures;  
                      
                    for (int i = 0; i < 6; i++) {  
                        futures[i] = std::async(std::launch::async,   
                            [&grid, x, y, z, paletteId, &orders, i]() {  
                                return tryExpansion(grid, x, y, z, paletteId, orders[i]);  
                            });  
                    }  
  
                    // 收集结果并选择最大体积  
                    int maxVolume = 0;  
                    for (int i = 0; i < 6; i++) {  
                        BlockRegion candidate = futures[i].get();  
                        int volume = (candidate.x2 - candidate.x1 + 1) *   
                                     (candidate.y2 - candidate.y1 + 1) *   
                                     (candidate.z2 - candidate.z1 + 1);  
                        if (volume > maxVolume) {  
                            maxVolume = volume;  
                            best = candidate;  
                        }  
                    }  
                } else {  
                    // 串行版本  
                    int maxVolume = 0;  
                    for (int i = 0; i < 6; i++) {  
                        BlockRegion candidate = tryExpansion(grid, x, y, z, paletteId, orders[i]);  
                        int volume = (candidate.x2 - candidate.x1 + 1) *   
                                     (candidate.y2 - candidate.y1 + 1) *   
                                     (candidate.z2 - candidate.z1 + 1);  
                        if (volume > maxVolume) {  
                            maxVolume = volume;  
                            best = candidate;  
                        }  
                    }  
                }  
  
                regions.push_back(best);  
  
                // 批量删除优化  
                removeRegion(grid, best);  
            }  
        }  
  
        return regions;  
    }  
  
private:  
    static BlockRegion tryExpansion(  
        const std::unordered_set<Key, KeyHash>& grid,  
        Coord x, Coord y, Coord z,  
        PaletteID paletteId,  
        const std::array<int, 3>& order)  
    {  
        BlockRegion region;  
        region.paletteId = paletteId;  
        region.x1 = region.x2 = x;  
        region.y1 = region.y2 = y;  
        region.z1 = region.z2 = z;  
  
        for (int axis : order) {  
            if (axis == 0) expandX(grid, region);  
            else if (axis == 1) expandZ(grid, region);  
            else expandY(grid, region);  
        }  
  
        return region;  
    }  
  
    static void expandX(const std::unordered_set<Key, KeyHash>& grid, BlockRegion& r) {  
        while (true) {  
            Coord nextX = r.x2 + 1;  
            bool canExpand = true;  
              
            for (Coord cy = r.y1; cy <= r.y2; cy++) {  
                for (Coord cz = r.z1; cz <= r.z2; cz++) {  
                    if (!grid.count({nextX, cy, cz})) {  
                        canExpand = false;  
                        goto exit_x;  
                    }  
                }  
            }  
            exit_x:  
            if (canExpand) r.x2 = nextX;  
            else break;  
        }  
    }  
  
    static void expandZ(const std::unordered_set<Key, KeyHash>& grid, BlockRegion& r) {  
        while (true) {  
            Coord nextZ = r.z2 + 1;  
            bool canExpand = true;  
              
            for (Coord cx = r.x1; cx <= r.x2; cx++) {  
                for (Coord cy = r.y1; cy <= r.y2; cy++) {  
                    if (!grid.count({cx, cy, nextZ})) {  
                        canExpand = false;  
                        goto exit_z;  
                    }  
                }  
            }  
            exit_z:  
            if (canExpand) r.z2 = nextZ;  
            else break;  
        }  
    }  
  
    static void expandY(const std::unordered_set<Key, KeyHash>& grid, BlockRegion& r) {  
        while (true) {  
            Coord nextY = r.y2 + 1;  
            bool canExpand = true;  
              
            for (Coord cx = r.x1; cx <= r.x2; cx++) {  
                for (Coord cz = r.z1; cz <= r.z2; cz++) {  
                    if (!grid.count({cx, nextY, cz})) {  
                        canExpand = false;  
                        goto exit_y;  
                    }  
                }  
            }  
            exit_y:  
            if (canExpand) r.y2 = nextY;  
            else break;  
        }  
    }  
  
    static void removeRegion(std::unordered_set<Key, KeyHash>& grid, const BlockRegion& r) {  
        size_t toRemove = (r.x2 - r.x1 + 1) * (r.y2 - r.y1 + 1) * (r.z2 - r.z1 + 1);  
          
        // 如果删除的方块数量超过一半,重建哈希表更高效  
        if (toRemove > grid.size() / 2) {  
            std::unordered_set<Key, KeyHash> newGrid;  
            newGrid.reserve(grid.size() - toRemove);  
              
            for (const auto& key : grid) {  
                auto [x, y, z] = key;  
                if (x < r.x1 || x > r.x2 ||   
                    y < r.y1 || y > r.y2 ||   
                    z < r.z1 || z > r.z2) {  
                    newGrid.insert(key);  
                }  
            }  
            grid = std::move(newGrid);  
        } else {  
            // 正常逐个删除  
            for (Coord cy = r.y1; cy <= r.y2; cy++) {  
                for (Coord cx = r.x1; cx <= r.x2; cx++) {  
                    for (Coord cz = r.z1; cz <= r.z2; cz++) {  
                        grid.erase({cx, cy, cz});  
                    }  
                }  
            }  
        }  
    }  
};
