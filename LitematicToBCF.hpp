#pragma once
#include "BCFCachedWriter.hpp"
#include "BlockStateConverter.hpp"
#include <nbt_tags.h>
#include <io/stream_reader.h>
#include <io/izlibstream.h>
#include <cmath>
#include <vector>
#include <string>
#include <iostream>

class LitematicToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;
    BlockStateConverter m_converter;

public:
    LitematicToBCF(const std::string& filename, const std::string& outputFilename,
        const std::string& conversionTableFile = "D:\\Projects\\TemplateTool\\snbt_convert.txt")
        : m_filename(filename), m_outputFilename(outputFilename) {

        if (!conversionTableFile.empty()) {
            if (!m_converter.loadFromFile(conversionTableFile)) {
                std::cerr << "Error: Failed to load conversion table file.\n";
            }
        }

        convert();
    }

    void convert() {
        // 1. 读取 GZIP 压缩的 NBT
        std::ifstream file(m_filename, std::ios::binary);
        if (!file) {
            std::cerr << "无法打开文件: " << m_filename << std::endl;
            return;
        }
        zlib::izlibstream gzFile(file);
        auto [rootName, nbtRoot] = nbt::io::read_compound(gzFile);

        // 2. 读取 Metadata 信息
        if (!nbtRoot->has_key("Metadata") || !nbtRoot->has_key("Regions")) {
            std::cerr << "错误: 文件不是有效的 Litematic (缺少 Metadata 或 Regions)" << std::endl;
            return;
        }

        const auto& meta = nbtRoot->at("Metadata").as<nbt::tag_compound>();

        int lm_version = static_cast<int>(nbtRoot->at("Version"));
        int lm_subversion = nbtRoot->has_key("SubVersion")
            ? static_cast<int>(nbtRoot->at("SubVersion"))
            : 0;
        int mc_version = static_cast<int>(nbtRoot->at("MinecraftDataVersion"));

        // 读取尺寸
        const auto& encSize = meta.at("EnclosingSize").as<nbt::tag_compound>();
        int width = static_cast<int>(encSize.at("x"));
        int height = static_cast<int>(encSize.at("y"));
        int length = static_cast<int>(encSize.at("z"));

        std::string author = static_cast<std::string>(meta.at("Author"));
        std::string name = static_cast<std::string>(meta.at("Name"));
        std::string desc = static_cast<std::string>(meta.at("Description"));

        // 创建 BCFCachedWriter
        BCFCachedWriter writer(m_outputFilename, "./temp_bcf_cache", 50000);

        // 3. 读取 Regions
        auto& regions = nbtRoot->at("Regions").as<nbt::tag_compound>();
        int regionCount = static_cast<int>(regions.size());

        for (const auto& [regionName, regionTag] : regions) {
            processRegion(regionTag.as<nbt::tag_compound>(), writer);
        }

        // 校验 RegionCount
        if (meta.has_key("RegionCount")) {
            int expectedCount = static_cast<int>(meta.at("RegionCount"));
            if (expectedCount != regionCount) {
                std::cerr << "警告: RegionCount 不匹配, 预期 " << expectedCount
                    << " 实际 " << regionCount << std::endl;
            }
        }

        // 校验尺寸
        if (meta.has_key("EnclosingSize")) {
            int calcW = 0, calcH = 0, calcL = 0;
            for (const auto& [regionName, regionTag] : regions) {
                const auto& region = regionTag.as<nbt::tag_compound>();
                const auto& size = region.at("Size").as<nbt::tag_compound>();
                const auto& pos = region.at("Position").as<nbt::tag_compound>();
                calcW = std::max(calcW, static_cast<int>(pos.at("x")) + static_cast<int>(size.at("x")));
                calcH = std::max(calcH, static_cast<int>(pos.at("y")) + static_cast<int>(size.at("y")));
                calcL = std::max(calcL, static_cast<int>(pos.at("z")) + static_cast<int>(size.at("z")));
            }

            if (calcW != width || calcH != height || calcL != length) {
                std::cerr << "错误: Metadata 尺寸与实际区域尺寸不匹配\n"
                    << "  Metadata: (" << width << ", " << height << ", " << length << ")\n"
                    << "  实际计算: (" << calcW << ", " << calcH << ", " << calcL << ")\n";
            }
        }

        // 设置时间信息（非必要，仅打印）
        if (meta.has_key("TimeCreated"))
            std::cout << "TimeCreated: " << static_cast<int64_t>(meta.at("TimeCreated")) << "\n";
        if (meta.has_key("TimeModified"))
            std::cout << "TimeModified: " << static_cast<int64_t>(meta.at("TimeModified")) << "\n";

        if (meta.has_key("PreviewImageData"))
            std::cout << "包含预览图像数据 (忽略)" << std::endl;

        // 4. 完成写入
        writer.finalize();

        std::cout << "✅ Litematic 转换完成: " << name << " by " << author << std::endl;
    }

