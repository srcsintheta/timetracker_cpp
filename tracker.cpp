#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <fmt/core.h>

#include "./time.hpp"
#include "./tracker.hpp"

using namespace std;

namespace TRACKER
{
    mutex mtx;
    condition_variable cv;
    bool done { false };

    void print_time(bool& countdown, int& countdown_seconds, bool& br) {
        /* prints a timer counting upwards in seconds
         */
        auto start = chrono::steady_clock::now();
		double countdown_hours {};

        while (true) {
            unique_lock<mutex> lock(mtx); // locks mutex
            /* wait_for() temporarily unlocks mutex and blocks thread for
             * maximum of one second; after waking up it re-locks mutex before
             * proceeding
             */
            cv.wait_for(lock, chrono::seconds(1), [](){ return done; });
            if (done) {
                break; // exit loop, complete thread
            }
            auto now = chrono::steady_clock::now();
            auto duration = chrono::duration_cast<chrono::seconds>(now-start);
            int hours   { static_cast<int>(duration.count() / 3600) };
            int minutes { static_cast<int>((duration.count() % 3600) / 60) };
            int seconds { static_cast<int>(duration.count() % 60) };

			if (!br & countdown)
			{
				countdown_seconds -= 1;
				countdown_hours =
					static_cast<double>(countdown_seconds) / 3600;

				cout << fmt::format("\t{:02.2f}\r", countdown_hours) << flush;

				if (countdown_hours < 0) {
					cout << fmt::format(
							"----------------------------------------------\n"
							"--  Finished set work time!                 --\n"
							"--  Feel free to continue                   --\n"
							"--  Time will continue to be counted        --\n"
							"----------------------------------------------\n"
							);
					countdown = false;
				}
			}
			else
			{
				cout << fmt::format("\t{:02}:{:02}:{:02}\r",
						hours, minutes, seconds) << flush;
			}
		}
    }

    string timeloop(bool& countdown, int& countdown_seconds, bool& br)
    {
        /* spawns timer (print_time) as separate thread
         * then waits for user input
         * sets a shared flag to true if input registered
         */

        string input;
        done = false;
		// launch print_time as separate thread
		thread t([&]() { print_time(countdown, countdown_seconds, br); });

        getline(cin, input);
        {
            lock_guard<mutex> lock(mtx);
            /* attemps to lock mutex, blocking if locked
             */
            done = true;
        } // Mutex automatically unlocked here due to end of scope

        cv.notify_one(); // notifies print_time thread to check 'done' flag
        /* (if it's in cv.wait_for())
         * (this may wake it up before full second has passed)
         */
        t.join(); // blocks until print_time thread has finished execution
        if (input.empty()) {
            return "";
        } else {
            return input;
        }
    }

    void update_work_time(
            soci::session& sql,
            string const actid,
            unordered_map<string, string>& smap,
            unordered_map<string, string>& emap,
            unsigned int const worked_seconds)
    {
        double hours { TIME::conv_seconds_to_hours(worked_seconds) };

        bool DAY_CHANGED { false };
        double before_midnight {};
        double after_midnight  {};

        if (smap["day"] != emap["day"]) {
            DAY_CHANGED = true;
            before_midnight  = 24.0 - stod(smap["hour"]);
            before_midnight -= (stod(smap["minute"]) / 60);
            before_midnight -= (stod(smap["second"]) / 60 / 60);
            after_midnight   =  stod(emap["hour"]);
            after_midnight  += (stod(emap["minute"]) / 60);
            after_midnight  += (stod(emap["second"]) / 60 / 60);
        } 
        else {
            before_midnight = hours;
        }

        if (DAY_CHANGED) {
            // we know the current day can't have an entry yet
            // we can simply insert the time here
            // (emap and after_midnight)
            sql <<
                "INSERT INTO history (id_activity, year, month, day, "
                "weeknumber, hours_on_day, date) "
                "VALUES "
                "(:id, :year, :month, :day, :wkno, :h_day, :date)",
                soci::use(actid),
                soci::use(emap["year"]),
                soci::use(emap["month"]),
                soci::use(emap["day"]),
                soci::use(emap["wkno"]),
                soci::use(after_midnight),
                soci::use(emap["date"]);
        }

        // time after midnight has been taken care of
        // now deal w/ before midnight and smap

        double hours_on_day { -1.0 };

        sql <<
            "SELECT hours_on_day FROM history "
            "WHERE date = :date "
            "AND id_activity = :id ",
            soci::use(smap["date"]),
            soci::use(actid),
            soci::into(hours_on_day);

        // if no value retrieved, insert
        if (hours_on_day == -1.0)
        {
            sql <<
                "INSERT INTO history (id_activity, year, month, day, "
                "weeknumber, hours_on_day, date) "
                "VALUES "
                "(:id, :yyyy, :mm, :dd, :wkno, :h_day, :dateval)",
                soci::use(actid),
                soci::use(smap["year"]),
                soci::use(smap["month"]),
                soci::use(smap["day"]),
                soci::use(smap["wkno"]),
                soci::use(before_midnight),
                soci::use(smap["date"]);
        }
        // if value retrieved, update
        else
        {
            hours_on_day += before_midnight;
            sql <<
                "UPDATE history SET "
                "hours_on_day = :hours "
                "WHERE date = :date "
                "AND id_activity = :id ",
                soci::use(hours_on_day),
                soci::use(smap["date"]),
                soci::use(actid);
        }

        // get hours_total from activities table and update value

        double hours_total { 0.0 };

        sql <<
            "SELECT hours_total FROM activities "
            "WHERE id = :id",
            soci::use(actid), soci::into(hours_total);

        hours_total += hours;

        sql <<
            "UPDATE activities SET "
            "hours_total = :hours "
            "WHERE id = :id",
            soci::use(hours_total),
            soci::use(actid);

        return;
    }
}
