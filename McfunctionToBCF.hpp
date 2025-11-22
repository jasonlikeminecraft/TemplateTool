#pragma once  
#include "BCFCachedWriter.hpp"  
#include <fstream>  
#include <sstream>  
#include <string>  
#include <vector>  
#include <algorithm>  

class McfunctionToBCF {
private:
    const std::string m_filename;
    const std::string m_outputFilename;

public:
    McfunctionToBCF(const std::string& filename, const std::string& outputFilename)
        : m_filename(filename), m_outputFilename(outputFilename) {
        convert();
    }

    void convert() {
        std::ifstream file(m_filename);
        if (!file) {
            throw std::runtime_error("无法打开文件: " + m_filename);
        }

        BCFCachedWriter writer(m_outputFilename, "./temp_bcf_cache", 50000);

        std::string line;
        int lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;
            // 跳过空行和注释  
            if (line.empty() || line[0] == '#') {
                continue;
            }

            try {
                parseLine(line, writer);
            }
            catch (const std::exception& e) {
                std::cerr << "第 " << lineNum << " 行解析错误: " << e.what() << std::endl;
            }
        }

        writer.finalize();
    }

private:
    void parseLine(const std::string& line, BCFCachedWriter& writer) {
        std::istringstream iss(line);
        std::string command;
        iss >> command;

        if (command == "setblock") {
            parseSetblock(iss, writer);
        }
        else if (command == "fill") {
            parseFill(iss, writer);
        }
        // 忽略其他指令  
    }

    // 解析坐标字符串,忽略 ~ 和 ^ 符号  
    int parseCoord(const std::string& coordStr) {
        std::string cleaned = coordStr;
        // 移除 ~ 和 ^ 符号  
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '~'), cleaned.end());
        cleaned.erase(std::remove(cleaned.begin(), cleaned.end(), '^'), cleaned.end());

        // 如果清理后为空字符串,返回 0  
        if (cleaned.empty()) {
            return 0;
        }

        return std::stoi(cleaned);
    }

    void parseSetblock(std::istringstream& iss, BCFCachedWriter& writer) {
        std::string xStr, yStr, zStr, blockStr;

        if (!(iss >> xStr >> yStr >> zStr >> blockStr)) {
            throw std::runtime_error("setblock 指令格式错误");
        }

        int x = parseCoord(xStr);
        int y = parseCoord(yStr);
        int z = parseCoord(zStr);

        auto [blockName, states] = parseBlockNameAndStates(blockStr);
        writer.addBlock(x, y, z, blockName, states);
    }

    void parseFill(std::istringstream& iss, BCFCachedWriter& writer) {
        std::string x1Str, y1Str, z1Str, x2Str, y2Str, z2Str, blockStr;

        if (!(iss >> x1Str >> y1Str >> z1Str >> x2Str >> y2Str >> z2Str >> blockStr)) {
            throw std::runtime_error("fill 指令格式错误");
        }

        int x1 = parseCoord(x1Str);
        int y1 = parseCoord(y1Str);
        int z1 = parseCoord(z1Str);
        int x2 = parseCoord(x2Str);
        int y2 = parseCoord(y2Str);
        int z2 = parseCoord(z2Str);

        auto [blockName, states] = parseBlockNameAndStates(blockStr);

        // 确保坐标顺序正确  
        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);
        if (z1 > z2) std::swap(z1, z2);

        // 遍历矩形区域  
        for (int y = y1; y <= y2; y++) {
            for (int z = z1; z <= z2; z++) {
                for (int x = x1; x <= x2; x++) {
                    writer.addBlock(x, y, z, blockName, states);
                }
            }
        }
    }

    std::pair<std::string, std::vector<std::pair<std::string, std::string>>>
        parseBlockNameAndStates(const std::string& fullName) {
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
                states.emplace_back(key, value);
            }

            pos = commaPos + 1;
        }

        return { blockName, states };
    }
};