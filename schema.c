/***********************************************************
 * Schema for assignments in the Databases course INF-2700 *
 * UIT - The Arctic University of Norway                   *
 * Author: Weihai Yu                                       *
 ************************************************************/

#include "schema.h"
#include "pmsg.h"
#include <string.h>

/** @brief Field descriptor */
typedef struct field_desc_struct {
  char *name;        /**< field name */
  field_type type;   /**< field type */
  int len;           /**< field length (number of bytes) */
  int offset;        /**< offset from the beginning of the record */
  field_desc_p next; /**< next field_desc of the table, NULL if no more */
} field_desc_struct;

/** @brief Table/record schema */
/** A schema is a linked list of @ref field_desc_struct "field descriptors".
    All records of a table are of the same length.
*/
typedef struct schema_struct {
  char *name;           /**< table name */
  field_desc_p first;   /**< first field_desc */
  field_desc_p last;    /**< last field_desc */
  int num_fields;       /**< number of fields in the table */
  int len;              /**< record length */
  tbl_p tbl;            /**< table descriptor */
} schema_struct;

/** @brief Table descriptor */
/** A table descriptor allows us to find the schema and
    run-time infomation about the table.
 */
typedef struct tbl_desc_struct {
  schema_p sch;      /**< schema of this table. */
  int num_records;   /**< number of records this table has. */
  page_p current_pg; /**< current page being accessed. */
  tbl_p next;        /**< next tbl_desc in the database. */
} tbl_desc_struct;

void put_field_info ( pmsg_level level, field_desc_p f )
{
  if ( f == NULL )
    {
      put_msg (level,  "  empty field\n");
      return;
    }	
  put_msg (level, "  \"%s\", ", f->name);
  if ( is_int_field (f) )
    append_msg (level,  "int ");
  else
    append_msg (level,  "str ");
  append_msg (level, "field, len: %d, offset: %d, ", f->len, f->offset);
  if (f->next != NULL)
    append_msg (level,  ", next field: %s\n", f->next->name);
  else
    append_msg (level,  "\n");
}

void put_schema_info ( pmsg_level level, schema_p s )
{
  if ( s == NULL )
    {
      put_msg (level,  "--empty schema\n");
      return;
    }
  field_desc_p f;
  put_msg (level, "--schema %s: %d field(s), totally %d bytes\n",
	   s->name, s->num_fields, s->len);
  for ( f = s->first; f != NULL; f = f->next)
    put_field_info (level, f);
  put_msg (level, "--\n");
}

void put_tbl_info ( pmsg_level level, tbl_p t )
{
  if ( t == NULL )
    {
      put_msg (level,  "--empty tbl desc\n");
      return;
    }
  put_schema_info (level, t->sch);
  put_file_info (level, t->sch->name);
  put_msg (level, " %d blocks, %d records\n",
	   file_num_blocks (t->sch->name), t->num_records);
  put_msg (level, "----\n");
}

void put_record_info ( pmsg_level level, record r, schema_p s )
{
  field_desc_p f;
  int i = 0;
  put_msg (level, "Record: ");
  for ( f = s->first; f != NULL; f = f->next, i++)
    {
      if ( is_int_field (f) )
	append_msg (level,  "%d", *(int *)r[i] );
      else
	append_msg (level,  "%s", (char *)r[i] );
			
      if (f->next != NULL)
	append_msg (level,  " | " );
    }
  append_msg (level,  "\n" );
}

void put_db_info ( pmsg_level level )
{
  char *db_dir = system_dir ();
  if (db_dir == NULL) return;
  put_msg (level, "======Database at %s:\n", db_dir);
  tbl_p tbl;
  for (tbl = db_tables; tbl != NULL; tbl = tbl->next)
    put_tbl_info (level, tbl);
  put_msg (level, "======\n");
}

field_desc_p new_int_field ( const char *name )
{
  field_desc_p res = (field_desc_p) malloc (sizeof (field_desc_struct));
  res->name = (char *) malloc (strlen (name) + 1);
  strcpy (res->name, name);
  res->type = INT_TYPE;
  res->len = INT_SIZE;
  res->offset = 0;
  res->next = NULL;
  return res;
}

