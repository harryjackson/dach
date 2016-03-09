#include "fdb.h"
#define SQLITE_API static
#define SQLITE_THREADSAFE 1
#include "sqlite3.h"
#include <assert.h>
#include <apr-1/apr_pools.h>
#include <pthread.h>
#include <apr-1/apr_thread_pool.h>
#include <apr-1/apr_thread_mutex.h>
#include <fcntl.h>
#include <pwd.h>
#include <uuid/uuid.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <string.h>
#include <unistd.h>
#if defined(__APPLE__)
#  define COMMON_DIGEST_FOR_OPENSSL
#  include <CommonCrypto/CommonDigest.h>
//#  define SHA1 CC_SHA1
#else
#  include <openssl/md5.h>
#endif

#define FDB_THREADS 16

const char *fdb_create_db = "\
create table if not exists fdb_files(                   \
fname         TEXT primary key not null,  \
fsize         BIGINT not null,            \
md5_partial   varchar(32) default \"0\",  \
md5_full      varchar(32) default \"0\"   \
);";

int main(int argc, const char* const argv[]) {
  assert(1 == SQLITE_THREADSAFE);
  apr_initialize();
  struct passwd *pw;
  uid_t uid;

  uid = geteuid ();
  pw = getpwuid (uid);
  if (pw) {
    printf("username=%s\n",pw->pw_name);
  }
  else {
    fprintf (stderr,"cannot find username for UID %u\n", (unsigned) uid);
    exit(EXIT_FAILURE);
  }
  assert(sqlite3_threadsafe() == 1);
  printf("command     == %s\n", argv[0]);
  printf("database    == %s\n", argv[1]);
  printf("search  dir == %s\n", argv[2]);

  int res = access(argv[2], F_OK);
  assert(res == 0);
  //printf("%s\n", strerror(res));
  if( argc!=3 ){
    fprintf(stderr, "Usage: %s DATABASE start_directory\n", argv[0]);
    return(1);
  }
  //sqlite3_int64 size = sqlite3_soft_heap_limit64(1000000);
  //printf("SQL Heap Size set to: %lld ??? \n", size);
  sqlite3 *db;
  int rc = sqlite3_open(argv[1], &db);
  if( rc != SQLITE_OK){
    //int ecode = sqlite3_errcode(db);
    //printf("Can't open database: %s\n", argv[1]);
    //fprintf(stderr, "ecode: %s\n", sqlite3_errstr(ecode));
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    assert(NULL);
    return(rc);
  }
  assert(db != NULL);

  rc = sqlite3_exec(db, fdb_create_db,NULL,NULL,NULL);
  if( rc != SQLITE_OK){
    fprintf(stderr, "Can't create database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    goto quick_exit;
  }

    rc = sqlite3_extended_result_codes(db, 1);
  if( rc != SQLITE_OK){
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    goto quick_exit;
  }

  apr_allocator_t * apr_alloc = NULL;
  apr_status_t st = apr_allocator_create(&apr_alloc);
  if(st != APR_SUCCESS) {
    fprintf(stderr, "Fatal: failed to create apr allocator: %s\n");
    goto quick_exit;
  }
 
  fdb_worker * worker = fdb_start_workers(apr_alloc, FDB_THREADS);
  worker->db = db;

  rc = walk_recur(argv[2], worker);
  printf("canceling threads\n");
  printf("Busy Count %lu\n", apr_thread_pool_busy_count(worker->apt_pool));
  rc = apr_thread_pool_tasks_cancel(worker->apt_pool, &worker->owner);
  if (rc != APR_SUCCESS) {
    fprintf(stderr, "Fatal: apr_thread_pool_tasks_cancel() == %d\n", rc);
    goto quick_exit;
  }
  rc = apr_thread_pool_destroy(worker->apt_pool);
  if (rc != APR_SUCCESS) {
    printf ("ERROR; return code from apr_thread_pol_destroy()is %d\n", rc);
  }

quick_exit:
  sqlite3_close(db);
  sqlite3_shutdown();
  return rc;
}

//static size_t io_file_size(const char *fname) {
//  assert(fname != NULL);
//  return get_file_size(fname);
//}
//
//static int io_file_exists(const char *filename) {
//  struct stat buffer;
//  return (stat (filename, &buffer) == 0);
//}

