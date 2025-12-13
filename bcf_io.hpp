#pragma once
#include <fstream>
#include <string>
#include "bcf_structs.hpp"

// -------------------- Endian-safe helpers --------------------
template<typename T> void write_le(std::ofstream& ofs, T v) { ofs.write(reinterpret_cast<const char*>(&v), sizeof(T)); }
template<typename T> void read_le(std::ifstream& ifs, T& v) { ifs.read(reinterpret_cast<char*>(&v), sizeof(T)); }

inline void write_u8(std::ofstream& ofs, uint8_t v) { write_le<uint8_t>(ofs, v); }
inline void write_u16(std::ofstream& ofs, uint16_t v) { write_le<uint16_t>(ofs, v); }
inline void write_u32(std::ofstream& ofs, uint32_t v) { write_le<uint32_t>(ofs, v); }
inline void write_u64(std::ofstream& ofs, uint64_t v) { write_le<uint64_t>(ofs, v); }
inline void write_i16(std::ofstream& ofs, int16_t v) { write_le<int16_t>(ofs, v); }

inline uint8_t  read_u8(std::ifstream& ifs) { uint8_t v; read_le(ifs, v); return v; }
inline uint16_t read_u16(std::ifstream& ifs) { uint16_t v; read_le(ifs, v); return v; }
inline uint32_t read_u32(std::ifstream& ifs) { uint32_t v; read_le(ifs, v); return v; }
inline uint64_t read_u64(std::ifstream& ifs) { uint64_t v; read_le(ifs, v); return v; }
inline int16_t  read_i16(std::ifstream& ifs) { int16_t v; read_le(ifs, v); return v; }

// -------------------- ×Ö·û´®Ð´¶Á --------------------
inline void writeString16(std::ofstream& ofs, const std::string& s) {
    size_t s_len = s.size();

    // 检查长度是否超出 uint16_t 的最大值 65535
    if (s_len > 65535) {
        // 抛出运行时错误，而不是静默截断数据
        throw std::runtime_error("String length exceeds the maximum allowed 65535 for BCF writeString16.");
    }
    
    // 长度在范围内，安全地转换为 uint16_t
    uint16_t len = static_cast<uint16_t>(s_len); 

    write_u16(ofs, len);
    if (len) ofs.write(s.data(), len);
}

inline std::string readString16(std::ifstream& ifs) {
    uint16_t len = read_u16(ifs);
    std::string s; if (len) { s.resize(len); ifs.read(&s[0], len); }
    return s;
}

// 写入32位长度前缀的字符串  
void writeString32(std::ofstream& ofs, const std::string& str) {
    size_t s_len = str.size();

    write_u32(ofs, s_len);
    if (s_len) ofs.write(str.data(), s_len);
}

// 读取32位长度前缀的字符串  
std::string readString32(std::ifstream& ifs) {
    uint16_t len = read_u32(ifs);
    std::string s; if (len) { s.resize(len); ifs.read(&s[0], len); }
    return s;
}
