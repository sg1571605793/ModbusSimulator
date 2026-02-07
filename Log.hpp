#ifndef LOG_HPP
#define LOG_HPP

#include <iostream>

template<typename T>
void Log(T && val){
    std::cout << val << std::endl;
}

template<typename T, typename ...Arg>
void Log(T &&val, Arg && ...args){
    std::cout << val << "\t";
    Log(std::forward<Arg>(args)...);
}

#endif // LOG_HPP