private:
    // region 解码逻辑与原代码一致
    void processRegion(const nbt::tag_compound& region, BCFCachedWriter& writer) {
        const auto& size = region.at("Size").as<nbt::tag_compound>();
        int sizeX = static_cast<int>(size.at("x").as<nbt::tag_int>());
        int sizeY = static_cast<int>(size.at("y").as<nbt::tag_int>());
        int sizeZ = static_cast<int>(size.at("z").as<nbt::tag_int>());



        const auto& position = region.at("Position").as<nbt::tag_compound>();
        int posX = static_cast<int>(position.at("x").as<nbt::tag_int>());
        int posY = static_cast<int>(position.at("y").as<nbt::tag_int>());
        int posZ = static_cast<int>(position.at("z").as<nbt::tag_int>());
        // ✅ 修正负尺寸
        if (sizeX < 0) { posX += sizeX; sizeX = -sizeX; }
        if (sizeY < 0) { posY += sizeY; sizeY = -sizeY; }
        if (sizeZ < 0) { posZ += sizeZ; sizeZ = -sizeZ; }
        const auto& palette = region.at("BlockStatePalette").as<nbt::tag_list>();
        int paletteSize = static_cast<int>(palette.size());
        std::vector<bool> isAirPalette = buildAirFilter(palette);

        const auto& blockStates = region.at("BlockStates").as<nbt::tag_long_array>();
        int bitsPerBlock = paletteSize > 1 ? static_cast<int>(std::ceil(std::log2(paletteSize))) : 1;
        int blocksPerLong = 64 / bitsPerBlock;
        int maxDecodableBlocks = static_cast<int>(blockStates.size()) * blocksPerLong;
        int totalBlocks = sizeX * sizeY * sizeZ;
        int actualBlocks = std::min(totalBlocks, maxDecodableBlocks);

        std::vector<int> blockIndices = decodeBlockStates(blockStates, actualBlocks, bitsPerBlock);
        if (blockIndices.size() < static_cast<size_t>(totalBlocks))
            blockIndices.resize(totalBlocks, 0);
        std::cout << "[DEBUG] Region pos=(" << posX << "," << posY << "," << posZ << ") size=(" << sizeX << "," << sizeY << "," << sizeZ << ")\n";
        int count = 0;
        for (int y = 0; y < sizeY; ++y) {
            for (int z = 0; z < sizeZ; ++z) {
                for (int x = 0; x < sizeX; ++x) {
                    // Litemapy index 计算方式
                    int index = x + (z * sizeX) + (y * sizeX * sizeZ);
                    int paletteIndex = blockIndices[index];

                    // 在满足条件之前打印前10个有效点
                    if (count < 10) {
                        std::cout << "x: " << x << ", y: " << y << ", z: " << z << ", paletteIndex: " << paletteIndex << std::endl;
                        count++;
                    }

                    if (paletteIndex < 0 || paletteIndex >= paletteSize || isAirPalette[paletteIndex])
                        continue;

                    auto& block = palette.at(paletteIndex).as<nbt::tag_compound>();
                    std::string blockName = static_cast<std::string>(block.at("Name"));

                    std::vector<std::pair<std::string, std::string>> statesVec;
                    if (block.has_key("Properties")) {
                        statesVec = extractBlockStates(block.at("Properties").as<nbt::tag_compound>());
                    }

                    auto [beBlockName, beStates] = m_converter.convert(blockName, statesVec);

                    writer.addBlock(posX + x, posY + y, posZ + z, beBlockName, beStates);
                }
            }
        }
    }

    std::vector<bool> buildAirFilter(const nbt::tag_list& palette) {
        std::vector<bool> isAir(palette.size(), false);
        for (size_t i = 0; i < palette.size(); i++) {
            auto& block = palette.at(i).as<nbt::tag_compound>();
            std::string name = static_cast<std::string>(block.at("Name"));
            if (name.find("air") != std::string::npos)
                isAir[i] = true;
        }
        return isAir;
    }


    std::vector<int> decodeBlockStates(
        const nbt::tag_long_array& blockStates,
        int totalBlocks,
        int bitsPerBlock
    ) {
        std::vector<int> indices;
        indices.reserve(totalBlocks);

        if (bitsPerBlock <= 0) bitsPerBlock = 1;
        const uint64_t mask = (1ULL << bitsPerBlock) - 1ULL;
        const size_t longsCount = blockStates.size();

        for (int i = 0; i < totalBlocks; ++i) {
            uint64_t bitIndex = static_cast<uint64_t>(i) * bitsPerBlock;
            size_t longIndex = bitIndex / 64;
            uint32_t bitOffset = bitIndex % 64;

            if (longIndex >= longsCount) {
                indices.push_back(0);
                continue;
            }

            // 注意：不要 byteswap，也不要 bit reverse
            uint64_t value = static_cast<uint64_t>(blockStates[longIndex]) >> bitOffset;

            // 跨 long 拼接
            if (bitOffset + bitsPerBlock > 64 && longIndex + 1 < longsCount) {
                uint64_t next = static_cast<uint64_t>(blockStates[longIndex + 1]);
                value |= (next << (64 - bitOffset));
            }

            indices.push_back(static_cast<int>(value & mask));
        }

        return indices;
    }
    std::vector<std::pair<std::string, std::string>> extractBlockStates(const nbt::tag_compound& properties) {
        std::vector<std::pair<std::string, std::string>> states;
        states.reserve(properties.size());
        for (const auto& [propName, propValue] : properties) {
            std::string valueStr = static_cast<std::string>(propValue);
            if (propName != "waterlogged")
                states.emplace_back(propName, std::move(valueStr));
        }
        return states;
    }
};
