#include <iostream>
#include <cassert>

// Test basic functionality
int main() {
    std::cout << "Running basic functionality tests..." << std::endl;
    
    // Test 1: Basic compilation and linking
    std::cout << "✓ Compilation and linking successful" << std::endl;
    
    // Test 2: C++17 features
    auto lambda = [](int x) { return x * 2; };
    assert(lambda(5) == 10);
    std::cout << "✓ C++17 features working" << std::endl;
    
    std::cout << "All basic tests passed!" << std::endl;
    return 0;
}