field_desc_p new_str_field ( const char *name, int len )
{
  field_desc_p res = (field_desc_p) malloc (sizeof (field_desc_struct));
  res->name = malloc (strlen (name) + 1);
  strcpy (res->name, name);
  res->type = STR_TYPE;
  res->len = len;
  res->offset = 0;
  res->next = NULL;
  return res;
}

static void release_field_desc ( field_desc_p f )
{
  if (f == NULL)
    return;
  free (f->name);
  free (f);
}


int is_int_field ( field_desc_p f )
{
  if ( f == NULL)
    return 0;
  return (f->type == INT_TYPE);
}

field_desc_p field_desc_next ( field_desc_p f )
{
  if ( f == NULL )
    {
      put_msg (ERROR, "field_desc_next: NULL field_desc_next.\n");
      return NULL;
    }
  return f->next;
}

static schema_p make_schema ( const char *name )
{
  schema_p res = (schema_p) malloc (sizeof (schema_struct));
  res->name = (char *) malloc (strlen (name) + 1);
  strcpy (res->name, name);
  res->first = NULL;
  res->last = NULL;
  res->num_fields = 0;
  res->len = 0;
  return res;
}

/** Release the memory allocated for the schema and its field descriptors.*/
static void release_schema ( schema_p sch )
{
  if ( sch == NULL )
    return;
  field_desc_p f, nextf;
  f = sch->first;
  while (f != NULL)
    {
      nextf = f->next;
      release_field_desc (f);
      f = nextf;
    }
  free (sch->name);
  free (sch);
}

char* schema_name ( schema_p sch )
{
  if ( sch == NULL )
    {
      put_msg (ERROR, "schema_name: NULL schema.\n");
      return NULL;
    }
  return sch->name;
}

field_desc_p schema_first_fld_desc ( schema_p sch )
{
  if ( sch == NULL )
    {
      put_msg (ERROR, "schema_first_fld_desc: NULL schema.\n");
      return NULL;
    }
  return sch->first;
}

field_desc_p schema_last_fld_desc ( schema_p sch )
{
  if ( sch == NULL )
    {
      put_msg (ERROR, "schema_last_fld_desc: NULL schema.\n");
      return NULL;
    }
  return sch->last;
}

int schema_num_flds ( schema_p sch )
{
  if ( sch == NULL )
    {
      put_msg (ERROR, "schema_num_flds: NULL schema.\n");
      return -1;
    }
  return sch->num_fields;
}

int schema_len ( schema_p sch )
{
  if ( sch == NULL )
    {
      put_msg (ERROR, "schema_len: NULL schema.\n");
      return -1;
    }
  return sch->len;
}

const char tables_desc_file[] = "db.db";

static void save_tbl_desc ( FILE *fp, tbl_p tbl )
{
  schema_p sch = tbl->sch;
  fprintf ( fp, "%s %d\n", sch->name, sch->num_fields );
  field_desc_p fld = schema_first_fld_desc ( sch );
  while ( fld != NULL )
    {
      fprintf (fp, "%s %d %d %d\n",
	       fld->name, fld->type, fld->len, fld->offset);
      fld = fld->next;
    }
  fprintf (fp, "%d\n", tbl->num_records);
}

static void save_tbl_descs ()
{
  tbl_p tbl = db_tables, next_tbl = NULL;
  char tbl_desc_backup[30] = "__";
  rename ( tables_desc_file, strcat (tbl_desc_backup, tables_desc_file));
  FILE *dbfile = fopen (tables_desc_file, "w+");
  while (tbl)
    {
      save_tbl_desc ( dbfile, tbl);
      release_schema (tbl->sch);
      next_tbl = tbl->next;
      free (tbl);
      tbl = next_tbl;
    }
  fclose (dbfile);
}

static void read_tbl_descs ()
{
  
  FILE *fp = fopen (tables_desc_file, "r");
  if ( fp == NULL ) return;
  char name[30] = "";
  schema_p sch;
  field_desc_p fld;
  int num_flds = 0, fld_type, fld_len;
  while ( !feof(fp) )
    {
      if ( fscanf ( fp, "%s %d\n", name, &num_flds ) < 2 )
	{
	  fclose (fp);
	  return;
	}
      sch = new_schema ( name );
      int i;
      for (i = 0; i < num_flds; i++ )
	{
	  fscanf ( fp, "%s %d %d", name, &(fld_type), &(fld_len) );
	  switch (fld_type) {
	  case INT_TYPE:
	    fld = new_int_field ( name );
	    break;
	  case STR_TYPE:
	    fld = new_str_field ( name, fld_len );
	    break;
	  }
	  fscanf (fp, "%d\n", &(fld->offset));
	  add_field ( sch, fld );
	}
      fscanf ( fp, "%d\n", &(sch->tbl->num_records) );
    }
  db_tables = sch->tbl;
  fclose (fp);
}

