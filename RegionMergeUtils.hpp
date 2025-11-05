#pragma once
#include "bcf_structs.hpp"
#include <vector>
#include <unordered_map>
#include <future>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <bit>

struct RegionMergeUtils {
    static constexpr int BITS_PER_SEG = 64;

    struct BitmaskGrid {
        PaletteID paletteId;
        int width, height, length;
        int xSegments; // width / 64
        std::vector<uint64_t> data;

        BitmaskGrid(int w, int h, int l, PaletteID pid)
            : paletteId(pid), width(w), height(h), length(l)
        {
            xSegments = (w + BITS_PER_SEG - 1) / BITS_PER_SEG;
            data.resize(size_t(h) * l * xSegments, 0);
        }

        inline void set(Coord x, Coord y, Coord z) {
            size_t idx = (size_t(y) * length + z) * xSegments + (x / BITS_PER_SEG);
            data[idx] |= (1ULL << (x % BITS_PER_SEG));
        }

        inline bool test(Coord x, Coord y, Coord z) const {
            size_t idx = (size_t(y) * length + z) * xSegments + (x / BITS_PER_SEG);
            return (data[idx] >> (x % BITS_PER_SEG)) & 1ULL;
        }

        inline void clear(Coord x, Coord y, Coord z) {
            size_t idx = (size_t(y) * length + z) * xSegments + (x / BITS_PER_SEG);
            data[idx] &= ~(1ULL << (x % BITS_PER_SEG));
        }
    };

    static std::vector<BlockRegion> mergeToRegions(
        const std::vector<BlockGroup>& groups,
        int width, int height, int length)
    {
        std::unordered_map<PaletteID, BitmaskGrid> grids;
        grids.reserve(groups.size());

        // Step 1: 构建 palette → bitmask 映射
        for (const auto& bg : groups) {
            auto& grid = grids.try_emplace(bg.paletteId, width, height, length, bg.paletteId).first->second;
            for (size_t i = 0; i < bg.count; i++)
                grid.set(bg.x[i], bg.y[i], bg.z[i]);
        }

        // Step 2: 多线程并行合并
        std::vector<BlockRegion> allRegions;
        std::mutex resultMutex;
        std::vector<std::future<void>> tasks;

        for (auto& [paletteId, grid] : grids) {
            tasks.push_back(std::async(std::launch::async, [&grid, &allRegions, &resultMutex]() {
                std::vector<BlockRegion> local;
                local.reserve(1024);
                mergeSingleGrid(grid, local);
                std::scoped_lock lock(resultMutex);
                allRegions.insert(allRegions.end(), local.begin(), local.end());
            }));
        }

        for (auto& f : tasks) f.get();
        return allRegions;
    }

private:
    static void mergeSingleGrid(BitmaskGrid& grid, std::vector<BlockRegion>& regions)
    {
        const int W = grid.width;
        const int H = grid.height;
        const int L = grid.length;

        for (int y = 0; y < H; y++) {
            for (int z = 0; z < L; z++) {
                for (int seg = 0; seg < grid.xSegments; seg++) {
                    uint64_t bits = grid.data[(size_t(y) * L + z) * grid.xSegments + seg];
                    while (bits) {
                        int bitIndex = std::countr_zero(bits);
                        bits &= bits - 1; // 清除最低位的1
                        Coord x = seg * BITS_PER_SEG + bitIndex;
                        if (x >= W || !grid.test(x, y, z)) continue;

                        // X方向扩展
                        Coord x2 = x;
                        while (x2 + 1 < W && grid.test(x2 + 1, y, z)) x2++;

                        // Z方向扩展
                        Coord z2 = z;
                        bool canExpandZ = true;
                        while (canExpandZ && z2 + 1 < L) {
                            for (Coord cx = x; cx <= x2; cx++) {
                                if (!grid.test(cx, y, z2 + 1)) {
                                    canExpandZ = false;
                                    break;
                                }
                            }
                            if (canExpandZ) z2++;
                        }

                        // Y方向扩展
                        Coord y2 = y;
                        bool canExpandY = true;
                        while (canExpandY && y2 + 1 < H) {
                            for (Coord cz = z; cz <= z2; cz++) {
                                for (Coord cx = x; cx <= x2; cx++) {
                                    if (!grid.test(cx, y2 + 1, cz)) {
                                        canExpandY = false;
                                        goto stopY;
                                    }
                                }
                            }
                            y2++;
                        }
                    stopY:;

                        // 添加 region
                        BlockRegion region;
                        region.paletteId = grid.paletteId;
                        region.x1 = x; region.x2 = x2;
                        region.y1 = y; region.y2 = y2;
                        region.z1 = z; region.z2 = z2;
                        regions.push_back(region);

                        // 清除已处理区域
                        for (Coord cy = y; cy <= y2; cy++)
                            for (Coord cz = z; cz <= z2; cz++)
                                for (Coord cx = x; cx <= x2; cx++)
                                    grid.clear(cx, cy, cz);
                    }
                }
            }
        }
    }
};
