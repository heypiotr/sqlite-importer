#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <iostream>

#include "sqlite3.h"

int main(int argc, char* argv[]) {
  using namespace std;
  using namespace boost;
  using namespace boost::iostreams;

  sqlite3* db;
  sqlite3_stmt* stmt;

  sqlite3_open("imported_logs.db", &db);

  // forever grateful to the Stack Overflow community for reseraching how to
  // greatly improve the insert-per-second performance of SQLite:
  // http://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite
  sqlite3_exec(db, "PRAGMA synchronous = OFF", NULL, NULL, NULL);
  sqlite3_exec(db, "PRAGMA journal_mode = MEMORY", NULL, NULL, NULL);
  sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);

  sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS imported_logs (ip TEXT, user TEXT, datetime TEXT, request TEXT, response_code INTEGER, referer TEXT, user_agent TEXT)", NULL, NULL, NULL);
  sqlite3_prepare_v2(db, "INSERT INTO imported_logs VALUES (?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL);

  // example regexp to match Apache Combined Log Format, e.g.:
  //
  // 127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326 "http://www.example.com/start.html" "Mozilla/4.08 [en] (Win98; I ;Nav)"
  //
  // the regexp below skips a few fields:
  // - identd information ("-")
  // - HTTP protocol version ("HTTP/1.0")
  // - request size ("2326")
  regex apache_combined_log_format("(?'ip'.+) - (?'user'.+) \\[(?'datetime'.+)\\] \"(?'request'.+ .+) .+\" (?'response_code'.+) .+ \"(?'referer'.+)\" \"(?'user_agent'.+)\"");

  unsigned long imported_count = 0;

  for (int i = 1; i < argc; ++i) {
    const char *filename = argv[i];
    cout << "Processing: " << filename << endl;

    ifstream file(filename, ios_base::in | ios_base::binary);
    filtering_istream in;
    in.push(gzip_decompressor());
    in.push(file);

    for (string line; getline(in, line);) {
      smatch matches;
      if (!regex_search(line, matches, apache_combined_log_format)) {
        continue;
      }

      sqlite3_bind_text(stmt, 1, matches["ip"].str().c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 2, matches["user"].str().c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 3, matches["datetime"].str().c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 4, matches["request"].str().c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 5, matches["response_code"].str().c_str(), -1, SQLITE_TRANSIENT); // SQLite will automatically convert to INTEGER
      sqlite3_bind_text(stmt, 6, matches["referer"].str().c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 7, matches["user_agent"].str().c_str(), -1, SQLITE_TRANSIENT);

      sqlite3_step(stmt);
      sqlite3_reset(stmt);

      ++imported_count;
    }
  }

  cout << "Imported " << imported_count << " rows" << endl;

  sqlite3_finalize(stmt);
  sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
  sqlite3_close(db);
}
