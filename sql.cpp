#include <cmath>
#include <iostream>
#include <iomanip>
#include <soci/soci.h>
#include <fmt/core.h>

#include "./time.hpp"
#include "./sql.hpp"

using namespace std;

namespace SQL
{
    void bootup(soci::session& sql)
    {
        // create activities table
        sql <<
            "CREATE TABLE activities ("
            "id INTEGER PRIMARY KEY, " // PRIMARY implies NOT NULL and UNIQUE
            "group_id INTEGER NOT NULL, "
            "name TEXT NOT NULL, "
            "added_when TEXT NOT NULL, "
            "is_activated INTEGER NOT NULL DEFAULT 1, "
            "hours_total NUMERIC NOT NULL DEFAULT 0.0"
            ");";

        // create history table
        sql << 
            "CREATE TABLE history ("
            "id_activity INTEGER NOT NULL, "
            "year INTEGER NOT NULL, "
            "month INTEGER NOT NULL, "
            "day INTEGER NOT NULL, "
            "weeknumber INTEGER NOT NULL, "
            "hours_on_day NUMERIC NOT NULL DEFAULT 0.0, "
            "date TEXT NOT NULL, " // simplifies some SQL queries
            "FOREIGN KEY (id_activity) REFERENCES activities(id)"
            ");";

        cout <<
            "First time running: Add activities to track!"     << endl <<
            "Each activity has a name (string, no whitespace)" << endl <<
            "and an associated group (integer)" << endl <<
            "('q' as activity name when done)"  << endl << endl;

        while (true) {
            string name, group_id;

            cout << "Enter activity name: ";
            cin >> name;

            if (name == "q") {
                break;
            }

            cout << "Enter group id: ";
            cin >> group_id;

            unordered_map<string, string> dtmap =
                TIME::get_datetime_map();

            sql <<
                "INSERT INTO activities "
                "(name, group_id, added_when, hours_total) "
                "VALUES "
                "(:name, :group_id, :added_when, :hours_total)",
                soci::use(name),
                soci::use(group_id),
                soci::use(dtmap["date"]),
                soci::use(0);
        }
    }

    soci::rowset<soci::row> get_dates_data(
            soci::session& sql,
            vector<string> const& dates)
    {
        // pick exactly the fields we need to make life easier for ourselves
        // in print_stat function
        string query = 
            "SELECT "
            "activities.id, activities.group_id, activities.name, "
            "history.hours_on_day "
            "FROM activities INNER JOIN history "
            "ON activities.id = history.id_activity "
            "WHERE DATE IN (";
        for (size_t i {}; i < dates.size(); ++i)
        {
            query += "'" + dates[i] + "'";
            if (i != dates.size() - 1) {
                query += ", ";
            }
        }
        query += ")";

        soci::rowset<soci::row> dates_data =
            sql.prepare << query;

        return dates_data;
    }

    void print_stats(
            soci::session& sql,
            soci::rowset<soci::row> const& data,
            int const days)
    {
        map<int, double> id_to_hh; // activity id to hours
        map<int, double> gr_to_hh; // group id to hours
        map<int, string> id_to_nm; // activity id to name
        
        /* soci::rowset doesn't have a .size() method nor can its internal
         * iterator be reset, this makes easily testing whether it's empty or
         * not non-trivial, testing if data.begin() == data.end() will 
         * advance the iterator; we could create a vector of our rowset,
         * but that seems inefficient, so we count the rows here
         */

        /* for reference: data is rowset<row> of following columns:
         * +----+----------+------+--------------+
         * | id | group_id | name | hours_on_day |
         * +----+----------+------+--------------+
         */

        int rowcount {};

        for (const soci::row& row : data)
        {
            ++rowcount;
            int         id { row.get<int>("id") };
            int         gr { row.get<int>("group_id") };
            string 		nm { row.get<string>("name") };
            double      hh { row.get<double>("hours_on_day") };

            id_to_hh[id] += hh;
            gr_to_hh[gr] += hh;
            id_to_nm[id]  = nm;
        }

        if (rowcount == 0)
        {
            cout << "No entries were retrieved, back to menu!" << endl;
            return;
        }

        // indentation levels
        string idt { "    " };   // 4 spaces
        string idT { "      " }; // 6 spaces

        // print group stats first by iterating over gr_to_hh map
        cout << "Group stats: " << endl;
        for (pair<int, double> group : gr_to_hh)
        {
            cout << fmt::format(
                    "{} Hours per group {}: {:7.2f} (avg of {:.2f} per day)\n",
                    idt, group.first, group.second, group.second / days);
       }

        // print activity stats

        cout << "Activity stats: " << endl;
        for (pair<int, double> pair : id_to_hh)
        {
            int    id { pair.first };
            double hh { pair.second };
            string nm { id_to_nm[id] };

            
            cout << fmt::format(
                    "  Activity: {} \n"
                    "  Worked  : {:.2f} \n"
                    "  Avg/Day : {:.2f} \n",
                    nm, hh, hh/days);

            double hh_total { 0.0 };

            sql <<
                "SELECT hours_total FROM activities "
                "WHERE id = :id",
                soci::use(id), soci::into(hh_total);

            cout << fmt::format(
                    "{} (total hours tracked: {:.2f} hours\n",
                    idT, hh_total);
        }
        cout << endl;
    }

