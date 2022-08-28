db: db_study.c
	gcc db_study.c -o db_study

run: db_study
	./ddb_studyb mydb.db