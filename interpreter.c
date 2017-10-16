/************************************************************
 * Interpreter for db2700 in the Databases course INF-2700  *
 * UIT - The Arctic University of Norway                    *
 * Author: Weihai Yu                                        *
 ************************************************************/

#include "interpreter.h"
#include "schema.h"
#include "test_data_gen.h"
#include "pmsg.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char* t_database = "database";
static const char* t_show = "show";
static const char* t_print = "print";
static const char* t_create = "create";
static const char* t_drop = "drop";
static const char* t_insert = "insert";
static const char* t_select = "select";
static const char* t_quit = "quit";
static const char* t_help = "help";
static const char* t_int = "int";
static const char* t_str = "str";

static FILE *in_s; /* input stream, default to stdin */

static char front_dir[512];  /* dir where front is running */
static char cmd_file[200];   /* the file where front gets commands */
static char rest_of_line[300]; /* used to read the rest of line */

static int init_with_options ( int argc, char* argv[] )
{
  int c;
  char new_sys_dir[512];

  getcwd (front_dir, sizeof (front_dir));
  new_sys_dir[0] = '\0';
  cmd_file[0] = '\0';
  msglevel = INFO;

  while ((c = getopt (argc, argv, "hm:d:c:")) != -1)
    switch (c)
      {
      case 'h':
	printf ("Usage: runtest [switches]\n");
	printf ("\t-h           help, print this message\n");
	printf ("\t-m [fewid]   msg level [fatal,error,warn,info,debug]\n");
	printf ("\t-d db_dir    default to ./tests/testfront\n");
	printf ("\t-c cmd file  eg. ./tests/testcmd.dbcmd, default to stdin\n");
	exit (0);
      case 'm':
	switch (optarg[0])
	  {
	  case 'f': msglevel = FATAL; break;
	  case 'e': msglevel = ERROR; break;
	  case 'w': msglevel = WARN; break;
	  case 'i': msglevel = INFO; break;
	  case 'd': msglevel = DEBUG; break;
	  }
	break;
      case 'd':
	strcpy (new_sys_dir, optarg);
      case 'c':
	strcpy (cmd_file, optarg);
	break;
      case '?':
	if (optopt == 'm' || optopt == 'd' || optopt == 'c')
	  printf ("Option -%c requires an argument.\n", optopt);
	else if (isprint (optopt))
	  printf ("Unknown option `-%c'.\n", optopt);
	else
	  printf ("Unknown option character `\\x%x'.\n", optopt);
	abort ();
      default:
	abort ();
      }
	
  if ( cmd_file[0] == '\0' )
      in_s = stdin;
  else
    {
      in_s = fopen ( cmd_file, "r");
      if (in_s == NULL)
	{
	  printf ("Cannot open file: %s\n", cmd_file);
	  in_s = stdin;
	}
      else
	put_msg (DEBUG, "file \"%s\" is open for read.\n", cmd_file);
    }

  if ( in_s == stdin )
    {
      printf ("Welcome to db2700 session\n");
      printf ("  - Enter \"help\" for instructions\n");
      printf ("  - Enter \"quit\" to leave the session\n");
    }
	
  if (new_sys_dir[0] == '\0')
    strcpy (new_sys_dir, "./tests/testfront");

  if ( set_system_dir (new_sys_dir) == -1 )
    {
      put_msg (ERROR, "cannot set database at %s\n", new_sys_dir);
      return -1;
    }

  return open_db ();
}

static void skip_line ()
{
  fgets(rest_of_line, 200, in_s);
}

static void syntax_error ( char *near_str )
{
  put_msg (ERROR, "Syntax error near\n");
  put_msg (ERROR, "  ... >>>%s<<<", near_str);
  if (fgets(rest_of_line, 200, in_s))
    append_msg (ERROR, rest_of_line);
}

static void show_help_info ()
{
  printf ("You can run the following commands:\n");
  printf (" - help\n");
  printf (" - quit\n");
  printf (" - # some comments in the res of a line\n");
  printf (" - print text\n");
  printf (" - show database\n");
  printf (" - create table table_name ( field_name field_type, ... )\n");
  printf (" - drop table table_name (CAUTION: data will be deleted!!!)\n");
  printf (" - insert into table_name values ( value_1, value_2, ... )\n");
  printf (" - select attr1, attr2 from table_name where attr = int_val;\n\n");
}

