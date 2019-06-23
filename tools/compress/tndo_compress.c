/*
Copyright (c) 2019 Miguel Mendez

This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.

    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

    3. This notice may not be removed or altered from any source distribution.
*/

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "zlib.h"

#include "lzh.h"
#include "lzss.h"
#include "lzw.h"

#include "tndo.h"

#define CHUNK 16384

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level) {
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];
  char tempName[255];

  /* write out TNDO header */
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_GENERIC);
  th->compression = htonl(TNDO_COMPRESSION_ZLIB);

  fseek(source, 0, SEEK_END);
  uint32_t u_size = ftell(source);
  fseek(source, 0, SEEK_SET);
  th->uncompressed_size = htonl(u_size);

  /* temp file to zlib to write to */
  /* we need this to populate the tndo */
  /* header */
  /* mktemp is horribly broken on OS X :/ */
  snprintf(tempName, 255, "/tmp/tndo%x", getpid());
  FILE *tempFile = fopen(tempName, "w+");
  if (!tempFile) {
    fprintf(stderr, "Error trying to create tmp file. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit(&strm, level);
  if (ret != Z_OK)
    return ret;

  /* compress until end of file */
  do {
    strm.avail_in = fread(in, 1, CHUNK, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;
      ret = deflate(&strm, flush);   /* no bad return value */
      assert(ret != Z_STREAM_ERROR); /* state not clobbered */
      have = CHUNK - strm.avail_out;
      if (fwrite(out, 1, have, tempFile) != have || ferror(tempFile)) {
        (void)deflateEnd(&strm);
        return Z_ERRNO;
      }
    } while (strm.avail_out == 0);
    assert(strm.avail_in == 0); /* all input will be used */

    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  assert(ret == Z_STREAM_END); /* stream will be complete */

  /* clean up and return */
  (void)deflateEnd(&strm);
  uint32_t c_size = ftell(tempFile);
  fseek(tempFile, 0, SEEK_SET);
  th->compressed_size = htonl(c_size);

  fwrite(th, sizeof(TndoHeader), 1, dest);
  free(th);

  uint8_t *c_buf = malloc(c_size);
  int read = fread(c_buf, c_size, 1, tempFile);
  if (read != 1) {
    fprintf(stderr, "Cannot read temp file. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  /* Write compressed data to TNDO file */
  fwrite(c_buf, c_size, 1, dest);
  fclose(dest);
  fclose(source);
  fclose(tempFile);
  int r_res = remove(tempName);
  if (r_res == -1) {
    fprintf(stderr, "Warning: Could not remove temp file %s.\n", tempName);
  }

  return Z_OK;
}

/* report a zlib or i/o error */
void zerr(int ret) {
  fputs("zpipe: ", stderr);
  switch (ret) {
  case Z_ERRNO:
    if (ferror(stdin))
      fputs("error reading stdin\n", stderr);
    if (ferror(stdout))
      fputs("error writing stdout\n", stderr);
    break;
  case Z_STREAM_ERROR:
    fputs("invalid compression level\n", stderr);
    break;
  case Z_DATA_ERROR:
    fputs("invalid or incomplete deflate data\n", stderr);
    break;
  case Z_MEM_ERROR:
    fputs("out of memory\n", stderr);
    break;
  case Z_VERSION_ERROR:
    fputs("zlib version mismatch!\n", stderr);
  }
}

/*
 Compress the source file using the native lzss backend.
 */
static int lzss(FILE *source, FILE *dest, uint32_t verbose, uint32_t cLevel) {

  uint32_t windowSize;

  switch (cLevel) {
  case 1:
    windowSize = LZSS_WINDOW_255;
    break;
  case 2:
    windowSize = LZSS_WINDOW_4095_4;
    break;
  case 3:
    windowSize = LZSS_WINDOW_4095_12;
    break;
  case 4:
    windowSize = LZSS_WINDOW_65535;
    break;
  default:
    fprintf(stderr, "Invalid compression level <%i>. Exiting.\n", cLevel);
    exit(EXIT_FAILURE);
  }

  /* write out TNDO header */
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_GENERIC);
  th->compression = htonl(TNDO_COMPRESSION_LZSS);

  fseek(source, 0, SEEK_END);
  uint32_t u_size = ftell(source);
  fseek(source, 0, SEEK_SET);

  void *u_data = malloc(u_size);
  size_t read = fread(u_data, u_size, 1, source);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to read file. Aborting.\n");
    abort();
  }

  compressedData *c = lzss_compress(u_data, u_size, windowSize, verbose);

  free(u_data);

  th->uncompressed_size = htonl(u_size);

  th->compressed_size = htonl(c->size);

  fwrite(th, sizeof(TndoHeader), 1, dest);
  free(th);

  /* Write compressed data to TNDO file */
  fwrite(c->data, c->size, 1, dest);
  free(c->data);
  free(c);
  fclose(dest);
  fclose(source);
  return Z_OK;
}

/*
 Compress the source file using the native lzw backend.
 */
static int lzw(FILE *source, FILE *dest, uint32_t verbose, uint32_t cLevel,
               uint32_t optimumDict) {

  uint32_t dictSize, codeLen;

  switch (cLevel) {
  case 1:
    dictSize = LZW_DICT_SIZE_SMALL;
    codeLen = LZW_CODE_LEN_SMALL;
    break;
  case 2:
    dictSize = LZW_DICT_SIZE_BIG;
    codeLen = LZW_CODE_LEN_BIG;
    break;
  default:
    fprintf(stderr, "Invalid compression level <%i>. Exiting.\n", cLevel);
    exit(EXIT_FAILURE);
  }

  /* write out TNDO header */
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_GENERIC);
  th->compression = htonl(TNDO_COMPRESSION_LZW);

  fseek(source, 0, SEEK_END);
  uint32_t u_size = ftell(source);
  fseek(source, 0, SEEK_SET);

  void *u_data = malloc(u_size);
  size_t read = fread(u_data, u_size, 1, source);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to read file. Aborting.\n");
    abort();
  }

  compressedData *c =
      lzw_compress(u_data, u_size, dictSize, codeLen, optimumDict, verbose);

  free(u_data);

  th->uncompressed_size = htonl(u_size);

  th->compressed_size = htonl(c->size);

  fwrite(th, sizeof(TndoHeader), 1, dest);
  free(th);

  /* Write compressed data to TNDO file */
  fwrite(c->data, c->size, 1, dest);
  free(c->data);
  free(c);
  fclose(dest);
  fclose(source);
  return Z_OK;
}

