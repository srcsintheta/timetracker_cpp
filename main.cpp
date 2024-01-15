// soci
#include <ios>
#include <soci/soci.h>
#include <soci/sqlite3/soci-sqlite3.h>

// stdlib libraries
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>

// other
#include <fmt/core.h>		

// own header files
#include "./sql.hpp"		// namespace: SQL
#include "./tracker.hpp"	// namespace: TRACKER
#include "./time.hpp"		// namespace: TIME

// function prototypes
void work(soci::session& sql);
void stats(soci::session& sql);
void configure(soci::session& sql);
void manual(soci::session& sql);

using namespace std;

// global constants
const string VERSION { "1.20" };
const string DB_NAME { "productivity.db" };

int main(void)
{
	try
	{
		// creates db if it doesn't exist
		soci::session sql("sqlite3", "db=" + DB_NAME);

		int tableCount {};
		sql << "SELECT COUNT(*) FROM sqlite_master WHERE type = 'table'",
			soci::into(tableCount);

		if (tableCount == 0) {
			cout << "Initializing db w/ needed tables" << endl;
			SQL::bootup(sql);
		}
		else if (tableCount == 2)
		{
			cout << "db exists, has right table count" << endl;
			cout << "(this is the extent of checking db integrity)" << endl;
		}
		else {
			throw runtime_error("db has wrong tableCount");
		}

		cout <<
			"Productivity tracker" << endl << 
			"Version: " << VERSION << endl << endl;

		while (1)
		{
			cout <<
				"Available options:\n"
				"(w)ork\n"
				"(s)tats\n"
				"(c)onfigure\n"
				"(m)anual\n"
				"(q)uit\n\n";

			cout << "Enter option: ";
			char option;
			cin >> option;

			switch (option) {
				case 'w':
					work(sql);
					break;
				case 's':
					stats(sql);
					break;
				case 'c':
					configure(sql);
					break;
				case 'm':
					manual(sql);
					break;
				case 'q':
					exit(0);
				default:
					cout << "Invalid option, enter valid option key!"
						<< endl << endl;
			}
		}
	}
	catch (const soci::sqlite3_soci_error& e)
	{
		cerr << "SQLite3 Error: " << e.what() << endl;
	}
	catch (const exception &e)
	{
		cerr << "Error: " << e.what() << endl;
	}
}

void work(soci::session& sql)
{
	/* work timer function
	 * user enters activity id, timer starts
	 * can switch back and forth between work phase and break
	 */

	soci::rowset<soci::row> activities =
		sql.prepare << "SELECT * FROM activities";

	/* activities table:
	 * +----+----------+------+------------+--------------+-------------+
	 * | id | group_id | name | added_when | is_activated | hours_total |
	 * +----+----------+------+------------+--------------+-------------+
	 */

	vector<int> actids;
	vector<string> actnms;

	// iterate over retrieved data, populate actids and atnms vectors
	// and print info while we're at it
	for (soci::row const& row : activities) {
		int 		id = row.get<int>("id");
		string nm = row.get<string>("name");
		actids.push_back(id);
		actnms.push_back(nm);
		cout << "ID - Name: " << id << " - "<< nm << endl;
	}

	cout << "Enter activity id and hit enter: ";
	int actid;
	cin >> actid;

	while (find(actids.begin(), actids.end(), actid) == actids.end()) {
		cout << "Not in database or activated, re-enter: ";
		cin >> actid;
	}

	bool countdown { false };
	int countdown_decision {};
	double countdown_hours { 0.0 };
	int countdown_seconds {};

	cout << fmt::format(
			"Count time either: 1) up 2) down\n"
			"Enter your option: "
			);

	while (!(cin >> countdown_decision) || 
			(countdown_decision != 1 && countdown_decision != 2)) {
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		cout << "Invalid input. Enter either 1 or 2: ";
	}

	if (countdown_decision == 1) {
		countdown = false;
	}
	else if (countdown_decision == 2) {
		countdown = true;
		cout << "Enter hours you want to work for: ";
		cin >> countdown_hours;
		countdown_seconds = static_cast<int>(countdown_hours * 3600); 
	}

	// clear buffer just in case (since timeloop uses getline)
	cin.ignore(numeric_limits<streamsize>::max(), '\n');

	// these variables are simply for showing prompt at the end
	unsigned int work  {};
	unsigned int pause {};

	// the following maps are used to have start and end values
	unordered_map<string, string> smap; // start time map
	unordered_map<string, string> emap; // end   time map

	string timeloopreturn;
	bool br {}; // keep track of whehter we're in a break or not

	while (1)
	{
		cout << "Started work timer!" << endl;
		br = false;

		// start
		smap = TIME::get_datetime_map();

		// work time clock cycle (s start, e end)
		auto s = chrono::steady_clock::now();
		timeloopreturn = TRACKER::timeloop(countdown, countdown_seconds, br);
		auto e = chrono::steady_clock::now();
		auto duration = chrono::duration_cast<chrono::seconds>(e-s);

		// end
		emap = TIME::get_datetime_map();

		// record worked time to database
		TRACKER::update_work_time(sql, to_string(actid), smap, emap,
				static_cast<unsigned int>(duration.count()));

		// add to total work time
		work += static_cast<unsigned int>(duration.count());

		// output worked time (in total) thus far
		cout << fmt::format("Worked for {:02} minutes and {:02} seconds",
				(work / 60), (work % 60)) << endl;

		if (timeloopreturn == "q") break;

		cout << "Started break timer!" << endl;
		br = true;

		// break time clock cycle (s start, e end)
		s = chrono::steady_clock::now();
		timeloopreturn = TRACKER::timeloop(countdown, countdown_seconds, br);
		e = chrono::steady_clock::now();
		duration = chrono::duration_cast<chrono::seconds>(e-s);

		// add to total pause time
		pause += static_cast<unsigned int>(duration.count());

		// output paused time (in total) thus far
		cout << fmt::format("Paused for {:02} minutes and {:02} seconds",
				(pause / 60), (pause % 60)) << endl;

		if (timeloopreturn == "q") break;
	}
	double hours_worked { TIME::conv_seconds_to_hours(work) };
	double hours_paused { TIME::conv_seconds_to_hours(pause) };

	cout << "Worked for: " <<
		TIME::conv_hours_to_timestring(hours_worked) << endl;
	cout << "Paused for: " << 
		TIME::conv_hours_to_timestring(hours_paused) << endl;

	return;

}

