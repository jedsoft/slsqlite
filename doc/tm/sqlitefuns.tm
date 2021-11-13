#d ppstmt prepared statement
#s+
custom_variable("tm_format", "slhlp");
if (tm_format == "html")
{
   tm_add_macro("vdt", "\\dtdd{\\code{$1}}{$2}", 2, 2);
   tm_add_macro("n", "\\newline", 0, 0);
   tm_add_macro("link", "\\href{#$1}{$1}", 1, 1);
}
else
{
   tm_add_macro("dl", "$1", 1, 1);
   tm_add_macro("vdt", "\\var{$1}: $2\\__newline__", 2, 2);
   tm_add_macro("n", "", 0, 0);
   tm_add_macro("link", "$1", 1, 1);
}
#s-

\function{sqlite_new}
\synopsis{Create a new SQLite instance}
\usage{Struct_Type sqlite_new(String_Type filename)}
\description
  Create a new SQLite database object.  This returns a structure with the
  field \var{db} holding the actual SQLite handle, and the methods
#v+
   get_table
   get_row
   get_array
   exec
   changes
   prepare
#v-
\seealso{sqlite_open, sqlite_get_table, sqlite_get_row, sqlite_get_array, sqlite_exec,
sqlite_changes, sqlite_prepare}
\done

\function{sqlite_open}
\synopsis{Open a SQLite database}
\usage{Sqlite_Type sqlite_open(String_Type filename)}
\description
  Open the sqlite database file \var{filename}.  If the database file does
  not exist, then a new database will be created as needed.  On failure a
  \var{SqliteError} exception is thrown.
\seealso{sqlite_new, sqlite_get_table, sqlite_get_array, sqlite_get_row, sqlite_exec, sqlite_changes}
\done

\function{sqlite_get_table}
\synopsis{Get a table of results from a SQLite query}
\usage{String_Type[] sqlite_get_table(Sqlite_Type db, String_Type query)}
\description
  This executes a query and returns the result as a 2d array of strings.
  The first row of the array contains the column headers.  This function does
  not support placeholders.
\notes
  You should only use this function if you need the column headers.
  Otherwise, use \ifun{sqlite_get_array}
\seealso{sqlite_open, sqlite_get_array, sqlite_get_row, sqlite_exec, sqlite_changes}
\done

\function{sqlite_get_row}
\synopsis{Get a row of results from a SQLite query}
\usage{sqlite_get_row(Sqlite_Type db, String_Type query, ...)}
\description
  This executes a query and pushes the elements of the first row of the
  result on the stack.  This supports string, integer, float and blob
  datatatypes. Blobs are returned as bstrings.  If there are no rows, a
  \var{SqliteError} exception is thrown even if the query executed flawlessly.
  Question marks in the query are placeholders.  Extra arguments to the
  function are bound to these placeholders from left to right.
\example
#v+
   (foo, bar) = sqlite_get_row("SELECT foo, bar FROM table WHERE baz = ?", "quux");
#v-
\notes
  To get integers greater than INT_MAX, use \ifun{sqlite_get_array} with a
  \var{LLONG_TYPE} type.
  
  To get the result of a query that returns multiple rows, use
  \ifun{sqlite_get_array} or use
#v+
  foreach foo, bar (db) using ("SELECT foo, bar FROM table WHERE baz = ?", "quux")
  {
      ....
  }
#v-
  Or in the object-oriented interface:
#v+
  foreach foo, bar (db.db) using ("SELECT foo, bar FROM table WHERE baz = ?", "quux")
  {
      ....
  }
#v-
\seealso{sqlite_open, sqlite_get_table, sqlite_get_array, sqlite_exec, sqlite_changes}
\done

\function{sqlite_get_array}
\synopsis{Get a 2-D array from a SQLite query}
\usage{Array_Type sqlite_get_array(Sqlite_Type db, DataType_type type, String_Type query, ...)}
\description
  Executes a query and returns the result as a 2d array of type \var{type}.
  This supports string, integer, long long, float and blob datatatypes.
  Question marks in the query are placeholders. Extra arguments to the
  function are bound to these placeholders from left to right.
