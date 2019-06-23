/*
 * Converts a png file to delta frames.
 * Created by Miguel Mendez 
 * This file is public domain.
 * Version 1.0 02 November 2018
 */

#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <png.h>

#define PNG_COLOR_TYPE_PLTE 3

typedef struct {
  png_bytep *rows;
  unsigned char *encoded;
  uint32_t *pal;
  int w;
  int h;
  int black_frame;
  int frame_h;
  int encoded_size;
  int pal_size;
  int num_cols;
  int bpp;
} canvas_t;

static void usage(char *progname) {

  fprintf(stderr, "Usage: %s -b -h height -i <input_file> -o <output_file>\n",
          progname);
  exit(EXIT_FAILURE);
}

// Generate LoadRGB32 palette from the PNG data.
static void generate_palette(canvas_t *canvas, png_colorp palette) {

  canvas->pal = calloc(sizeof(uint32_t), (canvas->num_cols * 3) + 2);
  canvas->pal[0] = htonl(canvas->num_cols << 16);
  canvas->pal_size = sizeof(uint32_t) * ((canvas->num_cols * 3) + 2);
  uint32_t *p = canvas->pal;
  p++;
  for (int i = 0; i < canvas->num_cols; i++) {
    *p++ = htonl(((uint32_t)palette[i].red << 24) | 0x00ffffff);
    *p++ = htonl(((uint32_t)palette[i].green << 24) | 0x00ffffff);
    *p++ = htonl(((uint32_t)palette[i].blue << 24) | 0x00ffffff);
  }
}

static canvas_t *load_png(FILE *input, int frame_height) {
  unsigned char header[8];
  canvas_t *canvas = calloc(sizeof(canvas_t), 1);
  png_structp png_ptr;
  png_infop info_ptr;
  png_byte c_type;
  png_colorp palette;
  int num_palette;

  fread(header, 1, 8, input);
  if (png_sig_cmp(header, 0, 8)) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Input file is not a valid PNG image. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png_ptr) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Failed to create PNG struct. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Failed to create PNG struct. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Init failed. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  png_init_io(png_ptr, input);
  png_set_sig_bytes(png_ptr, 8);
  png_read_info(png_ptr, info_ptr);

  canvas->w = png_get_image_width(png_ptr, info_ptr);
  canvas->h = png_get_image_height(png_ptr, info_ptr);
  canvas->bpp = (int)png_get_bit_depth(png_ptr, info_ptr);
  canvas->frame_h = frame_height;

  c_type = png_get_color_type(png_ptr, info_ptr);

  if (c_type != PNG_COLOR_TYPE_PLTE) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Fatal: Only colour mode 3 (Indexed) PNG images are "
                    "supported. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    fclose(input);
    free(canvas);
    fprintf(stderr, "Init failed. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  canvas->rows = (png_bytep *)calloc(canvas->h, sizeof(png_bytep));

  for (int i = 0; i < canvas->h; i++) {
    canvas->rows[i] = (png_byte *)calloc(1, canvas->w);
  }

  png_read_image(png_ptr, canvas->rows);

  canvas->encoded =
      calloc(canvas->w, canvas->h + (sizeof(uint32_t) * canvas->h));

  png_uint_32 plt = png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  if (plt != PNG_INFO_PLTE) {
    fclose(input);
    free(canvas->encoded);
    free(canvas);
    fprintf(stderr, "FATAL. png_get_PLTE failed. Exiting.\n");
    exit(EXIT_FAILURE);
  }

  canvas->num_cols = num_palette;
  generate_palette(canvas, palette);

  return canvas;
}

#define SEGMENT_LEN 10
#define BITMAP_LEN 32

// Encode delta frames.
// Format:
// Frame 0 is stored as is.
// Then, for the rest of the frames, for each row we store 1 32bit bitmap with
// the segments that are different from the previous frame, followed by the
// segments.
static void encode(canvas_t *canvas) {
  unsigned char *prev;
  unsigned char *current;
  int encoded_size = 0;
  int cur_frame;

  // Copy frame 0
  for (int i = 0; i < canvas->frame_h; i++) {
    memcpy(&canvas->encoded[i * canvas->w], canvas->rows[i], canvas->w);
  }

  encoded_size += canvas->w * canvas->frame_h;

  for (int frame = 1;
       frame < ((canvas->h / canvas->frame_h) - canvas->black_frame); frame++) {
    for (int l = 0; l < canvas->frame_h; l++) {
      prev = canvas->rows[((frame - 1) * canvas->frame_h) + l];
      current = canvas->rows[(frame * canvas->frame_h) + l];

      // Compute bitmaps.
      uint32_t bitmap = 0;
      int seg_offset = 0;
      uint32_t mask = 1 << (BITMAP_LEN - 1);
      for (int j = 0; j < BITMAP_LEN; j++) {
        for (int i = 0; i < SEGMENT_LEN; i++) {
          if (prev[seg_offset + i] != current[seg_offset + i]) {
            bitmap |= mask;
            break;
          }
        }
        mask = mask >> 1;
        seg_offset += SEGMENT_LEN;
      }

      uint32_t *p = (uint32_t *)&canvas->encoded[encoded_size];
      *p = htonl(bitmap);
      encoded_size += sizeof(uint32_t);

      // Store segments.
      mask = 1 << (BITMAP_LEN - 1);
      for (int i = 0; i < BITMAP_LEN; i++) {
        if (bitmap & mask) {
          memcpy(&canvas->encoded[encoded_size], &current[i * SEGMENT_LEN],
                 SEGMENT_LEN);
          encoded_size += SEGMENT_LEN;
        }
        mask = mask >> 1;
      }
    }
  }

  canvas->encoded_size = encoded_size;
}

