#pragma once  
#include <string>  
#include <vector>  
#include <unordered_map>  
#include <fstream>  
#include <sstream>  
#include <iostream>  
#include <unordered_set>

class BlockStateConverter {
private:
    // 使用完整的方块字符串作为键  
    std::unordered_map<std::string, std::string> javaToBedrockMap;

public:
    // 从文件加载转换表  
    bool loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file) {
            std::cerr << "无法打开转换表文件: " << filename << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            // 跳过空行  
            if (line.empty()) continue;

            // 解析格式: "in : <java_block>"  
            if (line.find("in :") == 0) {
                std::string javaBlock = line.substr(5); // 跳过 "in : "  

                // 读取下一行 (uni)  
                std::getline(file, line);

                // 读取下一行 (out)  
                if (std::getline(file, line) && line.find("out:") == 0) {
                    std::string bedrockBlock = line.substr(5); // 跳过 "out: "  

                    // 去除首尾空格  
                    javaBlock = trim(javaBlock);
                    bedrockBlock = trim(bedrockBlock);

                    javaToBedrockMap[javaBlock] = bedrockBlock;
                }
            }
        }

        std::cout << "加载了 " << javaToBedrockMap.size() << " 条转换规则" << std::endl;
        return true;
    }

    // 转换 Java 方块到 Bedrock 方块  
    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>
        convert(const std::string& javaBlockName,
            const std::vector<std::pair<std::string, std::string>>& javaStates) {

        // 重建完整的 Java 方块字符串  
        std::string fullJavaBlock = buildBlockString(javaBlockName, javaStates);

        // 查找转换表  
        auto it = javaToBedrockMap.find(fullJavaBlock);
        if (it != javaToBedrockMap.end()) {
            // 找到匹配, 解析 Bedrock 方块字符串  
            return parseBlockString(it->second);
        }

        // ⚠️ 仅打印一次未找到的方块  
        static std::unordered_set<std::string> missingBlocks;  // 记录已输出的未匹配方块

        if (missingBlocks.insert(fullJavaBlock).second) { // insert 返回 pair<iterator, bool>，bool 表示是否新插入
            std::cout << "未找到匹配的 Bedrock 方块: " << fullJavaBlock << std::endl;
        }

        // 返回原始方块（无转换）
        return { javaBlockName, javaStates };
    }
private:
    // 去除首尾空格  
    std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

    // 构建完整的方块字符串  
    std::string buildBlockString(const std::string& blockName,
        const std::vector<std::pair<std::string, std::string>>& states) {
        if (states.empty()) {
            return blockName;
        }

        std::string result = blockName + "[";
        for (size_t i = 0; i < states.size(); ++i) {
            if (i > 0) result += ",";
            result += states[i].first + "=\"" + states[i].second + "\"";
        }
        result += "]";
        return result;
    }

    // 解析方块字符串  
    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>
        parseBlockString(const std::string& fullName) {
        size_t bracketPos = fullName.find('[');

        if (bracketPos == std::string::npos) {
            return { fullName, {} };
        }

        std::string blockName = fullName.substr(0, bracketPos);
        std::vector<std::pair<std::string, std::string>> states;

        size_t start = bracketPos + 1;
        size_t end = fullName.find(']', start);

        if (end == std::string::npos) {
            return { blockName, {} };
        }

        std::string statesStr = fullName.substr(start, end - start);

        // 分割逗号  
        size_t pos = 0;
        while (pos < statesStr.size()) {
            size_t commaPos = statesStr.find(',', pos);
            if (commaPos == std::string::npos) {
                commaPos = statesStr.size();
            }

            std::string pair = statesStr.substr(pos, commaPos - pos);
            size_t equalPos = pair.find('=');

            if (equalPos != std::string::npos) {
                std::string key = pair.substr(0, equalPos);
                std::string value = pair.substr(equalPos + 1);

                // 去除引号  
                if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }

                states.emplace_back(trim(key), trim(value));
            }

            pos = commaPos + 1;
        }

        return { blockName, states };
    }
};