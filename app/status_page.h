#pragma once
#include "router.h"

class StatusPage {
   public:
    static void add_status_page(ion::Router& router);

   private:
    static std::string format_duration(uint64_t total_seconds);
};
