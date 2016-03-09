#include "dch_md5.h"
#include <assert.h>
#include <inttypes.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>


#define NUM_THREADS 32

static int barrier       = 0;
static int barrier_enwik = 0;

static uint8_t test_res_bin[7][16] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static uint8_t test_res_enwik8[7][16] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};


static const char *const test[7] = {
  "" , /*d41d8cd98f00b204e9800998ecf8427e*/
  "a", /*0cc175b9c0f1b6a831c399e269772661*/
  "abc"           , /*900150983cd24fb0d6963f7d28e17f72*/
  "message digest", /*f96b697d7cb7938d525a2f31aaf161d0*/
  "abcdefghijklmnopqrstuvwxyz"                                    , /*c3fcd3d76192e4007dfb496cca67e13b*/
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789", /*d174ab98d277d9f5a5611c2c9f419d9f*/
  "12345678901234567890123456789012345678901234567890123456789012345678901234567890" /*57edf4a22be3c955ac49da2e2107b67a*/
};
static const char *const test_res[7] = {
  "d41d8cd98f00b204e9800998ecf8427e",
  "0cc175b9c0f1b6a831c399e269772661",
  "900150983cd24fb0d6963f7d28e17f72",
  "f96b697d7cb7938d525a2f31aaf161d0",
  "c3fcd3d76192e4007dfb496cca67e13b",
  "d174ab98d277d9f5a5611c2c9f419d9f",
  "57edf4a22be3c955ac49da2e2107b67a"
};

static void * thread_test(void *bar);
static void * md5_file(void *filename);
static void   print_digest(void * v_digest);
static void   ms_sleep(int ms);

static void ms_sleep(int ms) {
  struct timespec waittime;

  waittime.tv_sec = (ms / 1000);
  ms = ms % 1000;
  waittime.tv_nsec = ms * 1000 * 1000;

  nanosleep( &waittime, NULL);
}


static void * md5_file(void *filename) {

  //printf("self=%p\n", pthread_self());
  const char *file = (const char *)filename;
  FILE *fh = fopen(file, "rb");
  assert(fh != NULL);

  size_t i = 0;
  while(i++ < 1) {
    md5_state_t state;
    md5_byte_t digest[16];
    md5_init(&state);
    char buf[1000];

    while (fread(buf, 1, 1000, fh) == 1000) {
      md5_append(&state, (const md5_byte_t *)buf, 1000);
    }
    fseek(fh, 0, 0);
    md5_finish(&state, digest);
    if(barrier_enwik == 0) {
      memcpy(&test_res_enwik8[0], digest, 16);
    }
    int r = memcmp(&test_res_enwik8[0], &digest, 16);
    assert(r == 0);
  }
  fclose(fh);

  //printf("\n");
  //print_digest(digest);
  //printf("\n");
  //print_digest(&test_res_enwik8[0]);
  return 0;
}

static void print_digest(void * v_digest) {
  md5_byte_t digest[16];
  memcpy(digest, v_digest, 16);
  int di = 0;
  for (di = 0; di < 16; ++di) {
    printf("%02x", digest[di]);
  }
}


static void * thread_test(void *bar) {
  int barrier = *(int*)bar;
  assert(barrier <= 1 && barrier >= 0);

  int i = 0;
  for (i = 0; i < 7; ++i) {
    md5_state_t state;
    md5_byte_t digest[16];

    md5_init(&state);
    md5_append(&state, (const md5_byte_t *)test[i], strlen(test[i]));
    md5_finish(&state, digest);

    if(barrier == 0) {
      memcpy(&test_res_bin[i], digest, 16);
      /*
       printf("MD5==");
       print_digest(digest);
       printf("==\n");
       printf("RES==");
       print_digest(&test_res_bin[i]);
       printf("==\n");
       */
      continue;

    }
    //printf("MD5 (\"%s\") = ", test[i]);

    /*
     printf("MD5==");
     print_digest(digest);
     printf("==\n");
     printf("RES==");
     print_digest(&test_res_bin[i]);
     printf("==\n");
     */
    if(barrier > 0) {
      int r = memcmp(&test_res_bin[i], digest, 16);
      if(r != 0) {
        printf("\nRES_bin==");
        print_digest(digest);
        printf("\n");
        printf("\nRES_bin==");
        print_digest(&test_res_bin[i]);
        printf("\n");
      }
      assert(r == 0);
    }
  }
  //thread_test(&barrier);
  //pthread_exit(NULL);
  return 0;
}

int main() {
  barrier = 0;

  const char *enwik = "/git/hutter/tmp/enwik8";

  md5_file(enwik);
  thread_test(&barrier);
  barrier       = 1;
  barrier_enwik = 1;

  pthread_t threads[NUM_THREADS];
  int rc;
  long t;
  int runs = 2;
  int n = 0;
  for(t = 0; t < NUM_THREADS; t++){
    //for(n = 0; n < runs ; n++) {
      //printf("In main: creating thread %ld\n", t);
      //rc = pthread_create(&threads[t], NULL, thread_test, (void *)&barrier);
      //if (rc){
      //  printf("ERROR; return code from pthread_create() is %d\n", rc);
      //  exit(-1);
      //}

      rc = pthread_create(&threads[t], NULL, md5_file, (void *)enwik);
      if (rc){
        printf("ERROR; return code from pthread_create() is %d\n", rc);
        exit(-1);
      //}
      //sleep(1);
    }
  }
  //sleep(5);
  t = 0;
  for(t = 0; t < NUM_THREADS; t++){
    pthread_join(&threads[t], NULL);
  }
  
  printf("\nSuccess\n");
  pthread_exit(0);
  
  /* Last thing that main() should do */
  //pthread_exit(NULL);
}


