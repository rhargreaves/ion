#pragma once

#include "http2_server.h"

class TestHelpers {
   public:
    static ion::Http2Server create_test_server();
};
