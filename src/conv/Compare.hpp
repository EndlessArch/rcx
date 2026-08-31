#ifndef RCX_CONV_COMPARE_HPP
#define RCX_CONV_COMPARE_HPP

#include <string>
#include <conv/Modernizer.hpp>

// NOTE: other operators be formed via `rewritten candidates`
inline
bool operator==(const std::string& s, char c) noexcept {
    return s.length() == 1 && s[0] == c;
}

inline bool operator==(char c, const std::string& s) noexcept {
    return s == c;
}

#endif // RCX_CONV_COMPARE_HPP