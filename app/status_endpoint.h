#pragma once
#include "router.h"

class StatusEndpoint {
   public:
    static void add_status_endpoint(ion::Router& router);
};