/*
 Compress the source file using the native lzh backend.
 */
static int lzh(FILE *source, FILE *dest, uint32_t verbose, uint32_t cLevel,
               uint32_t optimumDict) {

  uint32_t dictSize, codeLen;

  switch (cLevel) {
  case 1:
    dictSize = LZW_DICT_SIZE_SMALL;
    codeLen = LZW_CODE_LEN_SMALL;
    break;
  case 2:
    dictSize = LZW_DICT_SIZE_BIG;
    codeLen = LZW_CODE_LEN_BIG;
    break;
  default:
    fprintf(stderr, "Invalid compression level <%i>. Exiting.\n", cLevel);
    exit(EXIT_FAILURE);
  }

  /* write out TNDO header */
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_GENERIC);
  th->compression = htonl(TNDO_COMPRESSION_LZH);

  fseek(source, 0, SEEK_END);
  uint32_t u_size = ftell(source);
  fseek(source, 0, SEEK_SET);

  void *u_data = malloc(u_size);
  size_t read = fread(u_data, u_size, 1, source);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to read file. Aborting.\n");
    abort();
  }

  compressedData *c =
      lzh_compress(u_data, u_size, dictSize, codeLen, optimumDict, verbose);

  free(u_data);

  th->uncompressed_size = htonl(u_size);

  th->compressed_size = htonl(c->size);

  fwrite(th, sizeof(TndoHeader), 1, dest);
  free(th);

  /* Write compressed data to TNDO file */
  fwrite(c->data, c->size, 1, dest);
  free(c->data);
  free(c);
  fclose(dest);
  fclose(source);
  return Z_OK;
}

