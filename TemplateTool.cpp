#include <iostream>  
#include "BCFCachedWriter.hpp"  
#include "BCFStreamReader.hpp"  
#include "McstructureToBCF.hpp"
#include "SchematicToBCF.hpp"
#include <nbt_tags.h>
#include <io/stream_reader.h>
#include <io/izlibstream.h>
#include <io/ozlibstream.h>
#include <fstream>  

int main() {


   SchematicToBCF schematicToBCF("input.schematic", "output.bcf");



    //BCFStreamReader reader("output.bcf");

    //// 获取子区块总数  
    //size_t totalSubChunks = reader.getSubChunkCount();
    //std::cout << "Total sub-chunks: " << totalSubChunks << std::endl;

    //// 按需读取指定的子区块  
    //for (size_t i = 0; i < totalSubChunks; i++) {
    //    std::vector<BlockRegion> blocks = reader.getBlockRegions(i);

    //    std::cout << "Sub-chunk " << i << " has " << blocks.size() << " regions\n";
    //    auto origin = reader.getSubChunkOrigin(i); // 获取子区块的原点
    //    std::cout << "Sub-chunk " << i << " origin: " << origin.originX << ", " << origin.originY << ", " << origin.originZ << std::endl;
    //    // 遍历子区块中的所有方块
    //    for (auto& block : blocks) {
    //        const PaletteKey& pk = reader.getPaletteKey(block.paletteId);
    //        std::string blockType = reader.getBlockType(pk.typeId);
    //        std::cout << "Block type: " << blockType << " (palette id: " << block.paletteId << ")\n";

    //    }
    //}
    std::cin.get();
    return 0;
}