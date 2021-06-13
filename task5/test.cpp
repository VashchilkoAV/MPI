#include <iostream>
#include <string>
#include <sstream>

std::string operator*(const std::string& s, unsigned int n) {
    std::stringstream out;
    while (n--)
        out << s;
    return out.str();
}

std::string operator*(unsigned int n, const std::string& s) { return s * n; }



int main() {
    unsigned int k = 100;
    std::string a("123");
    std::string b = a*k;
    std::cout << b;
    return 0;
}