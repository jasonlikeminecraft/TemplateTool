#include <iostream>  
#include "BCFCachedWriter.hpp"  
#include "BCFStreamReader.hpp"  
#include "McstructureToBCF.hpp"
#include "SchematicToBCF.hpp"
#include "LitematicToBCF.hpp"
#include "SchemToBCF.hpp"
#include "McfunctionToBCF.hpp"
#include <nbt_tags.h>
#include <io/stream_reader.h>
#include <io/izlibstream.h>
#include <io/ozlibstream.h>
#include <fstream>  

int main() {
    try {
        while (true) {
            std::cout << "请输入转换类型:" << std::endl;
            std::cout << "1.Mcstructure\n2.Schematic\n3.Litematic\n4.Schem\n5.Mcfunction\n";
            int type;
            std::cin >> type;
            std::cout << "请输入输入文件路径:" << std::endl;
            std::string inputFileName;
            std::cin >> inputFileName;

            switch (type) {
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
                break;
            }
            }


            BCFStreamReader reader("output.bcf");

            //// 获取子区块总数  
            size_t totalSubChunks = reader.getSubChunkCount();
            std::cout << "Total sub-chunks: " << totalSubChunks << std::endl;

            //// 按需读取指定的子区块  
            for (size_t i = 0; i < totalSubChunks; i++) {
                std::vector<BlockRegion> blocks = reader.getBlockRegions(i);

                std::cout << "Sub-chunk " << i << " has " << blocks.size() << " regions\n";
                auto origin = reader.getSubChunkOrigin(i); // 获取子区块的原点
                std::cout << i << " origin: " << origin.originX << ", " << origin.originY << ", " << origin.originZ << std::endl;
                //    // 遍历子区块中的所有方块
                //    for (auto& block : blocks) {
                //        const PaletteKey& pk = reader.getPaletteKey(block.paletteId);
                //        std::string blockType = reader.getBlockType(pk.typeId);
                //        std::cout << "Block type: " << blockType << " (palette id: " << block.paletteId << ")\n";

                //    }
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