int open_db ( void )
{
  pager_terminate (); /* first clean up for a fresh start */
  pager_init ();
  read_tbl_descs ();
  return 0;
}

void close_db ( void )
{ 
  save_tbl_descs ();
  db_tables = NULL;
  pager_terminate ();
}

schema_p new_schema ( const char *name )
{
  tbl_p tbl = (tbl_p) malloc (sizeof (tbl_desc_struct));
  tbl->sch = make_schema ( name );
  tbl->sch->tbl = tbl;
  tbl->num_records = 0;
  tbl->current_pg = NULL;
  tbl->next = db_tables;
  db_tables = tbl;
  return tbl->sch;
}

tbl_p get_table ( const char *name )
{
  tbl_p tbl;
  for (tbl = db_tables; tbl != NULL; tbl = tbl->next)
    if ( strcmp (name, tbl->sch->name) == 0 )
      return tbl;
  return NULL;
}

schema_p get_schema ( const char *name )
{
  tbl_p tbl = get_table ( name );
  if ( tbl == NULL)
    return NULL;
  else
    return tbl->sch;
}

void remove_table ( tbl_p t )
{
  if (t == NULL) return;

  tbl_p tbl, prev = NULL;
  for (tbl = db_tables; tbl != NULL; prev = tbl, tbl = tbl->next)
    if ( tbl == t )
      {
	if (t == db_tables)
	  db_tables = t->next;
	else
	  prev->next = t->next;
	close_file (t->sch->name);
	char tbl_backup[30] = "__";
	rename ( t->sch->name, strcat (tbl_backup, t->sch->name) );
	release_schema (t->sch);
	free (t);
	return;
      }
}

void remove_schema ( schema_p s )
{
  if (s == NULL) return;
  remove_table (s->tbl);
}

static field_desc_p copy_field ( field_desc_p f )
{
  field_desc_p res = (field_desc_p) malloc (sizeof (field_desc_struct));
  res->name = (char *) malloc (strlen (f->name) + 1);
  strcpy (res->name, f->name);
  res->type = f->type;
  res->len = f->len;
  res->offset = 0;
  res->next = NULL;
  return res;  
}

static schema_p copy_schema ( schema_p s, const char *dest_name )
{
  if (s == NULL ) return NULL;
  schema_p dest = new_schema (dest_name);
  field_desc_p f = s->first;
  for ( ; f != NULL; f = f->next)
    add_field (dest, copy_field (f));
  return dest;
}

static field_desc_p get_field ( schema_p s, char *name )
{
  field_desc_p f = s->first;
  for ( ; f != NULL; f = f->next)
    if ( strcmp ( f->name, name ) == 0 ) return f;
  return NULL;
}

static schema_p make_sub_schema ( schema_p s, const char *dest_name,
				  int num_fields, char *fields[] )
{
  if (s == NULL ) return NULL;
  schema_p dest = new_schema (dest_name);
  field_desc_p f = NULL;
  int i = 0;
  for ( i= 0; i < num_fields; i++)
    {
      f = get_field ( s, fields[i] );
      if ( f != NULL)
	add_field (dest, copy_field (f));
      else
	{
	  put_msg (ERROR, "\"%s\" has no \"%s\" field\n",
		   s->name, fields[i]);
	  remove_schema (dest);
	  return NULL;
	}
    }
  return dest;
}

int add_field ( schema_p s, field_desc_p f )
{
  if ( s == NULL )
    return -1;
  if ( s->len + f->len > BLOCK_SIZE - PAGE_HEADER_SIZE )
    {
      put_msg (ERROR,
	       "schema already has %d bytes, adding %d will exceed limited %d bytes.\n",
	       s->len, f->len, BLOCK_SIZE - PAGE_HEADER_SIZE);
      return -1;
    }
  if ( s->num_fields == 0)
    {
      s->first = f;
      f->offset = 0;
    }
  else
    {
      s->last->next = f;
      f->offset = s->len;
    }
  s->last = f;
  s->num_fields++;
  s->len += f->len;
  return s->num_fields;
}

