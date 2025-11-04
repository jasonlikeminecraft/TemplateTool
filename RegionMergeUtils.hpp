#pragma once
#include "bcf_structs.hpp"
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <vector>
#include <algorithm>
#include <thread>
#include <future>
#include <array>
#include <cmath>

struct RegionMergeUtils {
    using EncodedKey = uint64_t;

    struct CoordRange {
        int xMin, xMax;
        int yMin, yMax;
        int zMin, zMax;

        int xBits, yBits, zBits;
        uint64_t xMask, yMask, zMask;

        void init(int xm, int xM, int ym, int yM, int zm, int zM) {
            xMin = xm; xMax = xM;
            yMin = ym; yMax = yM;
            zMin = zm; zMax = zM;

            xBits = std::ceil(std::log2(xMax - xMin + 1));
            yBits = std::ceil(std::log2(yMax - yMin + 1));
            zBits = std::ceil(std::log2(zMax - zMin + 1));

            xMask = ((uint64_t)1 << xBits) - 1;
            yMask = ((uint64_t)1 << yBits) - 1;
            zMask = ((uint64_t)1 << zBits) - 1;
        }

        inline EncodedKey encode(Coord x, Coord y, Coord z) const noexcept {
            return ((uint64_t)(x - xMin) & xMask) << (yBits + zBits) |
                   ((uint64_t)(y - yMin) & yMask) << zBits |
                   ((uint64_t)(z - zMin) & zMask);
        }

        inline std::tuple<Coord, Coord, Coord> decode(EncodedKey key) const noexcept {
            Coord z = key & zMask;
            Coord y = (key >> zBits) & yMask;
            Coord x = (key >> (yBits + zBits)) & xMask;
            return { x + xMin, y + yMin, z + zMin };
        }
    };

