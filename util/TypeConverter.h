#ifndef UTILS_TYPECONVERTER_H
#define UTILS_TYPECONVERTER_H

#include <sstream>

class TypeConverter {
public:
    template <typename T>
    static T str2num(const std::string &str) {
        T result;
        std::istringstream iss(str);
        iss >> result;
        return result;
    }

    template <typename T>
    static std::string num2str(const T &num) {
        std::ostringstream oss;
        oss << num;
        return oss.str();
    }
};

#endif