record new_record ( schema_p s )
{
  if ( s == NULL )
    {
      put_msg (ERROR,  "new_record: NULL schema!\n");
      exit (EXIT_FAILURE);
    }
  record res = malloc (sizeof(void *) * s->num_fields);
	
  /* allocate memory for the fields */
  field_desc_p f;
  int i = 0;
  for ( f = s->first; f != NULL; f = f->next, i++)
    {
	res[i] =  malloc (f->len);
    }
  return res;
}

void release_record ( record r, schema_p s )
{
  if ( r == NULL || s == NULL )
    {
      put_msg (ERROR,  "release_record: NULL record or schema!\n" );
      return;
    }
  int i;
  for ( i = 0; i < s->num_fields; i++ )
    {
      free ( r[i] );
    }
  free (r);
}

void assign_int_field ( void *field_p, int int_val )
{
  *(int *)field_p = int_val;
}
	
void assign_str_field ( void *field_p, char *str_val )
{
  strcpy (field_p, str_val);
}

int fill_record ( record r, schema_p s, ...)
{
  if ( r == NULL || s == NULL )
    {
      put_msg (ERROR,  "fill_record: NULL record or schema!\n" );
      return -1;
    }
  va_list vals;
  va_start (vals, s);
  field_desc_p f;
  int i = 0;
  for ( f = s->first; f != NULL; f = f->next, i++)
    {
      if ( is_int_field (f) )
	assign_int_field (r[i], va_arg (vals, int));
      else
	assign_str_field (r[i], va_arg (vals, char*));
    }
  return 0;
}

static void fill_sub_record ( record dest_r, schema_p dest_s,
			     record src_r, schema_p src_s)
{
  field_desc_p src_f, dest_f;
  int i = 0, j = 0;
  for ( dest_f = dest_s->first; dest_f != NULL; dest_f = dest_f->next, i++ )
    {
      for (j = 0, src_f = src_s->first;
	   strcmp ( src_f->name, dest_f->name ) != 0;
	   j++, src_f = src_f->next)
	;
      if ( is_int_field (dest_f) )
	assign_int_field (dest_r[i], *(int *)src_r[j]);
      else
	assign_str_field (dest_r[i], (char *)src_r[j]);
    }
}

int equal_record ( record r1, record r2, schema_p s )
{
  if ( r1 == NULL || r2 == NULL || s == NULL )
    {
      put_msg (ERROR,  "equal_record: NULL record or schema!\n" );
      return 0;
    }

  field_desc_p fd;
  int i = 0;;
  for ( fd = s->first; fd != NULL; fd = fd->next, i++)
    {
      if ( is_int_field (fd) )
	{
	  if (*(int *)r1[i] != *(int *)r2[i])
	    return 0;
	}
      else
	{
	  if ( strcmp ((char *)r1[i], (char *)r2[i]) != 0 )
	    return 0;
	}
    }
  return 1;
}

void set_tbl_position ( tbl_p t, tbl_position pos )
{
  switch (pos) {
  case TBL_BEG:
    {
      t->current_pg = get_page (t->sch->name, 0);
      page_set_pos_begin ( t-> current_pg );
    }
    break;
  case TBL_END:
    t->current_pg = get_page_for_append ( t->sch->name );
  }
}

int eot ( tbl_p t )
{
  return ( peof(t->current_pg) );
}

/** check if the the current position is valid */
static int page_valid_pos_for_get_with_schema ( page_p p, schema_p s )
{
  return (page_valid_pos_for_get (p, page_current_pos(p))
	  && (page_current_pos(p) - PAGE_HEADER_SIZE) % s->len == 0);
}

/** check if the the current position is valid */
static int page_valid_pos_for_put_with_schema ( page_p p, schema_p s )
{
  return (page_valid_pos_for_put (p, page_current_pos(p), s->len)
	  && (page_current_pos(p) - PAGE_HEADER_SIZE) % s->len == 0);
}

