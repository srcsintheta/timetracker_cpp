#pragma once

#include <soci/soci.h>

namespace SQL {

   void bootup(soci::session& sql); 

   soci::rowset<soci::row> get_dates_data(
           soci::session& sql,
           std::vector<std::string> const& dates
           );

   void print_stats(
           soci::session& sql,
           soci::rowset<soci::row> const& data,
           int const days
           );

   void print_activities(
           soci::session& sql,
           bool const print_deactivated
           );

   void enter_work_time(
           soci::session& sql,
           std::string const id,
           std::string const date,
           double hours
           );
}