static void quit ()
{
  close_db();
  if ( in_s != stdin )
    fclose (in_s);  
}

static void show_database ()
{
  char db_token[20];
  if ( fscanf (in_s, "%s", db_token) != 1)
    {
      put_msg (ERROR, "Show what?\n");
      return;
    }
  if ( strcmp (db_token, t_database) != 0 )
    {
      put_msg (ERROR, "Cannot show \"%s\".\n", db_token );
      return;
    }
  put_db_info (FORCE);
}

static void print_str ()
{
  if (fgets(rest_of_line, 200, in_s))
      printf ("%s\n", rest_of_line + 1);
}

static void create_tbl ()
{
  char tbl_name[30], field_name[30], field_type[4], separator[5];
  int len;
  if ( fscanf (in_s, " table %s (", tbl_name) != 1)
    {
      put_msg (ERROR, "Do not know what to create\n");
      return;
    }

  put_msg (DEBUG, "table name: \"%s\".\n", tbl_name);

  schema_p sch = get_schema (tbl_name);
  
  if ( sch != NULL )
    {
      put_msg (ERROR, "Table \"%s\" already exists.\n", tbl_name);
      skip_line ();
      return;      
    }
  sch = new_schema (tbl_name);
  while (!feof(in_s))
    {
      fscanf (in_s, "%s %3s", field_name, field_type);
      if ( strcmp (field_type, t_int) == 0 )
	{
	  add_field ( sch, new_int_field ( field_name ));
	}
      else if ( strcmp (field_type, t_str) == 0 )
	{
	  fscanf (in_s, "(%d)", &len);
	  add_field ( sch, new_str_field ( field_name, len ));
	}
      else
	{
	  strcat (field_name, " ");
	  strcat (field_name, field_type);
	  syntax_error (field_name);
	  remove_schema (sch);
	  return;
	}
      fscanf (in_s, "%s", separator);
      put_msg(DEBUG, "seperator: \"%s\".\n", separator);
      if (separator[0] == ')')
	{
	  skip_line ();
	  break;
	}
    }
}

static void trim_pending_semicolon ( char *str )
{
  int len = strlen (str);
  if ( str[len - 1] == ';')
    str[len - 1] = '\0';
}

static void drop_tbl ()
{
  char tbl_name[30];
  fscanf (in_s, " table %s;", tbl_name);
  trim_pending_semicolon (tbl_name);
  remove_table ( get_table(tbl_name) );
}

/* trim the ending ',', ')' or ");" of a string value */
static void trim_str_val_end ( char *str_val )
{
  int i = strlen (str_val) - 1;
  while (i > 0
	 && (str_val[i] == ',' || str_val[i] == ')' || str_val[i] == ';'))
    i--;
  str_val[i+1] = '\0';
}

static record new_filled_record ( schema_p s )
{
  record r = new_record ( s );
  int int_val;
  char str_val[MAX_STR_LEN];
  field_desc_p fld_d;
  int i = 0;
  for ( fld_d = schema_first_fld_desc(s);
	fld_d != NULL;
	fld_d = field_desc_next(fld_d), i++)
    {
      if ( is_int_field (fld_d) )
	{
	  fscanf (in_s, "%d,", &int_val);
	  assign_int_field (r[i], int_val);
	}
      else
	{
	  fscanf (in_s, "%s,", str_val);
	  trim_str_val_end ( str_val );
	  assign_str_field (r[i], str_val);
	}
    }
  skip_line ();
  put_record_info (DEBUG, r, s);
  return r;
}

static void insert_row ()
{
  char tbl_name[30];
	
  fscanf (in_s, " into %s values (", tbl_name);
  put_msg(DEBUG, "table name: \"%s\".\n", tbl_name);
  schema_p sch = get_schema (tbl_name);
  if (!sch)
    {
      put_msg (ERROR, "Schema \"%s\" does not exist.\n", tbl_name);
      skip_line ();
      return;
    }
  /* put_schema_info (DEBUG, sch); */
  record rec = new_filled_record ( sch );

  append_record (rec, sch);
  release_record (rec, sch);
}

/* split str into substrings, separated by char c,
   which is not white space. The substring has no white space. */
