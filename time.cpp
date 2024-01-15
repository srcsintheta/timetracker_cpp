#include <chrono>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

#include "./time.hpp"

using namespace std;

namespace TIME
{
    string get_datetime(void)
    {
        /* function returning string representing datetime
         */
        auto time_now   = chrono::system_clock::now();
        auto time_now_t = chrono::system_clock::to_time_t(time_now);
        stringstream time_now_ss;
        time_now_ss << put_time(localtime(&time_now_t), "%Y-%m-%d %X");
        return time_now_ss.str();
    }

    string get_date_string(void)
    {
        /* function querying chrono for time and returning just date string
         */
        string datetime { get_datetime() };
        return datetime.substr(0, datetime.size()-9);
    }

    string get_time_string(void)
    {
        /* function querying chrono for time and reurning just time string
         */
        string datetime { get_datetime() };
        return datetime.substr(11, datetime.size());
    }

    string get_weeknumber(void)
    {
        /* function getting ISO 8601 calendar week number
         * uses ctime
         */
        time_t t { time(nullptr) }; // current time as UTC
        tm* now  { localtime(&t) }; // conversion to localtime

        char buf[3];
        strftime(buf, sizeof(buf), "%V", now);
        /* "%V" denotes week number, character array will be nul terminated
         * buffer of three is sufficient
         */

        return string(buf);
    }

    string get_weeknumber_for_date(string const date)
    {
        /* function using ctime to get weeknumber for a data provided as string
         */
        tm time_in {0, 0, 0, 0, 0, 0};

        sscanf(date.c_str(), "%d-%d-%d",
                &time_in.tm_year,
                &time_in.tm_mon,
                &time_in.tm_mday
                );

        // correct for struct tm conventions
        time_in.tm_year -= 1900;  // years counted since 1900
        time_in.tm_mon  -= 1;     // months start at 0
        // (note days don't start at 0)

        mktime(&time_in);    // normalize tm struct

        char buf[3];
        strftime(buf, sizeof(buf), "%V", &time_in);
        // "%V" denotes week number, character array will be nul terminated

        return string(buf);
    }

    unordered_map<string, string> get_datetime_map()
        {
            /* function querying chrono for full datetime string
             * then creating a map with singular values
             * and returning that map
             */
            string datetime_str { get_datetime() };
            unordered_map<string, string> datetime_map;
            datetime_map["date"]   = from_datetime_extract_date (datetime_str);
            datetime_map["time"]   = from_datetime_extract_time (datetime_str);
            datetime_map["second"] =
                from_datetime_extract_second(datetime_str);
            datetime_map["minute"] =
                from_datetime_extract_minute(datetime_str);
            datetime_map["hour"]   = from_datetime_extract_hour (datetime_str);
            datetime_map["day"]    = from_datetime_extract_day  (datetime_str);
            datetime_map["wkno"]   = get_weeknumber();
            datetime_map["month"]  = from_datetime_extract_month(datetime_str);
            datetime_map["year"]   = from_datetime_extract_year (datetime_str);
            return datetime_map;
        }

    string from_datetime_extract_date(string const datetime)
    {
        /*  takes chrono datetime string and returns just date portion
         */
        return datetime.substr(0, datetime.size()-9);
    }

    string from_datetime_extract_time(string const datetime)
    {
        /* takes chrono datetime string and returns just time portion
         */
        return datetime.substr(11, datetime.size());
    }

    string from_datetime_extract_second(string const datetime)
    {
        /* takes chrono datetime string and returns just second portion
         */
        return datetime.substr(datetime.size()-2, 2);
    }

    string from_datetime_extract_minute(string const datetime)
    {
        /* takes chrono datetime string and returns just minute portion
         */
        return datetime.substr(datetime.size()-5, 2);
    }

    string from_datetime_extract_hour(string const datetime)
    {
        /* takes chrono datetime string and reutrns just hour portion
         */
        return datetime.substr(11, 2);
    }

    string from_datetime_extract_day(string const datetime)
    {
        /* takes chrono datetime string and reutrns just day portion
         */
        return datetime.substr(8, 2);
    }

    string from_datetime_extract_month(string const datetime)
    {
        /* takes chrono datetime string and reutrns just month portion
         */
        return datetime.substr(5, 2);
    }

    string from_datetime_extract_year(string const datetime)
    {
        /* takes chrono datetime string and reutrns just year portion
         */
        return datetime.substr(0, 4);
    }

    double conv_seconds_to_hours(unsigned int const seconds)
    {
        /* seconds to hours rounded to four decimal places
         */

        return round(seconds / 60.0 / 60.0 * 1000) / 1000;
        /* originally I had precision to two decimal places, but the potential
         * rounding error of 0.5% (18 seconds) per work phase was deemed to
         * large to be acceptable (with 100 work phases the total could be 1800
         * seconds off, or half an hour)
         */
    }

    string conv_hours_to_timestring(double const hours)
    {
        /* takes hour float and converts to hours and minutes string
         */
        int hours_i { static_cast<int>(hours) };
        int minutes { static_cast<int>(round((hours - hours_i) * 60)) };

        return to_string(hours_i) + ":" + (minutes < 10 ? "0" : "") +
            to_string(minutes);
    }


    vector<int> get_time_vector(void)
    {
        /* creates a vector storing values of yyyy-mm-dd hh:mm:ss singularly
         */
        stringstream time_ss(get_time_string());

        vector<int> time {};

        string year, month, day, hour, minute, second;

        getline(time_ss, year,   '-');
        getline(time_ss, month,  '-');
        getline(time_ss, day,    ' ');
        getline(time_ss, hour,   ':');
        getline(time_ss, minute, ':');
        getline(time_ss, second, '\n');

        time.push_back(stoi(year));
        time.push_back(stoi(month));
        time.push_back(stoi(day));
        time.push_back(stoi(hour));
        time.push_back(stoi(minute));
        time.push_back(stoi(second));

        return time;
    }

}