/*
 * Try all the native algorithms and use the smallest resulting file.
 */
static int best(FILE *source, FILE *dest, uint32_t verbose) {
  uint32_t dictSize, codeLen;
  uint32_t optimumDict = LZW_OPTIMUM_DICTIOMARY_ENABLE;
  uint32_t bestCompression;
  uint32_t windowSize;
  compressedData *c;

  fseek(source, 0, SEEK_END);
  uint32_t u_size = ftell(source);
  fseek(source, 0, SEEK_SET);

  void *u_data = malloc(u_size);
  size_t read = fread(u_data, u_size, 1, source);
  if (!read) {
    fprintf(stderr, "FATAL - Failed to read file. Aborting.\n");
    abort();
  }

  dictSize = LZW_DICT_SIZE_SMALL;
  codeLen = LZW_CODE_LEN_SMALL;
  compressedData *clzw1 =
      lzw_compress(u_data, u_size, dictSize, codeLen, optimumDict, verbose);

  dictSize = LZW_DICT_SIZE_BIG;
  codeLen = LZW_CODE_LEN_BIG;
  compressedData *clzw2 =
      lzw_compress(u_data, u_size, dictSize, codeLen, optimumDict, verbose);

  windowSize = LZSS_WINDOW_255;
  compressedData *clzss1 = lzss_compress(u_data, u_size, windowSize, verbose);

  windowSize = LZSS_WINDOW_4095_4;
  compressedData *clzss2 = lzss_compress(u_data, u_size, windowSize, verbose);

  windowSize = LZSS_WINDOW_4095_12;
  compressedData *clzss3 = lzss_compress(u_data, u_size, windowSize, verbose);

  windowSize = LZSS_WINDOW_65535;
  compressedData *clzss4 = lzss_compress(u_data, u_size, windowSize, verbose);

  if (clzw1->size < clzw2->size && clzw1->size < clzss1->size &&
      clzw1->size < clzss2->size && clzw1->size < clzss3->size &&
      clzw1->size < clzss4->size) {
    bestCompression = TNDO_COMPRESSION_LZW;
    c = clzw1;
  } else if (clzw2->size < clzw1->size && clzw2->size < clzss1->size &&
             clzw2->size < clzss2->size && clzw2->size < clzss3->size &&
             clzw2->size < clzss4->size) {
    bestCompression = TNDO_COMPRESSION_LZW;
    c = clzw2;
  } else if (clzss1->size < clzw1->size && clzss1->size < clzw2->size &&
             clzss1->size < clzss2->size && clzss1->size < clzss3->size &&
             clzss1->size < clzss4->size) {
    bestCompression = TNDO_COMPRESSION_LZSS;
    c = clzss1;
  } else if (clzss2->size < clzw1->size && clzss2->size < clzw2->size &&
             clzss2->size < clzss1->size && clzss2->size < clzss3->size &&
             clzss2->size < clzss4->size) {
    bestCompression = TNDO_COMPRESSION_LZSS;
    c = clzss2;
  } else if (clzss3->size < clzw1->size && clzss3->size < clzw2->size &&
             clzss3->size < clzss1->size && clzss3->size < clzss2->size &&
             clzss3->size < clzss4->size) {
    bestCompression = TNDO_COMPRESSION_LZSS;
    c = clzss3;
  } else {
    bestCompression = TNDO_COMPRESSION_LZSS;
    c = clzss4;
  }

  if (u_size <= c->size) {
    bestCompression = TNDO_COMPRESSION_NONE;
  }

  /* write out TNDO header */
  TndoHeader *th = malloc(sizeof(TndoHeader));
  memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
  th->type = htonl(TNDO_TYPE_GENERIC);
  th->compression = htonl(bestCompression);
  th->uncompressed_size = htonl(u_size);

  if (u_size <= c->size) {
    th->compressed_size = htonl(u_size);
  } else {
    th->compressed_size = htonl(c->size);
  }

  fwrite(th, sizeof(TndoHeader), 1, dest);
  free(th);

  /* Write compressed data to TNDO file */
  if (u_size <= c->size) {
    fwrite(u_data, u_size, 1, dest);
  } else {
    fwrite(c->data, c->size, 1, dest);
  }

  free(u_data);

  free(clzw1->data);
  free(clzw1);
  free(clzw2->data);
  free(clzw2);
  free(clzss1->data);
  free(clzss1);
  free(clzss2->data);
  free(clzss2);
  free(clzss3->data);
  free(clzss3);
  free(clzss4->data);
  free(clzss4);

  fclose(dest);
  fclose(source);
  return Z_OK;
}

