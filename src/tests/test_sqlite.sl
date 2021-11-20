prepend_to_slang_load_path (".");
set_import_module_path (".:" + get_import_module_path());

require ("sqlite");

private define sqlite_num_rows (db, tblname)
{
   variable s = db.prepare ("SELECT COUNT(*) from $tblname"$);
   if (SQLITE_ROW == s.step ())
     return s.fetch ();

   return 0;
}

private define sqlite_version (db)
{
   variable stmt = db.prepare ("SELECT SQLITE_VERSION()");
   if (SQLITE_ROW == stmt.step ())
     return stmt.fetch ();

   return -1;
}

private define get_table_as_struct (db, sql)
{
   variable stmt = sqlite_prepare (db.db, sql);
   variable i, ncols = sqlite_column_count (stmt);
   variable cols = String_Type[ncols];
   variable data = List_Type[ncols];

   _for i (0, ncols-1, 1)
     {
	cols[i] = sqlite_column_name (stmt, i);
	data[i] = {};
     }

   while (SQLITE_ROW == sqlite_step (stmt))
     {
	sqlite_fetch (stmt);

	_for i (ncols-1, 0, -1)
	  {
	     variable v = ();
	     list_append (data[i], v);
	  }
     }

   variable s = @Struct_Type(cols);
   if (0 == length (data[0])) return s;

   _for i (0, ncols-1, 1)
     {
	set_struct_field (s, cols[i], data[i]);
     }

   return s;
}

private define list_tables (db)
{
   variable sql = "select name from sqlite_master where type = 'table'";
   variable s = db.prepare (sql);
   variable tbls = {};
   while (s.step() == SQLITE_ROW)
     {
	list_append (tbls, s.fetch());
     }
   return tbls;
}

private define sqlite_define_table (db, tblname, colnames, coltypes)
{
   db.exec ("DROP TABLE IF EXISTS $tblname;"$);

   variable sqlcols = strjoin ("\""+colnames+"\"" + " " + coltypes, ",");
   variable sql =
     [
      %"DROP TABLE IF EXISTS $tblname;"$,
      "CREATE TABLE $tblname($sqlcols);"$,
     ];
   db.exec (strjoin (sql, "\n"));
}

define xslsh_main1 ()
{
   variable db = sqlite_new ("/tmp/places.sqlite");

   variable tbls = list_to_array (list_tables (db));
   variable tables = @Struct_Type(tbls);
   foreach (tbls)
     {
	variable tbl = ();
	set_struct_field (tables, tbl, get_table_as_struct (db, "SELECT * from $tbl;"$));
     }
   print (tables);
}

define slsh_main ()
{
   variable db = sqlite_new ("sltest.db");
   print (sqlite_version (db));
   list_tables (db);

   sqlite_define_table (db, "xexample", ["xstring", "xinteger", "xblob"], ["TEXT", "FOOINT", "BLOB"]);

   db.exec("INSERT INTO xexample(xstring, xinteger, xblob) VALUES (?,?,?)", "One", 1, "BStr\0");
   db.exec("INSERT INTO xexample(xstring, xinteger, xblob) VALUES (?,?,?)", "Two", 10, "BS\0tr\0");
   db.exec("INSERT INTO xexample(xstring, xinteger, xblob) VALUES (?,?,?)", "Three", 1000, "B\0Str\0");
   print (db.changes ());
   %print (db.get_table("SELECT * from xexample;"));
   %print (db.get_table ("PRAGMA table_info(xexample)"));
   () = fprintf (stdout, "Num Rows: %d\n", sqlite_num_rows (db, "xexample"));
   %() = fprintf (stdout, "Num Columns: %d\n", sqlite_num_colums (db, "xexample"));

   print (get_table_as_struct (db, "PRAGMA table_info(xexample)"));
   variable x = get_table_as_struct (db, "SELECT * from xexample;");
   print (x.xblob);
}

define slsh_main ()
{
   variable file = sprintf ("/tmp/test_sqlite_%X-%X.sqlite", getpid(), _time());

   variable db = sqlite_new (file);

   variable sql = "CREATE TABLE Friends(Id INTEGER PRIMARY KEY, Name TEXT);";
   db.exec (sql);

   variable stmt = db.prepare ("INSERT INTO Friends(Name) VALUES (?);");
   foreach (["Tom", "Rebecca", "Jim", "Roger", "Robert"])
     {
	variable name = ();
	db.exec (stmt, name);
     }

   print (db.get_table_list ());

   variable s = db.get_table ("SELECT * from Friends;");
   print (s);
   s = get_table_as_struct (db, "SELECT * from Friends");
   print (s);
   () = remove (file);
}