static page_p get_page_for_next_record ( schema_p s )
{
  page_p pg = s->tbl->current_pg;
  if (peof(pg)) return NULL;
  if (eop(pg))
    {
      unpin (pg);
      pg = get_next_page (pg);
      if ( pg == NULL)
	{
	  put_msg (FATAL, "get_page_for_next_record failed at block %d\n",
		   page_block_nr(pg) + 1);
	  exit (EXIT_FAILURE);
	}
      page_set_pos_begin (pg);
      s->tbl->current_pg = pg;
    }
  return pg;
}

static int get_page_record ( page_p p, record r, schema_p s )
{
  if ( p == NULL ) return 0;
  if (!page_valid_pos_for_get_with_schema (p, s))
    {
      put_msg (FATAL, "try to get record at invalid position.\n");
      exit (EXIT_FAILURE);
    }
  field_desc_p fld_desc;
  int i = 0;
  for ( fld_desc = s->first; fld_desc != NULL;
	fld_desc = fld_desc->next, i++)
    if ( is_int_field (fld_desc) )
      assign_int_field (r[i], page_get_int(p));
    else
      page_get_str(p, r[i], fld_desc->len);
  return 1;
}

int get_record ( record r, schema_p s )
{
  page_p pg = get_page_for_next_record (s);
  if ( pg == NULL ) return 0;
  return get_page_record ( pg, r, s );
}

static int int_equal ( int x, int y )
{
  return x == y;
}

int find_record_int_val ( record r, schema_p s, int offset,
			  int (*op) (int, int), int val)
{
  put_record_info(DEBUG, r,s);
  page_p pg = get_page_for_next_record (s);
  if ( pg == NULL ) return 0;
  int pos, rec_val;
  for ( ; pg != NULL; pg = get_page_for_next_record (s) )
    {
      pos = page_current_pos (pg);
      rec_val = page_get_int_at (pg, pos + offset);
      if ( (*op) (val, rec_val) )
	{
	  page_set_current_pos (pg, pos);
	  get_page_record ( pg, r, s );
	  return 1;
	}
      else
	page_set_current_pos (pg, pos + s->len);
    }
  return 0;
}

static int put_page_record ( page_p p, record r, schema_p s )
{
  if (!page_valid_pos_for_put_with_schema (p, s))
    return -1;

  field_desc_p fld_desc;
  int i = 0;
  for ( fld_desc = s->first; fld_desc != NULL;
	fld_desc = fld_desc->next, i++)
    if ( is_int_field (fld_desc) )
      page_put_int(p, *(int *)r[i]);
    else
      page_put_str(p, (char *)r[i], fld_desc->len);
  return 0;
}

int put_record ( record r, schema_p s )
{
  page_p p = s->tbl->current_pg;

  if (!page_valid_pos_for_put_with_schema (p, s))
    return -1;

  field_desc_p fld_desc;
  int i = 0;
  for ( fld_desc = s->first; fld_desc != NULL;
	fld_desc = fld_desc->next, i++)
    if ( is_int_field (fld_desc) )
      page_put_int(p, *(int *)r[i]);
    else
      page_put_str(p, (char *)r[i], fld_desc->len);
  return 0;
}

int append_record ( record r, schema_p s )
{
  tbl_p tbl = s->tbl;
  page_p pg = get_page_for_append ( s->name );
  if (pg == NULL)
    {
      put_msg (FATAL, "Failed to get page for appending to \"%s\".\n",
	       s->name);
      exit (EXIT_FAILURE);
    }
  if ( put_page_record (pg, r, s) == -1 )
    {
      /* not enough space in the current page */
      unpin (pg);
      pg = get_next_page ( pg );
      if (pg == NULL)
	{
	  put_msg (FATAL, "Failed to get page for \"%s\" block %d.\n",
		   s->name, page_block_nr(pg) + 1);
	  exit (EXIT_FAILURE);
	}
      if ( put_page_record (pg, r, s) == -1 )
	{
	  put_msg (FATAL, "Failed to put record to page for \"%s\" block %d.\n",
		   s->name, page_block_nr(pg) + 1);
	  exit (EXIT_FAILURE);
	}
    }
  tbl->current_pg = pg;
  tbl->num_records++;
  return 0;
}

