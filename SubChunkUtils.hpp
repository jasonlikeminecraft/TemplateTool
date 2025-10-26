#pragma once
#include "BlockUtils.hpp"


struct SubChunkUtils {

    // 写入子区块
    static void writeSubChunk(std::ofstream& ofs, const std::vector<BlockGroup>& groups, Coord originY) {
        std::streampos startPos = ofs.tellp();
        write_u64(ofs, 0); // 占位 subChunkSize
        write_i16(ofs, originY);
        write_u32(ofs, groups.size());

        for (const auto& bg : groups)
            BlockUtils::writeBlockGroup(ofs, bg);

        // 填写 subChunkSize
        std::streampos endPos = ofs.tellp();
        ofs.seekp(startPos);
        write_u64(ofs, static_cast<SubChunkSize>(endPos - startPos));
        ofs.seekp(endPos);
    }

    // 从文件读取子区块
    static std::vector<BlockGroup> readSubChunk(std::ifstream& ifs, SubChunkSize& subChunkSize, Coord& originY) {
        subChunkSize = read_u64(ifs);
        originY = read_i16(ifs);
        BlockCount bgc = read_u32(ifs);
        std::vector<BlockGroup> groups;
        groups.reserve(bgc);
        for (size_t i = 0; i < bgc; i++) {
            groups.push_back(BlockUtils::readBlockGroup(ifs));
        }
        return groups;
    }
};
