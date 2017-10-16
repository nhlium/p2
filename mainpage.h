/** @mainpage A Tiny DBMS for Assignments in INF-2700 
 * The data structures and functions in this document will
 * be used in the programming assignments of the Database course INF-2700.

 * A database table, consisting of a sequence of data @em records (of fixed length),
 * is stored in a @em file.
 * Physically on disk, a file consists of a number of disk @em blocks
 * and the data records are stored in these blocks.
 * The structure of records are defined in the @ref schema.h "schema" of the table.

 * To process data in a table, the data blocks are read into buffer
 * @em pages in the maim memory. 
 * A @ref pager.h "pager" provides the functions to work with
 * the pages.
 *
 * A database user can run SQL-like commands through a
 * @ref front.h "front end".
 *
 * Programming physical data management involves tremendous efforts in
 * memory management and debugging. For most of the data structures, you
 * can find useful @ref pmsg.h "print operations" at your disposal.
 *
 * To learn how to use the pager, schema and record, 
 * read the source code in @ref testschema.c and @ref interpreter.c.
*/
