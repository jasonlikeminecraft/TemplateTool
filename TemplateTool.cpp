#include <iostream>  
#include "BCFCachedWriter.hpp"  
#include "BCFStreamReader.hpp"  
int main() {


    BCFStreamReader reader("output.bcf");
    // 2. 获取子区块总数  
    size_t totalSubChunks = reader.getSubChunkCount();
    std::cout << "Total sub-chunks: " << totalSubChunks << std::endl;

    // 3. 按需读取指定的子区块  
    for (size_t i = 0; i < totalSubChunks; i++) {
        // getBlocks() 只读取第 i 个子区块,返回 vector<BlockInfo>  
        std::vector<BlockInfo> blocks = reader.getBlocks(i);

        std::cout << "Sub-chunk " << i << " has " << blocks.size() << " blocks" << std::endl;

        // 4. 处理每个方块  
        for (const auto& block : blocks) {
            std::cout << "Block at (" << block.x << ", " << block.y << ", " << block.z << "): "
                << block.type << std::endl;

           //  访问方块状态  
            for (const auto& [stateName, stateValue] : block.states) {
                std::cout << "  " << stateName << ": " << stateValue << std::endl;
            }
        }
    }

    //// 创建 BCFCachedWriter 实例  
    //// 参数: 输出文件名, 临时目录, 内存中最大方块数  
    //BCFCachedWriter writer("output.bcf", "./temp_bcf_cache", 5000);

    //// 添加方块 - 自动管理内存和临时文件  
    //// 参数: x, y, z, 方块类型, 状态列表  
    //writer.addBlock(0, 0, 0, "minecraft:stone", {});
    //writer.addBlock(1, 0, 0, "minecraft:stone", {});
    //writer.addBlock(2, 1, 1, "minecraft:grass", { {"snowy", 1} });

    //int blocCount = 0;
    //// 可以添加大量方块,不用担心内存溢出  
    //for (int x = 0; x < 100; x++) {
    //    for (int y = -56; y < 320; y++) {
    //        for (int z = 0; z < 100; z++) {
    //            writer.addBlock(x, y, z, "minecraft:dirt", {});
    //            blocCount++;
    //        }
    //    }
    //}

    //// 完成写入 - 合并所有临时文件并写入最终 BCF 文件  
    //writer.finalize();

    //std::cout << "BCF file written successfully!" << std::endl;
    //std::cout << "Total blocks written: " << blocCount << std::endl;

    return 0;
}