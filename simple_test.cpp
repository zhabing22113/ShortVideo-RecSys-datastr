#include <iostream>
#include <fstream>

int main() {
    std::cout << "Test started" << std::endl;
    
    std::ofstream out("test_result.txt");
    out << "Test passed!" << std::endl;
    out.close();
    
    std::cout << "Test completed" << std::endl;
    std::system("pause");
    return 0;
}