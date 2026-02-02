#pragma once
#include <fstream>
#include <string>
#include "bcf_structs.hpp"
#define NOMINMAX
#include <windows.h>

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






inline bool is_valid_utf8(const std::string& s) {
    const unsigned char* bytes = reinterpret_cast<const unsigned char*>(s.data());
    size_t len = s.size();

    for (size_t i = 0; i < len; ) {
        unsigned char c = bytes[i];

        // 1-byte (0xxxxxxx)
        if (c <= 0x7F) {
            i += 1;
        }
        // 2-byte (110xxxxx 10xxxxxx)
        else if ((c >> 5) == 0x6) {
            if (i + 1 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            i += 2;
        }
        // 3-byte (1110xxxx 10xxxxxx 10xxxxxx)
        else if ((c >> 4) == 0xE) {
            if (i + 2 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            if ((bytes[i + 2] >> 6) != 0x2) return false;
            i += 3;
        }
        // 4-byte (11110xxx 10xxxxxx 10xxxxxx 10xxxxxx)
        else if ((c >> 3) == 0x1E) {
            if (i + 3 >= len) return false;
            if ((bytes[i + 1] >> 6) != 0x2) return false;
            if ((bytes[i + 2] >> 6) != 0x2) return false;
            if ((bytes[i + 3] >> 6) != 0x2) return false;
            i += 4;
        }
        else {
            return false;
        }
    }
    return true;
}



inline std::string gbk_to_utf8(const std::string& gbk) {
    if (gbk.empty()) return {};

    // GBK -> UTF-16
    int wlen = MultiByteToWideChar(936, 0, gbk.data(), (int)gbk.size(), nullptr, 0);
    if (wlen <= 0) return {};

    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(936, 0, gbk.data(), (int)gbk.size(), &wstr[0], wlen);

    // UTF-16 -> UTF-8
    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, nullptr, 0, nullptr, nullptr);
    if (u8len <= 0) return {};

    std::string u8(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wlen, &u8[0], u8len, nullptr, nullptr);

    return u8;
}
inline std::string ensure_utf8(const std::string& s) {
    if (s.empty()) return s;

    if (is_valid_utf8(s)) {
        return s;              // 已经是 UTF-8
    }

    return gbk_to_utf8(s);     // 不是 → 当 GBK 转
}





// -------------------- ×Ö·û´®Ð´¶Á --------------------
inline void writeString16(std::ofstream& ofs, const std::string& str) {
    // 强制规范为 UTF-8
    std::string u8 = ensure_utf8(str);

    size_t s_len = u8.size();

    // BCF string16 最大 65535
    if (s_len > 0xFFFF) {
        throw std::runtime_error(
            "String length exceeds the maximum allowed 65535 for BCF writeString16 (after UTF-8 normalization)."
        );
    }

    uint16_t len = static_cast<uint16_t>(s_len);

    write_u16(ofs, len);
    if (len) {
        ofs.write(u8.data(), len);
    }
}


inline std::string readString16(std::ifstream& ifs) {
    uint16_t len = read_u16(ifs);
    std::string s; if (len) { s.resize(len); ifs.read(&s[0], len); }
    return s;
}

// 写入32位长度前缀的字符串  
inline void writeString32(std::ofstream& ofs, const std::string& str) {
    uint32_t len = static_cast<uint32_t>(str.size());
    write_u32(ofs, len);
    if (len) ofs.write(str.data(), len);
}

// 读取32位长度前缀的字符串  
inline std::string readString32(std::ifstream& ifs) {
    uint32_t len = read_u32(ifs);
    std::string s; if (len) { s.resize(len); ifs.read(&s[0], len); }
    return s;
}