void stats(soci::session& sql)
{
	/* prompts user for days X into the past stats should be shown for
	 * bit complicated:
	 * -) uses chrono timepoint converted to timestruct
	 * -) loops backwards X days creating all appropriate dates
	 * -) populates a vector with those dates (to be later used to query db)
	 * -) calls external function passing vector to retrieve data
	 * -) then passes it to another to print the stats
	 */

	unordered_map<string, string> datetime = TIME::get_datetime_map();
	int yy = stoi(datetime["year"]);
	int mm = stoi(datetime["month"]);
	int dd = stoi(datetime["day"]);

	cout << "Stats on last X days (today excluded): ";
	int no_of_days;
	cin >> no_of_days;

	vector<string> dates;

	auto tp_date = chrono::system_clock::now();					// current date
	time_t tt_time = chrono::system_clock::to_time_t(tp_date);
	tm* local_time = localtime(&tt_time);						// time_struct

	// oddities of time_struct:
	// a) tm_year is years since 1900
	// b) months start at 0
	// c) but days don't

	local_time->tm_year = yy - 1900;
	local_time->tm_mon  = mm - 1;
	local_time->tm_mday = dd;
	local_time->tm_mday -= 1; // start from yesterday

	string oldestdatefromdb;
	sql << "SELECT MIN(date) FROM history", soci::into(oldestdatefromdb);

	for (int i = 0; i < no_of_days; ++i) {

		time_t date = mktime(local_time);
		tm* day = localtime(&date);
		stringstream ss;
		ss << put_time(day, "%Y-%m-%d");
		dates.push_back(ss.str());

		if (oldestdatefromdb == ss.str())
		{
			no_of_days = i;
			cout <<
				"Oldest entry in history table is " << ss.str() << endl <<
				"(showing stats for last " << no_of_days+1 << " days)" << endl;
			break;
		}

		local_time->tm_mday -= 1;  // go one back further into the past
	}

	cout << endl;

	SQL::print_stats(sql, SQL::get_dates_data(sql, dates), no_of_days);

	return;
}

void configure(soci::session& sql)
{
	/* let's user add, deactivte, reactivate (already deactiviated) activities
	 * `is_activated` is a column in the activities table (int) to represent
	 * the activation status
	 */

	SQL::print_activities(sql, true);

	cout << 
		"Options: \n"
		"(a)dd new\n"
		"(d)eactivate\n"
		"(r)eactivate\n"
		"(q)uit\n";

	cout << "Choose: ";
	string choice;
	cin >> choice;
	cout << endl;

	if (choice == "a")
	{
		string name, group;
		cout << "Enter activity name: ";
		cin >> name;
		cout << "Enter group: ";
		cin >> group;

		string date = TIME::get_date_string();
		sql <<
			"INSERT INTO activities "
			"(name, group_id, added_when) "
			"VALUES (:name, :groupid, :added)",
			soci::use(name),
			soci::use(group),
			soci::use(date);
	}
	else if (choice == "d")
	{
		cout << "Enter activity id: ";
		string id;
		cin >> id;

		sql <<
			"UPDATE activities SET is_activated = 0 WHERE id = :id",
			soci::use(id);
	}
	else if (choice == "r")
	{
		cout << "Enter activity id: ";
		string id;
		cin >> id;

		sql <<
			"UPDATE activites SET is_activated = 1 WHERE id = :id",
			soci::use(id);
	}
	else if (choice == "q")
	{
		return;
	}
	else
	{
		throw runtime_error("Illegal option entered");
	}
	return;
}

void manual(soci::session& sql)
{
	SQL::print_activities(sql, false);

	cout << "For which activity id do you want to enter a time: ";
	string id;
	cin >> id;

	cout << "Date (yyyy-mm-dd): ";
	string date;
	cin >> date;

	cout << "Enter hours (double): ";
	double hours;
	cin >> hours;

	SQL::enter_work_time(sql, id, date, hours);

	return;
}
