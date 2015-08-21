# sqlite-importer

Ever find yourself in need to quickly analyze a big chunk of text data—e.g.,
Apache log files? I sometimes do. And while it's okay to fumble with `grep`,
`sed`, and the likes for a while, I quickly end up mumbling to myself, "if only
this were an SQL database …"

And so one of those days, I just ended up writing this super-simple app to
import log files into a SQLite database. And now I'm sharing (-:

## How to get started

It's a C++ app, so you'll need to compile it yourself.

### Prerequisites

- [Boost libraries](http://www.boost.org)
- [SQLite](https://www.sqlite.org) (including C bindings)

On OS X w/ Homebrew, you can simply do:

```
brew install sqlite boost
```

### Compiling

Makefile to the rescue!

```
make
```

### Importing

```
./sqlite-importer file1.log.gz file2.log.gz
```

### Profit :moneybag:

```
sqlite3 imported_logs.db
```

**Hint:** consider creating indexes for the columns (or groups of columns) that
often end up in your `WHERE` clauses. SQLite has a great overview of how [query
planning](https://www.sqlite.org/queryplanner.html) works, which should help you
understand if an index would help or not.

## Customizing

The app uses the Apache Combined Log Format by default, and reads from gzipped
files, but you can easily adapt it to your own needs:

-   change the table name and/or schema:

    ```c++
    sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS imported_logs (ip TEXT, user TEXT, datetime TEXT, request TEXT, response_code INTEGER, referer TEXT, user_agent TEXT)", NULL, NULL, NULL);
    // don't forget to modify the insert statement too:
    sqlite3_prepare_v2(db, "INSERT INTO imported_logs VALUES (?, ?, ?, ?, ?, ?, ?)", -1, &stmt, NULL);
    // … and the bindings:
    sqlite3_bind_text(stmt, 1, matches["ip"].str().c_str(), -1, SQLITE_TRANSIENT);
    // etc.
    ```

-   modify the regular expression used to extract data from each line of input:

    ```c++
    regex apache_combined_log_format("(?'ip'.+) - (?'user'.+) \\[(?'datetime'.+)\\] \"(?'request'.+ .+) .+\" (?'response_code'.+) .+ \"(?'referer'.+)\" \"(?'user_agent'.+)\"");
    ```

-   read from non-compressed files:

    ```c++
    // comment this line out:
    in.push(gzip_decompressor());
    ```

## Acknowledgments

I cannot thank the Stack Overflow community enough for
[this brilliant piece][1], originally by [Mike Willekes][2]. Importing 7 GB of
logs suddenly went from :sleeping: to :rocket:.

[1]: http://stackoverflow.com/questions/1711631/improve-insert-per-second-performance-of-sqlite
[2]: http://stackoverflow.com/users/203690/mike-willekes
