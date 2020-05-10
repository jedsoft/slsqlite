import ("sqlite");

private define get_db_and_args (n)
{
   variable args = __pop_list (n-1);
   variable db = ();
   return db.db, args;
}

private define get_stmt_and_args (n)
{
   variable args = __pop_list (n-1);
   variable s = ();
   return s.stmt, args;
}

private define get_table (db, name)
{
   return sqlite_get_table (db.db, name);
}

private define get_row ()
{
   variable db, args; (db, args) = get_db_and_args (_NARGS);
   return sqlite_get_row (db, __push_list (args));
}

private define get_array ()
{
   variable db, args; (db, args) = get_db_and_args (_NARGS);
   return sqlite_get_array (db, __push_list (args));
}

private define exec ()
{
   variable db, args; (db, args) = get_db_and_args (_NARGS);
   variable st = list_pop (args);
   if (typeof (st) == Struct_Type)
     st = st.stmt;
   % This function calls: bind + step
   sqlite_exec (db, st, __push_list (args));
}

private define changes ()
{
   variable db, args; (db, args) = get_db_and_args (_NARGS);
   return sqlite_changes (db, __push_list (args));
}

private define reset (stmt)
{
   sqlite_reset (stmt.stmt);
}

private define bind_params ()
{
   variable stmt, args;
   (stmt, args) = get_stmt_and_args (_NARGS);
   sqlite_bind_params (stmt, __push_list (args));
   variable options = __qualifiers();
   if (options != NULL)
     {
	variable name;
	foreach name (get_struct_field_names(options))
	  {
	     variable index = sqlite_bind_parameter_index(stmt, ":" + name);
	     ifnot (index) continue;
	     sqlite_bind_param (stmt, index, get_struct_field (options, name));
	  }
     }
}

private define step ()
{
   variable stmt, args;
   (stmt, args) = get_stmt_and_args (_NARGS);
   return sqlite_step (stmt, __push_list (args));
}

private define fetch (stmt)
{
   return sqlite_fetch(stmt.stmt);
}

private define prepare (db, sql)
{
   variable stmt = sqlite_prepare (db.db, sql);
   if (stmt == NULL) return NULL;
   variable s = struct
     {
	% S-Lang frees struct fields in the order they're listed in, so list
	% the stmt field before the db field
	stmt = stmt,
	db = db,
	bind_params = &bind_params,
	step = &step,
	fetch = &fetch,
	reset = &reset,
     };
   return s;
}

private define read_table ()
{
   variable db, args; (db, args) = get_db_and_args (_NARGS);
   variable stmt = list_pop (args);
   if (typeof(stmt) == Struct_Type)
     stmt = stmt.stmt;
   else
     stmt = sqlite_prepare (db, stmt);

   if (length (args))
     sqlite_bind_params (stmt, __push_list(args));

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
   %if (0 == length (data[0])) return s;

   _for i (0, ncols-1, 1)
     {
	set_struct_field (s, cols[i], data[i]);
     }

   return s;
}

private define get_table_list (db)
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

define sqlite_new (name)
{
   variable db = sqlite_open (name);
   if (db == NULL)
     return NULL;

   variable s = struct
     {
	db = db,
	get_table = &get_table,
	get_table_list = &get_table_list,
	read_table = &read_table,
	get_row = &get_row,
	get_array = &get_array,
	exec = &exec,
	changes = &changes,
	prepare = &prepare,
     };

   return s;
}

$1 = path_concat (path_concat (path_dirname (__FILE__), "help"), "sqlite.hlp");
if (NULL != stat_file ($1))
  add_doc_file ($1);

provide ("sqlite");