static void usage(int argc, char **argv) {
  printf("Usage: %s -v -e -c <compression level> -compression_backend -i "
         "<source> -o <destination>\n",
         argv[0]);
  printf("-c : Set compression level (LZSS/LZW-only): 1-2 for LZW, 1-4 for "
         "LZSS\n");
  printf("-e : Enable optimum dictionary generation (LZW/LZH-only). Increses "
         "compression time but yields better results.\n");
  printf("-v : Verbose mode. No output otherwise.\n");
  printf("Supported compression backends are:\n");
  printf("-b : Find the best native compression setting automatically.\n");
  printf("-h : Use native hybrid LZSS/LZW encoder (fastest decompression but "
         "biggest files)\n");
  printf("-l : Use native LZW encoder (fastest decompression but biggest "
         "files)\n");
  printf("-s : Use native LZSS encoder (fastest decompression but biggest "
         "files)\n");
  printf("-z : Use zlib (default)\n");

  exit(EXIT_FAILURE);
}

/* compress input file and write the compressed data to output file */
int main(int argc, char **argv) {
  uint32_t comp_setting, ch;
  uint32_t verbose = 0;
  uint32_t ret;
  uint32_t level = Z_DEFAULT_COMPRESSION;
  uint32_t cLevel = 1;
  uint32_t optimumDict = LZW_OPTIMUM_DICTIOMARY_DISABLE;
  FILE *src, *dst;

  comp_setting = TNDO_COMPRESSION_ZLIB;

  while ((ch = getopt(argc, argv, "bhesvzli:o:c:")) != -1) {
    switch (ch) {
    case 'v':
      verbose = 1;
      break;
    case 'c':
      cLevel = atoi(optarg);
      break;
    case 'b':
      comp_setting = TNDO_COMPRESSION_BEST;
      break;
    case 'z':
      comp_setting = TNDO_COMPRESSION_ZLIB;
      break;
    case 'l':
      comp_setting = TNDO_COMPRESSION_LZW;
      break;
    case 's':
      comp_setting = TNDO_COMPRESSION_LZSS;
      break;
    case 'h':
      comp_setting = TNDO_COMPRESSION_LZH;
      break;
    case 'i':
      src = fopen(optarg, "r");
      if (!src) {
        fprintf(stderr, "FATAL - Cannot open <%s> for reading.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'o':
      dst = fopen(optarg, "w");
      if (!dst) {
        fprintf(stderr, "FATAL - Cannot open <%s> for writing.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'e':
      optimumDict = LZW_OPTIMUM_DICTIOMARY_ENABLE;
      break;
    case '?':
    default:
      usage(argc, argv);
    }
  }

  if (!src || !dst) {
    usage(argc, argv);
  }

  argc -= optind;
  argv += optind;

  switch (comp_setting) {
  case TNDO_COMPRESSION_ZLIB:
    ret = def(src, dst, level);
    break;
  case TNDO_COMPRESSION_LZW:
    ret = lzw(src, dst, verbose, cLevel, optimumDict);
    break;
  case TNDO_COMPRESSION_LZH:
    ret = lzh(src, dst, verbose, cLevel, optimumDict);
    break;
  case TNDO_COMPRESSION_LZSS:
    ret = lzss(src, dst, verbose, cLevel);
    break;
  case TNDO_COMPRESSION_BEST:
    ret = best(src, dst, verbose);
    break;
  }

  return ret;
}