\seealso{sqlite_open, sqlite_get_table, sqlite_get_row, sqlite_exec, sqlite_changes}
\done

\function{sqlite_exec}
\synopsis{Execute a SQLite query}
\usage{sqlite_exec(Sqlite_Type db, String_Type query, ...)}
\description
  Execute a SQL query on a sqlite database, without returning a result.
  Question marks in the query are placeholders.  Extra arguments to the
  function are bound to these placeholders from left to right.
\example
#v+
  sqlite_exec(db, "INSERT INTO table(foo,bar,baz) VALUES (?,?,?)", 1, 2, 3);
#v-
  Or in the object-oriented interface:
#v+
  db.exec("INSERT INTO table(foo,bar,baz) VALUES (?,?,?)", 1, 2, 3);
#v- 
\seealso{sqlite_open, sqlite_get_table, sqlite_get_array, sqlite_get_row, sqlite_changes}
\done

\function{sqlite_changes}
\synopsis{Get the number of rows affected by a SQLite query}
\usage{Int_Type sqlite_changes(Sqlite_Type db)}
\description
  This function returns the number of database rows that were changed (or
  inserted or deleted) by the most recently completed INSERT, UPDATE, or
  DELETE statement.  The change count for "DELETE FROM table" will be zero
  regardless of the number of elements that were originally in the table.
\seealso{sqlite_open, sqlite_get_table, sqlite_get_array, sqlite_get_row, sqlite_exec}
\done

\function{sqlite_prepare}
\synopsis{create a SQLite prepared statement}
\usage{Sqlite_Statement_Type sqlite_prepare(Sqlite_Type db, String query)}
\description
  Prepare a SQL query.
\example
#v+
   stmt = sqlite_prepare(db, "INSERT INTO table(foo,bar,baz) VALUES (?,?,?)");
#v-
  \var{stmt} will now hold a Sqlite_Statement_Type.
  Or using the object-oriented interface:
#v+
   stmt = db.prepare("INSERT INTO table(foo,bar,baz) VALUES (?,?,?)");
#v-
  \var{stmt} will now hold a struct with, a field \var{stmt} holding the
  Sqlite_Statement_Type, a field \var{db} holding the Database object, and the
  methods
#v+
   bind_params
   step
   fetch
   reset
#v-
\notes
  In the procedural interface, you should destroy all \ppstmt{}s before
  the database goes out of scope. Otherwise the database will not be
  closed. In the object-oriented interface, this is ensured by the Statement
  object holding a reference to the Database object.
\seealso{sqlite_bind_params, sqlite_step, sqlite_fetch, sqlite_reset}
\done

\function{sqlite_bind_params}
\synopsis{bind values to a prepared statement}
\usage{int sqlite_bind_params(Sqlite_Statement_Type, ..)}
\description
  Bind values to a prepared statement.  Arguments are bound to placeholders
  in the statement from left to right.  If called with fewer arguments than
  the \ppstmt has placeholders, the remaining bindings are not modified.
  Parameter bindings are not affected by \ifun{sqlite_reset}.
\example
#v+
  stmt = sqlite_prepare(db, "insert into table(foo, bar) values (?,?)");
  sqlite_bind_params(stmt, "foo", "bar");
  slite_bind_params(stmt, "baz");
#v-
  The first parameter will now be set to ``baz'', the second to ``bar''.
  The object-oriented method \sfun{bind_params} also supports named
  parameters as qualifiers.
#v+
  stmt = db.prepare("insert into table(foo, bar) values (:foo,:bar)");
  stmt.bind_params(stmt, "foo", "bar");
  stmt.bind_params(;  bar = "baz");
#v-
  Now the first parameter will be ``foo'', the second will be ``baz''.
\notes
  This function must be called after \ifun{sqlite_prepare} or
  \ifun{sqlite_reset} and before \ifun{sqlite_step}. Bindings are not
  cleared by the \ifun{sqlite_reset} routine. Unbound parameters are
  interpreted as NULL.