static int str_split ( char *str, char c, char *arr[], int max_count )
{
  int count = 0;
  char *p;

  for ( p = str; *p != '\0'; p++)
    {
      if ( *p == c )
	{
	  *p = '\0';
	  if (++count > max_count)
	    {
	      put_msg (ERROR, "str_split: more substring than allowed %d\n",
		       max_count);
	      return 0;
	    }
	}
    }
  count++;
  
  char str_tmp[11], str_tmp_after_space[11];
  int i;

  for ( i = 0, p = str; i < count; i++, p += strlen (p) + 1 )
    {
      if ( sscanf (p,"%s%s", str_tmp, str_tmp_after_space) != 1 )
	{
	  put_msg (ERROR,
		   "str_split: empty string or white space not allowed.\n");
	  for ( --i; i >= 0; i--)
	    {
	      free (arr[i]), arr[i] = NULL;
	    }
	  return 0;
	}
      arr[i] = (char *) malloc ( strlen(str_tmp) + 1 );
      strcpy (arr[i], str_tmp);
    }

  return count;
}

typedef struct select_desc {
  tbl_p from_tbl, right_tbl;
  char where_attr[11], where_op[3];
  int where_val;
  int num_attrs;
  char *attrs[10];
} select_desc;

static select_desc* new_select_desc ()
{
  select_desc *slct = (select_desc *) malloc (sizeof (select_desc));
  int i;
  for ( i = 0; i < 10; i++ ) slct->attrs[i] = NULL;
  slct->where_attr[0] = '\0';
  slct->where_op[0] = '\0';
  slct->num_attrs = 0;
  slct->from_tbl = NULL;
  slct->right_tbl = NULL;
  return slct;
}

static void release_select_desc (select_desc *slct)
{
  if ( slct == NULL ) return;
  int i;
  for ( i = 0; i < slct->num_attrs; i++ )
    free (slct->attrs[i]);
  free (slct); slct = NULL;
}

static select_desc* parse_select ()
{
  select_desc *slct = new_select_desc ();
  char in_str[200] = "", from_str[100] = "", join_with[20] = "";
  char *where_str, *join_str, *p;

  fscanf (in_s, "%[^;];", in_str);

  p = strstr (in_str, " from ");
  if ( p == NULL )
    {
      put_msg (ERROR, "select %s: from which table to select?\n", in_str);
      release_select_desc (slct);
      return NULL;
    }
  *p = '\0';
  p += 6;
  if ( sscanf (p, "%s", from_str) != 1 )
    {
      put_msg (ERROR, "select from what?\n");
      release_select_desc (slct);
      return NULL;
    }
  slct->from_tbl = get_table (from_str);
  if (slct->from_tbl == NULL)
    {
      put_msg (ERROR, "select: table \"%s\" does not exist.\n", from_str);
      release_select_desc (slct);
      return NULL;
    }

  slct->num_attrs = str_split ( in_str, ',', slct->attrs, 10);
  if (slct->num_attrs == 0)
    {
      put_msg (ERROR, "select from %s: select what?\n", p);
      release_select_desc (slct);
      return NULL;
    }

  p += 2;
  join_str = strstr(p, " natural join ");
  if ( join_str != NULL )
    {
      join_str += 14;
      put_msg (DEBUG, "from: \"%s\", natural join: \"%s\"\n",
	       from_str, join_str);
    if ( sscanf (join_str, "%s", join_with) != 1) {
      put_msg (ERROR, "natural join with \"%s\" is not supported.\n",
	       join_str);
      release_select_desc (slct);
      return NULL;
    }
    if ( strcmp(from_str, join_str) == 0 ) {
      put_msg (ERROR, "natural join on same table is not supported.\n");
      release_select_desc (slct);
      return NULL;
    }
    slct->right_tbl = get_table(join_with);
    if (slct->right_tbl == NULL) {
      put_msg(ERROR, "natural join: table \"%s\" does not exist.\n",
	      join_with);
      release_select_desc (slct);
      return NULL;
    }
    }

  where_str = strstr (p, " where ");
  if ( where_str != NULL ) where_str += 7;

  put_msg (DEBUG, "from: \"%s\", where: \"%s\"\n", from_str, where_str);

  if ( where_str != NULL )
    {
      if (sscanf ( where_str, "%s %s %d",
		   slct->where_attr, slct->where_op, &slct->where_val)
	  != 3)
	{
	  put_msg (ERROR, "query \"%s\" is not supported.\n", where_str);
	  release_select_desc (slct);
	  return NULL;
	}
    }
  return slct;
}

