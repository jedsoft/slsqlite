import ("sqlite");
private define get_args (n)
{
   variable args = __pop_args (n-1);
   variable db = ();
   return db.db, args;
}

private define get_table (db, name)
{
   return sqlite_get_table (db.db, name);
}

private define get_row ()
{
   variable db, args; (db, args) = get_args (_NARGS);
   return sqlite_get_row (db, __push_args (args));
}

private define get_array ()
{
   variable db, args; (db, args) = get_args (_NARGS);
   return sqlite_get_array (db, __push_args (args));
}

private define exec ()
{
   variable db, args; (db, args) = get_args (_NARGS);
   return sqlite_exec (db, __push_args (args));
}

private define changes ()
{
   variable db, args; (db, args) = get_args (_NARGS);
   return sqlite_changes (db, __push_args (args));
}

private define reset (stmt)
{
   sqlite_reset (stmt.stmt);
}

private define bind_params ()
{
   variable args = __pop_args (_NARGS-1);
   variable stmt = ();
   sqlite_bind_params (stmt.stmt, __push_args (args));
   variable options = __qualifiers();
   if (options != NULL)
     {
	variable name;
	foreach name (get_struct_field_names(options))
	  {
	     variable index = sqlite_bind_parameter_index(stmt.stmt, ":" + name);
	     ifnot (index) continue;
	     sqlite_bind_param (stmt.stmt, index, get_struct_field (options, name));
	  }
     }
}
   
private define step ()
{
   variable args = __pop_args (_NARGS-1);
   variable stmt = ();
   return sqlite_step (stmt.stmt, __push_args (args));
}

private define fetch (stmt)
{
   return sqlite_fetch(stmt.stmt);
}

private define prepare ()
{
   variable db, args;
   args = __pop_args (_NARGS-1);
   db = ();
   variable stmt = sqlite_prepare (db.db, __push_args (args));
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

define sqlite_new (name)
{
   variable db = sqlite_open (name);
   if (db == NULL)
     return NULL;

   variable s = struct
     {
	db = db,
	get_table = &get_table,
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
