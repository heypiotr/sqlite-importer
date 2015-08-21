sqlite-importer: sqlite-importer.cpp
	g++ -O2 -lboost_regex -lboost_iostreams -lsqlite3 sqlite-importer.cpp -o sqlite-importer

test: sqlite-importer
	./sqlite-importer example.log.gz
	sqlite3 imported_logs.db "select * from imported_logs"