    static std::vector<BlockRegion> mergeToRegions(
        const std::vector<BlockGroup>& groups)
    {
        std::vector<BlockRegion> regions;
        std::unordered_map<PaletteID, std::unordered_set<EncodedKey>> grids;
        grids.reserve(groups.size());

        // 先扫描坐标范围
        CoordRange range{ INT_MAX, INT_MIN, INT_MAX, INT_MIN, INT_MAX, INT_MIN };
        for (auto& bg : groups) {
            for (size_t i = 0; i < bg.count; i++) {
                range.xMin = std::min(range.xMin, bg.x[i]);
                range.xMax = std::max(range.xMax, bg.x[i]);
                range.yMin = std::min(range.yMin, bg.y[i]);
                range.yMax = std::max(range.yMax, bg.y[i]);
                range.zMin = std::min(range.zMin, bg.z[i]);
                range.zMax = std::max(range.zMax, bg.z[i]);
            }
        }
        range.init(range.xMin, range.xMax, range.yMin, range.yMax, range.zMin, range.zMax);

        // 构建整数哈希集合
        for (const auto& bg : groups) {
            auto& grid = grids[bg.paletteId];
            grid.reserve(bg.count);
            for (size_t i = 0; i < bg.count; i++)
                grid.insert(range.encode(bg.x[i], bg.y[i], bg.z[i]));
        }

        // 6 种扩展顺序
        static const std::array<std::array<int, 3>, 6> orders = {{
            {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
        }};

        for (auto& [paletteId, grid] : grids) {
            while (!grid.empty()) {
                auto it = grid.begin();
                auto [x, y, z] = range.decode(*it);

                BlockRegion best;

                if (grid.size() > 5000) {
                    std::array<std::future<BlockRegion>, 6> futures;
                    for (int i = 0; i < 6; i++)
                        futures[i] = std::async(std::launch::async,
                            [&grid, x, y, z, paletteId, &orders, &range, i]() {
                                return tryExpansion(grid, x, y, z, paletteId, orders[i], range);
                            });

                    int maxVolume = 0;
                    for (int i = 0; i < 6; i++) {
                        BlockRegion candidate = futures[i].get();
                        int vol = (candidate.x2 - candidate.x1 + 1) *
                                  (candidate.y2 - candidate.y1 + 1) *
                                  (candidate.z2 - candidate.z1 + 1);
                        if (vol > maxVolume) { maxVolume = vol; best = candidate; }
                    }
                } else {
                    int maxVolume = 0;
                    for (int i = 0; i < 6; i++) {
                        BlockRegion candidate = tryExpansion(grid, x, y, z, paletteId, orders[i], range);
                        int vol = (candidate.x2 - candidate.x1 + 1) *
                                  (candidate.y2 - candidate.y1 + 1) *
                                  (candidate.z2 - candidate.z1 + 1);
                        if (vol > maxVolume) { maxVolume = vol; best = candidate; }
                    }
                }

                regions.push_back(best);
                removeRegion(grid, best, range);
            }
        }

        return regions;
    }

private:
    static bool has(const std::unordered_set<EncodedKey>& grid, Coord x, Coord y, Coord z, const CoordRange& range) noexcept {
        return grid.count(range.encode(x, y, z));
    }

    static BlockRegion tryExpansion(
        const std::unordered_set<EncodedKey>& grid,
        Coord x, Coord y, Coord z,
        PaletteID pid,
        const std::array<int,3>& order,
        const CoordRange& range)
    {
        BlockRegion r{pid,x,x,y,y,z,z};
        for (int axis : order) {
            if (axis==0) expandX(grid,r,range);
            else if(axis==1) expandZ(grid,r,range);
            else expandY(grid,r,range);
        }
        return r;
    }

    static void expandX(const std::unordered_set<EncodedKey>& grid, BlockRegion& r, const CoordRange& range) {
        while(true){
            Coord nx = r.x2+1;
            for(Coord cy=r.y1;cy<=r.y2;cy++) for(Coord cz=r.z1;cz<=r.z2;cz++)
                if(!has(grid,nx,cy,cz,range)) return;
            r.x2 = nx;
        }
    }

    static void expandZ(const std::unordered_set<EncodedKey>& grid, BlockRegion& r, const CoordRange& range){
        while(true){
            Coord nz = r.z2+1;
            for(Coord cx=r.x1;cx<=r.x2;cx++) for(Coord cy=r.y1;cy<=r.y2;cy++)
                if(!has(grid,cx,cy,nz,range)) return;
            r.z2 = nz;
        }
    }

    static void expandY(const std::unordered_set<EncodedKey>& grid, BlockRegion& r, const CoordRange& range){
        while(true){
            Coord ny = r.y2+1;
            for(Coord cx=r.x1;cx<=r.x2;cx++) for(Coord cz=r.z1;cz<=r.z2;cz++)
                if(!has(grid,cx,ny,cz,range)) return;
            r.y2 = ny;
        }
    }

static void removeRegion(std::unordered_set<EncodedKey>& grid, const BlockRegion& r, const CoordRange& range){
    size_t toRemove = (r.x2-r.x1+1)*(r.y2-r.y1+1)*(r.z2-r.z1+1);
    if(toRemove > grid.size()/4){ // 超大块时并行删除
        std::vector<EncodedKey> keysToRemove;
        keysToRemove.reserve(toRemove);
        for(Coord cy=r.y1; cy<=r.y2; cy++)
            for(Coord cx=r.x1; cx<=r.x2; cx++)
                for(Coord cz=r.z1; cz<=r.z2; cz++)
                    keysToRemove.push_back(range.encode(cx,cy,cz));

        size_t numThreads = std::thread::hardware_concurrency();
        if(numThreads==0) numThreads=4;
        size_t chunkSize = (keysToRemove.size() + numThreads - 1) / numThreads;

        std::vector<std::future<void>> futures;
        for(size_t t=0; t<numThreads; t++){
            size_t start = t*chunkSize;
            size_t end = std::min(start+chunkSize, keysToRemove.size());
            if(start>=end) break;
            futures.push_back(std::async(std::launch::async, [&grid, &keysToRemove, start, end](){
                for(size_t i=start; i<end; i++)
                    grid.erase(keysToRemove[i]);
            }));
        }

        for(auto& f : futures) f.get();
    } else if(toRemove > grid.size()/2){
        // 删除大半表，重建哈希表更高效
        std::unordered_set<EncodedKey> newGrid;
        newGrid.reserve(grid.size() - toRemove);
        for(auto key:grid){
            auto [x,y,z]=range.decode(key);
            if(x<r.x1||x>r.x2||y<r.y1||y>r.y2||z<r.z1||z>r.z2)
                newGrid.insert(key);
        }
        grid.swap(newGrid);
    } else {
        // 小块，直接删除
        for(Coord cy=r.y1; cy<=r.y2; cy++)
            for(Coord cx=r.x1; cx<=r.x2; cx++)
                for(Coord cz=r.z1; cz<=r.z2; cz++)
                    grid.erase(range.encode(cx,cy,cz));
    }
}
