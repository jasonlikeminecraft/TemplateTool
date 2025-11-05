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

    //    std::cout << "Sub-chunk " << i << " has " << blocks.size() << " regions\n" ;

    //    // 处理每个区域  
    //    for (const auto& region : blocks) {
    //        // 通过 paletteId 获取 PaletteKey  
    //        const PaletteKey& pk = reader.getPaletteKey(region.paletteId);

    //        // 获取方块类型字符串  
    //        std::string blockType = reader.getBlockType(pk.typeId);

    //        std::cout << "Region at (" << region.x1 << ", " << region.y1 << ", " << region.z1
    //            << ") to (" << region.x2 << ", " << region.y2 << ", " << region.z2 << "): "
    //            << blockType << std::endl;

          // 访问方块状态    
            //if (!pk.states.empty()) {
            //    std::cout << "  States:" << std::endl;
            //    for (const auto& [stateId, stateValueId] : pk.states) {
            //        std::string stateName = reader.getStateName(stateId);
            //        std::string stateValue = reader.getStateValue(stateValueId);  // 新增方法  
            //        std::cout << "    " << stateName << ": " << stateValue << std::endl;
            //    }
            //}

       // }
  //  }


    //McstructureToBCF mcstructureToBCF("input.mcstructure" ,"output.bcf");
    //mcstructureToBCF.convert();

    //// 创建 BCFCachedWriter 实例  
    //// 参数: 输出文件名, 临时目录, 内存中最大方块数  
    //BCFCachedWriter writer("output.bcf", "./temp_bcf_cache", 5000);

    //// 添加方块 - 自动管理内存和临时文件  
    //// 参数: x, y, z, 方块类型, 状态列表  
    ////addBlock(int x, int y, int z, const std::string& blockType, const std::vector<std::pair<std::string, std::string>>& states)
    //writer.addBlock(0, 0, 0, "minecraft:stone", {});
    //writer.addBlock(1, 0, 0, "minecraft:stone", {});
    //writer.addBlock(2, 1, 1, "minecraft:grass", { {"snowy", "True"}});

    //int blocCount = 0;
    //// 可以添加大量方块,不用担心内存溢出  
    //for (int x = 0; x < 100; x++) {
    //    for (int y = -56; y < 320; y++) {
    //        for (int z = 0; z < 100; z++) {
    //            writer.addBlock(x, y, z, "minecraft:dirt", { {"state",std::to_string(blocCount)} });
    //            blocCount++;
    //        }
    //    }
    //}

    //// 完成写入 - 合并所有临时文件并写入最终 BCF 文件  
    //writer.finalize();

    //std::cout << "BCF file written successfully!" << std::endl;
    //std::cout << "Total blocks written: " << blocCount << std::endl;
    std::cout << "Press any key to exit..." << std::endl;

    std::cin.get();
    return 0;
}