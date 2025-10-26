#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include "BCFUtils.hpp"
#include "BCFReader.hpp"
int main() {
    std::vector<BlockGroup> sub0, sub1;

    // 添加方块
    BlockGroup bg;
    bg.paletteId = 0;
    BlockUtils::addBlock(bg, 0, 0, 0);
    BlockUtils::addBlock(bg, 1, 0, 0);
    sub0.push_back(bg);

    BlockGroup bg2;
    bg2.paletteId = 1;
    BlockUtils::addBlock(bg2, 2, 1, 1);
    sub0.push_back(bg2);

    sub1.push_back(bg); // 第二个子区块重复使用

    // -------------------- 合并重复 BlockGroup --------------------
    sub0 = MergeUtils::mergeBlockGroups(sub0);
    sub1 = MergeUtils::mergeBlockGroups(sub1);

    std::vector<std::vector<BlockGroup>> subChunks = { sub0, sub1 };

    // Palette / Type / State
    std::vector<PaletteKey> paletteList = { {1,{}}, {2,{{0,1}}} };
    std::map<BlockTypeID, std::string> typeMap = { {1,"stone"},{2,"grass"} };
    std::map<BlockStateID, std::string> stateMap = { {0,"snowy"} };

    // 写文件
    BCFUtils::writeBCF("1.bcf", subChunks, paletteList, typeMap, stateMap);
    std::map<BlockTypeID, std::string> typeMap1{};
    std::map<BlockStateID, std::string> stateMap1{};

    BCFReader reader("1.bcf");
    for (size_t i = 0; i < reader.getSubChunkCount(); i++)
    {
        auto subChunk = reader.getBlocks(i);
        for (auto& bg : subChunk) {
            if(bg.states.empty())
                std::cout << "Block: " << bg.type << " " << bg.x << " " << bg.y << " " << bg.z << " " << "\n";
            for (auto& block : bg.states) {
                std::cout << "Block: " << bg.type << " " << bg.x << " " << bg.y << " " << bg.z << " " << block.first << ": " << block.second << "\n";
                }
        }
    }
    // 读文件
    //auto readSubChunks = BCFUtils::readAllSubChunks("1.bcf");
    //for (size_t i = 0; i < readSubChunks.size(); i++) {
    //    std::cout << "SubChunk " << i << " has " << readSubChunks[i].size() << " BlockGroups\n";
    //}
}