static void display_tbl_header ( tbl_p t )
{
  if ( t == NULL )
    {
      put_msg (INFO,  "Trying to display non-existant table.\n");
      return;
    }
  schema_p s = t->sch;
  field_desc_p f;
  for ( f = s->first; f != NULL; f = f->next)
    put_msg (FORCE, "%20s", f->name);
  put_msg (FORCE, "\n");
  int i;
  for ( f = s->first; f != NULL; f = f->next)
    {
      for ( i = 0; i < 20 - strlen(f->name); i++)
	put_msg (FORCE, " ");
      for ( i = 0; i < strlen(f->name); i++)
	put_msg (FORCE, "-");
    }
  put_msg (FORCE, "\n");
}

static void display_record ( record r, schema_p s )
{
  field_desc_p f = s->first;
  int i = 0;
  for ( ; f != NULL; f = f->next, i++ )
    {
      if ( is_int_field (f) )
	put_msg (FORCE, "%20d", *(int *)r[i] );
      else
	put_msg (FORCE, "%20s", (char *)r[i] );
    }
  put_msg (FORCE, "\n");
}

void table_display ( tbl_p t )
{
  if (t == NULL) return;
  display_tbl_header (t);
  
  schema_p s = t->sch;
  record rec = new_record ( s );
  set_tbl_position ( t, TBL_BEG );
  while ( get_record (rec, s) )
    {
      display_record (rec, s);
    }
  put_msg (FORCE, "\n");

  release_record (rec, s);
}

