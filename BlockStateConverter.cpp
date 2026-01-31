#include "BlockStateConverter.hpp"
#include "block_state_data.hpp"  // 内置表

bool BlockStateConverter::loadFromStream(std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        if (line.find("in :") == 0) {
            std::string javaBlock = trim(line.substr(5));

            std::getline(in, line); // uni

            if (std::getline(in, line) && line.find("out:") == 0) {
                std::string bedrockBlock = trim(line.substr(5));
                javaToBedrockMap[javaBlock] = bedrockBlock;
            }
        }
    }

    std::cout << "加载了 " << javaToBedrockMap.size() << " 条转换规则" << std::endl;
    return true;
}

//bool BlockStateConverter::loadBuiltinTable() {
//    std::string data(BLOCK_STATE_TABLE, BLOCK_STATE_TABLE_SIZE);
//    std::istringstream ss(data);
//    return loadFromStream(ss);
//}
