#pragma once
#include <soci/soci.h>

#include <string>

namespace TRACKER {

    std::string timeloop(bool& countdown, int& countdown_seconds, bool& br);
    void print_time(bool& countdown, int& countdown_seconds, bool& br);
	void print_time_old(void);

    void update_work_time(
            soci::session& sql,
            std::string const actid,
            std::unordered_map<std::string, std::string>& smap,
            std::unordered_map<std::string, std::string>& emap,
            unsigned int const worked_seconds);
}

