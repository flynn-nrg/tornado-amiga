/*
 * Converts a png file to chunky raw.
 * Created by Miguel Mendez 
 * This file is public domain.
 * Version 1.0 17 January 2019
 */

#include <arpa/inet.h>
#include <ctype.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <png.h>

#define PNG_COLOR_TYPE_PLTE 3
#define PNG_COLOR_TYPE_GRAYSCALE 0

#define DO_NOT_SAVE_PALETTE 1
#define PALETTE_ONLY 2

typedef struct {
  png_bytep *rows;
  unsigned char *chunky;
  uint32_t *pal;
  int w;
  int h;
  int chunky_size;
  int pal_size;
  int num_cols;
  int bpp;
} canvas_t;

static void usage(char *progname) {

  fprintf(stderr, "Usage: %s [-n] [-p] -i <input_file> -o <output_file>\n",
          progname);
  fprintf(stderr, "       -n : Do not save palette.\n");
  fprintf(stderr, "       -p : Only save the palette.\n");
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

// Convert from X bpp to 8 bpp data.
static void expand_row(unsigned char *dst, unsigned char *src, int len,
                       int bpp) {
  int factor = 8 / bpp;
  int dst_index = 0;
  for (int i = 0; i < len / factor; i++) {
    // TODO: Implement this properly to hable bbp != 4
    unsigned char nibble1 = (src[i] & 0xf0) >> 4;
    unsigned char nibble2 = (src[i] & 0xf);
    dst[dst_index++] = nibble1;
    dst[dst_index++] = nibble2;
  }
}

static canvas_t *load_png(FILE *input) {
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

  c_type = png_get_color_type(png_ptr, info_ptr);
  if (c_type != PNG_COLOR_TYPE_PLTE && c_type != PNG_COLOR_TYPE_GRAYSCALE) {
    fclose(input);
    free(canvas);
    fprintf(
        stderr,
        "Fatal: Only colour mode 3 (Indexed) and 0 (grayscale) PNG images are "
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

  canvas->chunky = calloc(canvas->w, canvas->h);

  for (int i = 0; i < canvas->h; i++) {
    if (canvas->bpp != 8) {
      expand_row(&canvas->chunky[i * canvas->w], canvas->rows[i], canvas->w,
                 canvas->bpp);
    } else {
      memcpy(&canvas->chunky[i * canvas->w], canvas->rows[i], canvas->w);
    }
  }

  canvas->chunky_size = canvas->w * canvas->h;

  if (c_type != PNG_COLOR_TYPE_GRAYSCALE) {
    png_uint_32 plt = png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
    if (plt != PNG_INFO_PLTE) {
      fclose(input);
      free(canvas->chunky);
      free(canvas);
      fprintf(stderr, "FATAL. png_get_PLTE failed. Exiting.\n");
      exit(EXIT_FAILURE);
    }

    canvas->num_cols = num_palette;
    generate_palette(canvas, palette);

  } else {
    canvas->num_cols = 0;
  }

  return canvas;
}

// palette in LoadRGB32 format
// pixel data
static void write_raw_file(canvas_t *canvas, FILE *output, uint32_t options) {
  int written;

  if (!(options & DO_NOT_SAVE_PALETTE) && (canvas->num_cols > 0)) {
    written = fwrite(canvas->pal, canvas->pal_size, 1, output);
    if (written != 1) {
      fprintf(stderr, "FATAL - Could not write palette to output file.\n");
    }
  }

  if (!(options & PALETTE_ONLY)) {
    written = fwrite(canvas->chunky, canvas->chunky_size, 1, output);
    if (written != 1) {
      fprintf(stderr,
              "FATAL - Could not write encoded frames to output file.\n");
    }
  }

  free(canvas->chunky);
  free(canvas->pal);
  free(canvas);
}

int main(int argc, char **argv) {
  int ch;
  FILE *infile, *outfile;
  uint32_t options = 0;

  if (argc < 4)
    usage(argv[0]);

  while ((ch = getopt(argc, argv, "pni:o:")) != -1) {
    switch (ch) {
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
    case 'n':
      options |= DO_NOT_SAVE_PALETTE;
      break;
    case 'p':
      options |= PALETTE_ONLY;
      break;
    case '?':
    default:
      usage(argv[0]);
    }
  }
  argc -= optind;
  argv += optind;

  canvas_t *canvas = load_png(infile);

  int total = canvas->w * canvas->h;
  printf("Image information: ");
  printf("W: %i, H: %i, Bit depth: %i, ", canvas->w, canvas->h, canvas->bpp);
  printf("Colours: %i\n", canvas->num_cols);
  if (!(options & DO_NOT_SAVE_PALETTE)) {
    printf("Palette size: %i.\n", canvas->pal_size);
  }
  write_raw_file(canvas, outfile, options);

  fclose(infile);
  fclose(outfile);

  exit(0);
}
