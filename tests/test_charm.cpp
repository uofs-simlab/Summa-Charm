#include <charm++.h>
#include <iostream>

class TestChare : public CBase_TestChare {
public:
    TestChare() {
        std::cout << "✓ Charm++ chare created successfully" << std::endl;
        CkExit();
    }
};

class Main : public CBase_Main {
public:
    Main(CkArgMsg* m) {
        std::cout << "Running Charm++ integration tests..." << std::endl;
        CProxy_TestChare::ckNew();
    }
};

#include "test_charm.def.h"