int binary_search_int(tbl_p t, int val_to_find, record rec){
  //page_p ok = pages[0];
  //put_msg(DEBUG, "Numb blocks %d\n",file_num_blocks(t->sch->name));
  //put_msg(DEBUG, "PAGES[0]");

  //put_page_info(DEBUG,ok);
  //return 0;
  put_msg(DEBUG, "IN BINARY SEARCH\n");
  put_msg(DEBUG, " Num records: %d\n",t->num_records);        // Number of records in the table 

  schema_p s = t->sch;                                        // Fetch schema 
  put_record_info(DEBUG,rec,s);
  put_msg(DEBUG, "Record len: %d\n",s->len);                  // s->len =  Length of each record 
  int page_data = 500 - PAGE_HEADER_SIZE;                     // The real size of a page - the header start of 20 bytes PAGE_HEADER_SIZE
 
  int num_pages = ((t->num_records * s->len) + (page_data-1)) / page_data; // The number of pages the records in the table span, the additional calculations are for solving a num_page result of 3.5. Which should equal to 4 pages.  
  if(num_pages < 1){    // Cant be a zero page span of records. Must be atleast 1 page. 
    num_pages = 1;
  }
  num_pages = file_num_blocks(s->name);
  int max_pages = file_num_blocks(s->name);
  put_msg(DEBUG,"NUM PAGES: %d\n",num_pages);
  put_msg(DEBUG, "Table records span %d pages\n",num_pages);

  int page_to_search = num_pages / 2;                       // The page to start searching in.  
  
  put_msg(DEBUG, "Page to search: %d\n",page_to_search);

  
  int pos = 0;                        // Holds current position of the page   
  int value = 0;                      // Holds the found value 
  int found = 0;                      // Integer for breaking out of while loop, used as a boolean 
  int last_searched_page = 0;         // The last page we searched.
  int same_page_search_count = 0;     // If this is bigger than 1 we are reading the same page over and over, which means the value is not in the database.
  
  while(!found){
    //page_p pg = get_page(s->name,page_to_search);             // Fetch the page we want to search through 
    page_p pg = get_page(s->name,page_to_search);
    last_searched_page = page_to_search;
    put_msg(DEBUG, "Page pos: %d\n",page_current_pos(pg));
    put_page_info(DEBUG,pg);
    
    page_set_current_pos(pg,PAGE_HEADER_SIZE);              // Set position to beginning of the page, which is right after the header data, 20
    pos = page_current_pos(pg);                             // Fetch the current position 
    
    //while(pos != 500){                                      // Search through the page until the entire page has been read. 
      while(eop(pg) != 1){
      pos = page_current_pos(pg);                           // Fetch position again 
      if(pos == 500){                                       // If we have read all records in page
        break;                                              // Leave loop 
      }
      value = page_get_int_at(pg,pos);                      // Get the value of the integer 
       
      if(value != val_to_find){                             // If the found value is not the one we are looking for 
        page_set_current_pos(pg, pos + s->len);             // Move to next record, 24 = record length 

      }else{
        put_msg(DEBUG, "Found the wanted value\n");         // We have found our value 
        found = 1;
        page_set_current_pos(pg,pos);                       // Set position 
        get_page_record(pg, rec, s);                        // fetch the record 
        put_page_info(DEBUG,pg);                            
        return 1;                                           // Leave loop 
      }
    }
    if(pos == 500 || eop(pg) == 1) {                                         // This means that the record we want is in a different page 
      put_msg(DEBUG, "Wanted record is in a different page\n");
      
      put_msg(DEBUG, "VAL TO FIND: %d\n",val_to_find);
      put_msg(DEBUG, "VAL FOUND: %d\n",value);
      if(val_to_find > value){                              // If the value we want to find is bigger than the last value of the page we searched, it means that that the value is in a higher page. 
        int pages_left  = num_pages - page_to_search;     // Remaining number of pages - the one we just read. 
        int search_page = (pages_left + (2-1)) / 2;           // Finding the numebr of the page we want to search - In relvancy of the initial num_pages. 
        page_to_search = page_to_search + search_page;        // Calculating the page we want to search 

        put_msg(DEBUG, "Pages left: %d\n",pages_left);
        put_msg(DEBUG, "Serach_page: %d\n",search_page);        
        put_msg(DEBUG, "PAGE TO SEARCH: %d\n",page_to_search);
      }else if(val_to_find < value){                          // If the value we want is smaller than the last value of the page we searched, it means that the value is in a lower page. 
        //int pages_left = num_pages - page_to_search - 1;      // Do the same as above, we calculate which page we want to search through. 
        int pages_left = page_to_search - 1;
        int search_page = (pages_left + (2-1)) / 2;
        num_pages = pages_left;
        put_msg(DEBUG, "Pages left: %d\n",pages_left);
        put_msg(DEBUG, "Serach_page: %d\n",search_page);
        put_msg(DEBUG, "PRE page_to_search %d\n",page_to_search);
        //page_to_search = page_to_search - search_page; // This might be wrong 
        page_to_search = search_page;
        put_msg(DEBUG, "page_to_search %d\n",page_to_search);
        if(page_to_search < 0){                               // If for some reason the calculation bugs out and sets a negative page number, we simply set the page to page 0. 
          page_to_search = 0;
        }

        put_msg(DEBUG, "PAGE TO SEARCH: %d\n",page_to_search);
      } 

        if(page_to_search == last_searched_page){
          put_msg(DEBUG, "VALUE NOT IN DATABSE\n");
          put_page_info(DEBUG,pg);
          
          break;
        }
    }
  }


  
//b+ low depth, so low amount of reads and seeks 
// seek slower than read 

  put_msg(DEBUG, "LEAVING BINARY SEARCH\n");
  

  return 0;
}

int find_record_all(record rec, schema_p s, int offset, int val, const char* op){
    page_p pg = get_page_for_next_record(s);      
  if(pg == NULL){
    return 0;
  }

  const char* t_lessEqual   = "<=";
  const char* t_biggerEqual = ">=";
  const char* t_less        = "<";
  const char* t_bigger      = ">";
  const char* t_notEqual    = "!=";

  int pos, rec_val;
  for( ; pg != NULL; pg = get_page_for_next_record(s) ){ // Loop through each record 
    pos = page_current_pos(pg);                           // Get position
    rec_val = page_get_int_at(pg, pos + offset);          // Get value 

    if(strcmp(op,t_notEqual) == 0){
      if(val != rec_val){                                   // If the found value is not equal to the value specified, we add it 
        page_set_current_pos(pg, pos);                      // Move position 
        get_page_record(pg, rec, s);                        // Get the record 
        return 1;                                           // Return 
      }else{
        page_set_current_pos(pg, pos + s->len);             
      }
    }else if(strcmp(op,t_bigger) == 0){
      if(val < rec_val){                                    // If the found value is not equal to the value specified, we add it 
        page_set_current_pos(pg, pos);                      // Move position 
        get_page_record(pg, rec, s);                        // Get the record 
        put_page_info(DEBUG,pg);
        return 1;                                           // Return 
      }else{
        page_set_current_pos(pg, pos + s->len);             
      }
    }else if(strcmp(op,t_less) == 0){
      if(val > rec_val){                                   // If the found value is not equal to the value specified, we add it 
        page_set_current_pos(pg, pos);                      // Move position 
        get_page_record(pg, rec, s);                        // Get the record 
        return 1;                                           // Return 
      }else{
        page_set_current_pos(pg, pos + s->len);             
      }
    }else if(strcmp(op,t_biggerEqual) == 0){
      if(val <= rec_val){                                   // If the found value is not equal to the value specified, we add it 
        page_set_current_pos(pg, pos);                      // Move position 
        get_page_record(pg, rec, s);                        // Get the record 
        return 1;                                           // Return 
      }else{
        page_set_current_pos(pg, pos + s->len);             
      }
    }else if(strcmp(op,t_lessEqual) == 0){
      if(val >= rec_val){                                   // If the found value is not equal to the value specified, we add it 
        page_set_current_pos(pg, pos);                      // Move position 
        get_page_record(pg, rec, s);                        // Get the record 
        return 1;                                           // Return 
      }else{
        page_set_current_pos(pg, pos + s->len);             
      }
    }
    
  }
  return 0;
}




