#include <iostream>
#include <chrono>  // 添加计时功能的头文件
#include "BCFCachedWriter.hpp"
#include "BCFStreamReader.hpp"
#include "McstructureToBCF.hpp"
#include "SchematicToBCF.hpp"
#include "LitematicToBCF.hpp"
#include "SchemToBCF.hpp"
#include "McfunctionToBCF.hpp"
#include "BCFBlockMerger.hpp"
#include <nbt_tags.h>
#include <io/stream_reader.h>
#include <io/izlibstream.h>
#include <io/ozlibstream.h>
#include <fstream>

int main() {
    try {
        while (true) {
            std::cout << "请输入转换类型:" << std::endl;
            std::cout << "0.合并优化\n1.Mcstructure\n2.Schematic\n3.Litematic\n4.Schem\n5.Mcfunction\n";
            int type;
            std::cin >> type;
            std::cout << "请输入输入文件路径:" << std::endl;
            std::string inputFileName;
            std::cin >> inputFileName;

            auto start = std::chrono::high_resolution_clock::now();  // 记录开始时间

            switch (type) {
            case 0:
            {
                BCFBlockMerger merger(inputFileName);
                break;
            }
            case 1:
            {
                McstructureToBCF mcstructureToBCF(inputFileName, "output.bcf");
                break;
            }
            case 2:
            {
                SchematicToBCF schematicToBCF(inputFileName, "output.bcf");
                break;
            }
            case 3:
            {
                LitematicToBCF litematicToBCF(inputFileName, "output.bcf");
                break;
            }
            case 4:
            {
                SchemToBCF schematicToBCF(inputFileName, "output.bcf");
                break;
            }
            case 5:
            {
                McfunctionToBCF mcfunctionToBCF(inputFileName, "output.bcf");
                break;
            }
            default:
            {
                std::cout << "无效的输入类型" << std::endl;
                continue;  // 无效类型时继续循环，避免计算转换时间
            }
            }

            auto end = std::chrono::high_resolution_clock::now();  // 记录结束时间
            std::chrono::duration<double> elapsed = end - start;  // 计算转换所用时间

            std::cout << "转换时间: " << elapsed.count() << " 秒" << std::endl;

            BCFStreamReader reader("output.bcf");

            //// 获取子区块总数  
            size_t totalSubChunks = reader.getSubChunkCount();
            std::cout << "Total sub-chunks: " << totalSubChunks << std::endl;

            //// 按需读取指定的子区块  
            for (size_t i = 0; i < totalSubChunks; i++) {
                std::vector<BlockRegion> blocks = reader.getBlockRegions(i);

                std::cout << "Sub-chunk " << i << " has " << blocks.size() << " regions\n";
                auto origin = reader.getSubChunkOrigin(i);
                std::cout << i << " origin: " << origin.originX << ", " << origin.originY << ", " << origin.originZ << std::endl;

                // 遍历子区块中的所有方块区域  
                for (auto& block : blocks) {
                    const PaletteKey& pk = reader.getPaletteKey(block.paletteId);
                    std::string blockType = reader.getBlockType(pk.typeId);
                    std::cout << "Block type: " << blockType << " (palette id: " << block.paletteId << ")\n";

                    // 读取 NBT 数据  
                    auto nbtData = reader.getBlockNBTData(block.paletteId);
                    if (nbtData) {
                        std::cout << "  Has NBT data\n";
                        // 访问 NBT 数据  
                        // 例如: if (nbtData->has_key("Items")) { ... }  
                    }
                }
            }
            //  std::cin.get();
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 0;
    }
    return 0;
}