// Returns 0 if verification is ok.
static int verify(canvas_t *canvas) {

  unsigned char *output_buffer, *payload;
  int frame_count = 1;
  uint32_t *bptr;
  uint32_t bitmap;
  uint32_t bitmap_store[256];

  output_buffer = calloc(canvas->frame_h * canvas->w, 1);

  // Copy frame 0
  memcpy(output_buffer, canvas->encoded, canvas->frame_h * canvas->w);

  payload = canvas->encoded;
  payload += canvas->frame_h * canvas->w;

  for (int frame = 1;
       frame < ((canvas->h / canvas->frame_h) - canvas->black_frame); frame++) {
    for (int j = 0; j < canvas->frame_h; j++) {
      bptr = (uint32_t *)payload;
      bitmap = *bptr;
      bitmap = ntohl(bitmap);
      bitmap_store[j] = bitmap;
      payload += sizeof(uint32_t);
      uint32_t mask = 1 << (BITMAP_LEN - 1);
      for (int i = 0; i < BITMAP_LEN; i++) {
        if (bitmap & mask) {
          memcpy(&output_buffer[(j * canvas->w) + (i * SEGMENT_LEN)], payload,
                 SEGMENT_LEN);
          payload += SEGMENT_LEN;
        }
        mask = mask >> 1;
      }
    }

    // Verify rows for current frame.
    for (int i = 0; i < canvas->frame_h; i++) {
      int res = memcmp(&output_buffer[i * canvas->w],
                       canvas->rows[(frame * canvas->frame_h) + i], canvas->w);
      if (res) {
        fprintf(stderr,
                "Comparison for frame %i, row %i failed = %i, bitmap = %x\n",
                frame, i, res, bitmap_store[i]);
        abort();
      }
    }
  }

  // Free resources.
  for (int i = 0; i < canvas->h; i++) {
    free(canvas->rows[i]);
  }

  free(canvas->rows);
  free(output_buffer);

  return 0;
}

static void write_anim_file(canvas_t *canvas, FILE *output) {
  int written;
  uint32_t num_frames;

  written = fwrite(canvas->pal, canvas->pal_size, 1, output);
  if (written != 1) {
    fprintf(stderr, "FATAL - Could not write palette to output file.\n");
  }

  num_frames = htonl((canvas->h / canvas->frame_h) - canvas->black_frame);
  written = fwrite(&num_frames, sizeof(uint32_t), 1, output);
  if (written != 1) {
    fprintf(stderr, "FATAL - Could not write frame marker to output file.\n");
  }

  written = fwrite(canvas->encoded, canvas->encoded_size, 1, output);
  if (written != 1) {
    fprintf(stderr, "FATAL - Could not write encoded frames to output file.\n");
  }

  free(canvas->encoded);
  free(canvas->pal);
  free(canvas);
}

int main(int argc, char **argv) {
  int ch;
  int height;
  FILE *infile, *outfile;
  int black_frame = 0;

  if (argc < 6)
    usage(argv[0]);

  while ((ch = getopt(argc, argv, "bi:o:h:")) != -1) {
    switch (ch) {
    case 'h':
      height = atoi(optarg);
      break;
    case 'i':
      infile = fopen(optarg, "r");
      if (!infile) {
        fprintf(stderr, "FATAL - Cannot open <%s> for reading.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'o':
      outfile = fopen(optarg, "w");
      if (!outfile) {
        fprintf(stderr, "FATAL - Cannot open <%s> for writing.\n", optarg);
        exit(EXIT_FAILURE);
      }
      break;
    case 'b':
      black_frame = 1;
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }
  argc -= optind;
  argv += optind;

  canvas_t *canvas = load_png(infile, height);

  canvas->black_frame = black_frame;

  encode(canvas);
  verify(canvas);

  int total = canvas->w * canvas->h;
  printf("Image information: ");
  printf("W: %i, H: %i, Bit depth: %i, ", canvas->w, canvas->h, canvas->bpp);
  printf("Colours: %i, Frames: %i\n", canvas->num_cols,
         (canvas->h / canvas->frame_h) - canvas->black_frame);
  printf("Original size: %i.\nEncoded size: %i (%f%%).\n", total,
         canvas->encoded_size,
         ((float)canvas->encoded_size / (float)total) * 100.0f);
  printf("Palette size: %i.\n", canvas->pal_size);

  write_anim_file(canvas, outfile);

  fclose(infile);
  fclose(outfile);

  exit(0);
}
