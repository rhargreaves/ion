#pragma once
#include "router.h"

namespace ion::wasm {

class TestRoutes {
   public:
    static void add_test_routes(Router& router);
};

}  // namespace ion::wasm
