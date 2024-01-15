#pragma once
#include <string>
#include <unordered_map>
#include <vector>

namespace TIME {

    // get functions
    std::string get_datetime(void);
    std::string get_date_string(void);
    std::string get_time_string(void);
    std::string get_weeknumber(void);
    std::string get_weeknumber_for_date(std::string const date);
    std::unordered_map<std::string, std::string> get_datetime_map();
    std::vector<int> get_time_vector(void);

    // from datetime string retreive only certain things
    std::string from_datetime_extract_date(std::string const datetime);
    std::string from_datetime_extract_time(std::string const datetime);
    std::string from_datetime_extract_second(std::string const datetime);
    std::string from_datetime_extract_minute(std::string const datetime);
    std::string from_datetime_extract_hour(std::string const datetime);
    std::string from_datetime_extract_day(std::string const datetime);
    std::string from_datetime_extract_month(std::string const datetime);
    std::string from_datetime_extract_year(std::string const datetime);

    // conversion functions
    double conv_seconds_to_hours(unsigned int const seconds);
    std::string conv_hours_to_timestring(double const hours);
}