    void print_activities(soci::session& sql, bool const print_deactivated)
    {
        /* prints the activities from activities table
         * print_deactivated is a flag whether to print deactivated activities
         */

        soci::rowset<soci::row> activities =
            sql.prepare << "SELECT * FROM activities";

        cout << "\nActivities: \n\n";

        cout << fmt::format("{:<10}{:<10}{:<20}",
                "id", "group", "name") << endl;

        /* activities
         * +----+----------+------+------------+--------------+-------------+
         * | id | group_id | name | added_when | is_activated | hours_total |
         * +----+----------+------+------------+--------------+-------------+
         */

        for (const soci::row& row : activities) {
            if (!(row.get<int>("is_activated")) && !(print_deactivated))
                continue;
            cout << fmt::format("{:<10}{:<10}{:<20}",
                    row.get<int>("id"),
                    row.get<int>("group_id"),
                    row.get<string>("name"));

            if (!(row.get<int>("is_activated")))
                cout << "(deactivated)";
            cout << endl;
        }
        cout << endl << endl;
    }

    void enter_work_time(
            soci::session& sql,
            string const id,
            string const date,
            double hours)
    {
        // some basic sanity checks
        if ((date.length() != 10) || (hours < 0) || (stoi(id) < 0)) {
            runtime_error("Failed input sanity checks for manual entry");
        }

        // round hours to four decimal places
        hours = round(hours * 1000) / 1000;

        // get weeknumber and some other stuff for the history table entry
        string wkno = TIME::get_weeknumber_for_date(date);
        // our already written extract functions assume datetime string
        string datetime = date + " xx:xx";

        string year  = TIME::from_datetime_extract_year(datetime);
        string month = TIME::from_datetime_extract_month(datetime);
        string day   = TIME::from_datetime_extract_day(datetime);

        double hoursday { -1.0 };

        sql <<
            "SELECT hours_on_day FROM history "
            "WHERE id_activity = :id AND date = :date",
            soci::use(id), soci::use(date),
            soci::into(hoursday);

        if (hoursday == -1.0)
        {
            sql <<
                "INSERT INTO history "
                "(id_activity, year, month, day, weeknumber, hours_on_day, "
                "date) "
                "VALUES "
                "(:id, :year, :month, :day, :wkno, :hours, :date)",
                soci::use(id),
                soci::use(year),
                soci::use(month),
                soci::use(day),
                soci::use(wkno),
                soci::use(hours),
                soci::use(date);
        }
        else
        {
            hours += hoursday;
            sql <<
                "UPDATE history "
                "SET hours_on_day = :hours "
                "WHERE id_activity = :id AND date = :date",
                soci::use(hours),
                soci::use(id),
                soci::use(date);
        }

        // update hours value in activities table as well
        double total_hours {};

        sql <<
            "SELECT hours_total FROM activities WHERE id = :id",
            soci::use(id), soci::into(total_hours);

        total_hours += hours;

        sql <<
            "UPDATE activities "
            "SET hours_total = :hours "
            "WHERE id = :id",
            soci::use(total_hours),
            soci::use(id);
    }
}