\seealso{sqlite_prepare, sqlite_bind_param, sqlite_step, sqlite_fetch, sqlite_reset}
\done

\function{sqlite_bind_param}
\synopsis{bind a value to a prepared statement}
\usage{int sqlite_bind_params(Sqlite_Statement_Type, Int_Type n, value)}
\description
  In the SQL strings input to \ifun{sqlite_prepare}, literals may be
  replaced by a parameter in one of these forms:

#v+
  ? 
  ?NNN 
  :VVV 
  @VVV 
  $VVV 
#v-

  In the parameter forms shown above NNN is an integer literal, VVV is an
  alpha-numeric parameter name. The values of these parameters can be set
  using the \ifun{sqlite_bind_param} function.

  The first argument to \ifun{sqlite_bind_param} is a prepared statement
  returned by \ifun{sqlite_prepare}.  The second argument is the index of
  the parameter, counting from 1.  When the same named parameter is used
  more than once, second and subsequent occurrences have the same index as
  the first occurrence. The index for named parameters can be looked up
  using the function \ifun{sqlite_bind_parameter_index} if desired.  The
  index for ``?NNN'' parameters is the value of NNN.
\notes
  You can set all parameters at once with \ifun{sqlite_bind_params}.  To set
  a specific parameter, you can use named parameters of the ``:VVV'' form
  with the object method \sfun{bind_params}.
\seealso{sqlite_prepare, sqlite_bind_params, sqlite_bind_parameter_index, sqlite_step, sqlite_fetch, sqlite_reset}
\done

\function{sqlite_bind_parameter_index}
\synopsis{Return the index of an SQL parameter given its name}
\usage{int sqlite_bind_parameter_index(Sqlite_Statement_Type S, String_Type N)}
\description
  The \ifun{sqlite_bind_parameter_index} function returns the index of the
  SQL parameter in \ppstmt \exmp{S} whose name matches the string \exmp{N},
  or 0 if there is no match.
\seealso{sqlite_bind_param}
\done

\function{sqlite_step}
\synopsis{evaluate a prepared statement}
\usage{int sqlite_step(Sqlite_Statement_Type, ..)}
\description
  This function evaluates a \ppstmt.
  
  This function returns a SQLite return code.  If the query has completed, it
  returns SQLITE_DONE (101).  If the query has a result row ready, it returns
  SQLITE_ROW (100). If SQLite returns an error code, this function throws a
  SqliteError.
\notes  
  If you want to execute a query and iterate over the results, use
#v+
  stmt = db.prepare("sql");
  stmt.bind_params("foo", "bar")
  while (SQLITE_ROW == stmt.step())
  {
     result = stmt.fetch();
  }
#v-
  Or
#v+
  foreach result (db.db) using ("sql", "foo", "bar")
  {
    ...
  }
#v-
  If you want to execute a query multiple times with different parameters,
  you would use
#v+
  stmt = db.prepare("sql");
  foreach param (array)
  {
    stmt.bind_params(param);
    ()=stmt.step());
    stmt.reset();
  }
#v-
\seealso{sqlite_prepare, sqlite_bind_params, sqlite_fetch, sqlite_reset}
\done

\function{sqlite_fetch}
\synopsis{Get a row of results from a prepared statement}
\usage{sqlite_fetch(Sqlite_Statement_Type)}
\description
  This pushes the elements of the current result row of a prepared statement
  on the stack.
\seealso{sqlite_prepare, sqlite_step, sqlite_reset, sqlite_get_row}
\done

\function{sqlite_reset}
\synopsis{reset a prepared statement}
\usage{sqlite_reset(Sqlite_Statement_Type S)}
\description
  The \ifun{sqlite_reset} function resets the \ppstmt \exmp{S} back to the
  beginning of its program.
\seealso{sqlite_prepare, sqlite_bind_params, sqlite_step, sqlite_fetch}
\done