static void select_rows ()
{
  select_desc *slct = parse_select ();
  tbl_p where_tbl = NULL, res_tbl = NULL, join_tbl = NULL;

  if ( slct == NULL ) return;

  if ( slct->right_tbl != NULL)
    {
      join_tbl = table_natural_join (slct->from_tbl, slct->right_tbl);
      if ( join_tbl == NULL) { release_select_desc (slct); return; }
    }
  
  if ( slct->where_attr[0] != '\0' && slct->where_op[0] != '\0' )
    {
      put_msg(DEBUG,"attr: %s op: %s value: %d\n",slct->where_attr,slct->where_op,slct->where_val);
      where_tbl = table_search ( join_tbl ? join_tbl : slct->from_tbl,
				 slct->where_attr,
				 slct->where_op,
				 slct->where_val);
      if (where_tbl == NULL) { release_select_desc (slct); return; }
    }

  if ( slct->attrs[0][0] == '*' )
    table_display (where_tbl ? where_tbl
		   : (join_tbl ? join_tbl : slct->from_tbl));
  else
    {
      res_tbl = table_project ( where_tbl ? where_tbl : slct->from_tbl,
				"tmp_res_01", slct->num_attrs, slct->attrs);
      table_display (res_tbl);
    }

  remove_table (join_tbl);
  remove_table (where_tbl);
  remove_table (res_tbl);
  release_select_desc (slct);
}


schema_p test_schema(const char *name){
  
  int num_attrs = 2;
  int attr_types[2];
  attr_types[0] = INT_TYPE;
  attr_types[1] = STR_TYPE;
  remove_schema(get_schema(name));



  schema_p sch = new_schema(name);


  int i;
  for(i = 0; i < num_attrs; i++){
    switch(attr_types[i]){
      case INT_TYPE:
        add_field(sch,new_int_field("id"));
        break;

      case STR_TYPE:
      add_field(sch,new_str_field("name",20));
      break;
    }
  }
  put_schema_info(DEBUG,sch);
  return sch;
}

void fill_test_record(schema_p s, record r, int id){
  char test_str[20];
  test_str[0] = 'a';
  fill_record(r, s, id, test_str);
}

void test_gen(schema_p s, record *r, int n){
  int i;
  for( i = 0; i < n; i++){
    r[i] = new_record(s);
    fill_test_record(s,r[i],i);
    put_record_info(DEBUG,r[i],s);
    append_record(r[i],s);
  }
}




void interpret ( int argc, char* argv[] )
{
  if (init_with_options (argc, argv) == -1)
    exit (EXIT_FAILURE);

  char token[30];

  int gen_data = 1;

  if(gen_data){
    record b[80];
    schema_p s = test_schema("lol");
    test_gen(s,b,80);
    //record b = new_record(s);
    //append_record (b, s);
    //release_record (b, s);
    //fill_record(b,s,0);
    //append_record(b,s);
    //release_record(b,s);
    //test_gen(s,r,30);
  }
	
  while (!feof(in_s))
    {
      if (in_s == stdin)
	printf ("db2700> ");
      fscanf (in_s, "%s", token);
      put_msg (DEBUG, "current token is \"%s\".\n", token);
      if (strcmp(token, t_quit) == 0)
	{ quit(); break;}
      if (token[0] == '#')
	{ skip_line (); continue; }
      if (strcmp(token, t_help) == 0)
	{ show_help_info (); continue; }
      if (strcmp(token, t_show) == 0)
	{ show_database(); continue; }
      if (strcmp(token, t_print) == 0)
	{ print_str(); continue; }
      if (strcmp(token, t_create) == 0)
	{ create_tbl(); continue; }
      if (strcmp(token, t_drop) == 0)
	{ drop_tbl(); continue; }
      if (strcmp(token, t_insert) == 0)
	{ insert_row(); continue; }
      if (strcmp(token, t_select) == 0)
	{ select_rows(); continue; }
      syntax_error (token);
    }
}