tbl_p table_search (tbl_p t, char *attr, char *op, int val)
{
  if (t == NULL) return NULL;

  int search_binary = 1;  // Set to 0 for linear search or 1 for binary search on equality = 

  put_msg(DEBUG, " Number of records in table: %d %d\n",t->num_records,t[4]);
  schema_p s = t->sch;
  field_desc_p f;
  int i = 0;
  for ( f = s->first; f != NULL; f = f->next, i++)
    if ( strcmp (f->name, attr) == 0)
      {
	if ( f->type != INT_TYPE )
	  {
	    put_msg (ERROR, "\"%s\" is not an integer field.\n", attr);
	    return NULL;
	  }
	break;
      }
  if ( f == NULL ) return NULL;

  char tmp_name[30] = "tmp_tbl__";  
  strcat (tmp_name, s->name);
  schema_p res_sch = copy_schema ( s, tmp_name );
  record rec = new_record ( s );

  set_tbl_position ( t, TBL_BEG );
  // Check the op for <,<=, >, >=, != , if so we call our own function
  if(op[0] == '<' || op[0] == '>' || op[0] == '!'){ // The op contains atleast <, > or ! 
    put_msg(DEBUG," <, >, or ! operator\n");
    put_msg(DEBUG, "%s\n", op);
    
                                     
    while(find_record_all(rec,s,f->offset,val,op)){ // For operators not '='
       put_record_info(DEBUG,rec,s);
       append_record(rec, res_sch);
    }
  }else{ 
    if(search_binary){                        // If we want a binary search do it 
      int res = binary_search_int(t,val,rec);
      if(res){
        put_pager_profiler_info(DEBUG);
        put_record_info(DEBUG,rec,s);
        append_record(rec,res_sch);
      }      
    }
    else{                                     // For linear search 

      while ( find_record_int_val (rec, s, f->offset, int_equal, val) ) // Equality search, attr = val
      { 
        put_page_info(DEBUG, t->current_pg);
        put_record_info (DEBUG, rec, s);
        append_record ( rec, res_sch );
        //put_pager_profiler_info(DEBUG);
      }
      
    }
  }


  

  release_record (rec, s);

  return res_sch->tbl;
}

tbl_p table_project ( tbl_p t, const char *dest_name,
		      int num_fields, char *fields[] )
{
  schema_p s = t->sch;
  schema_p dest = make_sub_schema ( s, dest_name, num_fields, fields );
  if ( dest == NULL ) return NULL;

  record rec = new_record ( s ), rec_dest = new_record ( dest );

  set_tbl_position ( t, TBL_BEG );
  while ( get_record ( rec, s ) ) 
    {
      fill_sub_record ( rec_dest, dest, rec, s );
      put_record_info (DEBUG, rec_dest, dest);
      append_record ( rec_dest, dest );
    }

  release_record (rec, s);
  release_record (rec_dest, dest);

  return dest->tbl;
}

tbl_p table_natural_join (tbl_p left, tbl_p right)
{
  return NULL;
}
