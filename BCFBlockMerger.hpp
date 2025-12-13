#pragma once  
#include "BCFStreamReader.hpp"  
#include "BCFCachedWriter.hpp"  
#include <vector>  
#include <string>  
#include <iostream>
class BCFBlockMerger {
private:
    std::string inputFilename;
    std::string outputFilename;

public:
    BCFBlockMerger(const std::string& input) {
        inputFilename = input;
        // 自动添加_merged后缀  
        size_t dotPos = input.find_last_of('.');
        if (dotPos != std::string::npos) {
            outputFilename = input.substr(0, dotPos) + "_merged" + input.substr(dotPos);
        }
        else {
            outputFilename = input + "_merged.bcf";
        }
        mergeAndSave();
    }

    void mergeAndSave() {
        BCFStreamReader reader(inputFilename);
        BCFCachedWriter writer(outputFilename);

        size_t totalSubChunks = reader.getSubChunkCount();

        for (size_t i = 0; i < totalSubChunks; i++) {
            auto regions = reader.getBlockRegions(i);
            auto origin = reader.getSubChunkOrigin(i);

            // 遍历所有区域，展开为单个方块  
            for (const auto& region : regions) {
                const PaletteKey& paletteKey = reader.getPaletteKey(region.paletteId);
                std::string blockType = reader.getBlockType(paletteKey.typeId);

                // 构建状态向量  
                std::vector<std::pair<std::string, std::string>> states;
                for (const auto& state : paletteKey.states) {
                    std::string stateName = reader.getStateName(state.first);
                    std::string stateValue = reader.getStateValue(state.second);
                    states.emplace_back(stateName, stateValue);
                }

                // 将区域展开为单个方块并添加  
                for (Coord x = region.x1; x <= region.x2; x++) {
                    for (Coord y = region.y1; y <= region.y2; y++) {
                        for (Coord z = region.z1; z <= region.z2; z++) {
                            // 使用原始坐标（加上origin偏移）  
                            writer.addBlock(
                                origin.originX + x,
                                origin.originY + y,
                                origin.originZ + z,
                                blockType,
                                states
                            );
                        }
                    }
                }
            }
        }

        writer.finalize();
        std::cout << "合并完成! 输出文件: " << outputFilename << std::endl;
    }
};