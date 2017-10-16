#ifndef _TESTPAGER_H_
#define _TESTPAGER_H_

#include "pager.h"

extern void test_page_write ( const char *fname );
extern void test_page_read ( const char *fname );
extern void test_page_write_with_offset ( const char *fname );
extern void test_page_read_with_offset ( const char *fname );

#endif