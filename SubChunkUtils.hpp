#pragma once
#include "BlockUtils.hpp"


struct SubChunkUtils {

    // 写入子区块
    static void writeSubChunk(std::ofstream& ofs,
        const std::vector<BlockRegion>& regions,
        Coord originY) {
        std::streampos startPos = ofs.tellp();
        write_u64(ofs, 0); // 占位 subChunkSize  
        write_i16(ofs, originY);
        write_u32(ofs, regions.size());

        for (const auto& region : regions) {
            write_u32(ofs, region.paletteId);
            write_i16(ofs, region.x1);
            write_i16(ofs, region.y1);
            write_i16(ofs, region.z1);
            write_i16(ofs, region.x2);
            write_i16(ofs, region.y2);
            write_i16(ofs, region.z2);
        }

        // 回写 subChunkSize  
        std::streampos endPos = ofs.tellp();
        ofs.seekp(startPos);
        write_u64(ofs, static_cast<SubChunkSize>(endPos - startPos));
        ofs.seekp(endPos);
    }
    // 从文件读取子区块
    static std::vector<BlockRegion> readSubChunk(std::ifstream& ifs,
        SubChunkSize& subChunkSize,
        Coord& originY) {
        subChunkSize = read_u64(ifs);
        originY = read_i16(ifs);
        BlockCount regionCount = read_u32(ifs);

        std::vector<BlockRegion> regions;
        regions.reserve(regionCount);

        for (size_t i = 0; i < regionCount; i++) {
            BlockRegion region;
            region.paletteId = read_u32(ifs);
            region.x1 = read_i16(ifs);
            region.y1 = read_i16(ifs);
            region.z1 = read_i16(ifs);
            region.x2 = read_i16(ifs);
            region.y2 = read_i16(ifs);
            region.z2 = read_i16(ifs);
            regions.push_back(region);
        }

        return regions;
    }
    static PaletteID getPaletteId(
        const std::vector<BlockGroup>& groups,
        int x, int y, int z)
    {
        for (const auto& group : groups) {
            for (size_t i = 0; i < group.count; i++) {
                if (group.x[i] == x &&
                    group.y[i] == y &&
                    group.z[i] == z) {
                    return group.paletteId;
                }
            }
        }

        return static_cast<PaletteID>(-1); // 未找到, 空气 or 未存储
    }
};
