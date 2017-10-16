#include <string.h>
#include "testschema.h"
#include "test_data_gen.h"
#include "pmsg.h"

#define NUM_RECORDS 1000

/* The records generated in test_tbl_write().
   Will be used in test_tbl_read() to check for correctness. */
record in_recs[NUM_RECORDS];


void test_tbl_write ( const char *tbl_name )
{	
  put_msg (INFO, "test_tbl_write (\"%s\") ...\n", tbl_name);

  open_db ();
  /* put_pager_info (DEBUG, "After open_db"); */

  char id_attr[11] = "Id", str_attr[11] = "Str"; 
  char *attrs[] = {strcat(id_attr, tbl_name),
		   strcat(str_attr,tbl_name),
		   "Int"};
  int attr_types[] = {INT_TYPE, STR_TYPE, INT_TYPE};    
  schema_p sch = create_test_schema ( tbl_name, 3, attrs, attr_types );
			
  test_data_gen ( sch, in_recs, NUM_RECORDS );

  int rnr;
  for (rnr = 0; rnr < NUM_RECORDS; rnr++)
  {
    /* put_msg (DEBUG, "rnr %d  ", rnr); */
    append_record (in_recs[rnr], sch);
    /* put_pager_info (DEBUG, "After writing a record"); */
  }

  /* put_pager_info (DEBUG, "Before page_terminate"); */
  put_db_info (DEBUG);
  close_db ();
  /* put_pager_info (DEBUG, "After close_db"); */
	
  put_pager_profiler_info (INFO);
  put_msg (INFO,  "test_tbl_write() done.\n\n");
}

void test_tbl_read ( const char *tbl_name )
{
  put_msg (INFO,  "test_tbl_read (\"%s\") ...\n", tbl_name);
	
  open_db ();
  /* put_pager_info (DEBUG, "After open_db"); */
	
  schema_p sch = get_schema ( tbl_name );
  tbl_p tbl = get_table (tbl_name);
  record out_rec = new_record ( sch );
  set_tbl_position ( tbl, TBL_BEG );
  int rnr = 0;

  while ( !eot(tbl) )
    {
      get_record (out_rec, sch);
      if ( !equal_record( out_rec, in_recs[rnr], sch) )
	{
	  put_msg (FATAL, "test_tbl_read:\n");
	  put_record_info (FATAL, out_rec, sch);
	  put_msg (FATAL, "should be:\n");
	  put_record_info (FATAL, in_recs[rnr], sch);
	  exit (EXIT_FAILURE);
	}
      release_record (in_recs[rnr++], sch);
      /* put_pager_info (DEBUG, "After reading a record"); */
    }

  if ( rnr != NUM_RECORDS )
    put_msg (ERROR, "only %d of %d records read", rnr, NUM_RECORDS);

  release_record (out_rec, sch);

  /* put_pager_info (DEBUG, "Before page_terminate"); */
  put_pager_profiler_info (INFO);
  close_db ();
  /* put_pager_info (DEBUG, "After close_db"); */
	
  put_msg (INFO,  "test_tbl_read() succeeds.\n");
	
}
