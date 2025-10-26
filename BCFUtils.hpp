#pragma once
#include "bcf_structs.hpp"
#include "bcf_io.hpp"
#include "SubChunkUtils.hpp"
#include <vector>
#include <map>
#include <fstream>
struct BCFUtils {

    // 写整个 BCF 文件
    static void writeBCF(const std::string& path, const std::vector<std::vector<BlockGroup>>& subChunks,
        const std::vector<PaletteKey>& paletteList,
        const std::map<BlockTypeID, std::string>& typeMap,
        const std::map<BlockStateID, std::string>& stateMap)
    {
        std::ofstream ofs(path, std::ios::binary);
        BCFHeader header;
        write_le<BCFHeader>(ofs, header);

        for (size_t i = 0; i < subChunks.size(); i++) {
            Coord originY = static_cast<Coord>(i * 16);
            SubChunkUtils::writeSubChunk(ofs, subChunks[i], originY);
        }

        // 写 Palette 映射
        header.paletteOffset = ofs.tellp();
        write_u32(ofs, paletteList.size());
        for (size_t pid = 0; pid < paletteList.size(); pid++) {
            const auto& k = paletteList[pid];
            write_u32(ofs, static_cast<uint32_t>(pid));
            write_u16(ofs, k.typeId);
            write_u16(ofs, k.states.size());
            for (auto& s : k.states) { write_u8(ofs, s.first); write_u8(ofs, s.second); }
        }

        // 写方块类型映射
        header.blockTypeMapOffset = ofs.tellp();
        write_u32(ofs, typeMap.size());
        for (auto& kv : typeMap) { write_u16(ofs, kv.first); writeString16(ofs, kv.second); }

        // 写状态映射
        header.stateNameMapOffset = ofs.tellp();
        write_u32(ofs, stateMap.size());
        for (auto& kv : stateMap) { write_u8(ofs, kv.first); writeString16(ofs, kv.second); }
        header.subChunkCount = subChunks.size();
        ofs.seekp(0); write_le<BCFHeader>(ofs, header);

        ofs.close();
    }

    // 从文件读取子区块
    static std::vector<std::vector<BlockGroup>> readAllSubChunks(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        std::string line;
        char byte;

        if (!ifs) { std::cerr << "Failed to open file\n"; return {}; }
        //while (ifs.get(byte)) {
        //    // 打印字节的十六进制表示
        //    std::cout << std::hex << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
        //}
        BCFHeader header; 
        read_le<BCFHeader>(ifs, header);
        std::vector<std::vector<BlockGroup>> subChunks;
        subChunks.reserve(header.subChunkCount);
        for (size_t i = 0; i < header.subChunkCount; i++) {
            SubChunkSize sz; Coord oy;
            subChunks.push_back(SubChunkUtils::readSubChunk(ifs, sz, oy));
        }
        return subChunks;
    }

    std::vector<PaletteKey> readPalette(const std::string& path) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) { std::cerr << "Failed to open file\n"; return {}; }

        // 先读取文件头
        BCFHeader header;
        read_le<BCFHeader>(ifs, header);

        // 跳转到 palette 偏移
        ifs.seekg(header.paletteOffset, std::ios::beg);

        uint32_t paletteCount = read_u32(ifs);
        std::vector<PaletteKey> paletteList;
        paletteList.reserve(paletteCount);

        for (uint32_t i = 0; i < paletteCount; ++i) {
            uint32_t pid = read_u32(ifs);
            BlockTypeID typeId = read_u16(ifs);
            uint16_t stateCount = read_u16(ifs);

            PaletteKey pk;
            pk.typeId = typeId;
            pk.states.reserve(stateCount);
            for (uint16_t j = 0; j < stateCount; j++) {
                BlockStateID stateName = read_u8(ifs);
                BlockStateID stateValue = read_u8(ifs);
                pk.states.push_back({ stateName,stateValue });
            }

            paletteList.push_back(pk);
        }

        return paletteList;
    }
};
