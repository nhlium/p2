#ifndef _TESTSCHEMA_H_
#define _TESTSCHEMA_H_

#include "schema.h"

extern void test_tbl_write ( const char *tbl_name );
extern void test_tbl_read ( const char *tbl_name );
extern void test_tbl_natural_join ( const char *my_tbl, const char *yr_tbl );

#endif
