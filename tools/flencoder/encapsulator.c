/*
Copyright (c) 2019 Luis Pons

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

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <stdint.h>
#include <tndo.h>

#include "FastQuantizer.h"
#include "bstream.h"
#include "de-encapsulator.h"
#include "fixed.h"

#include "encapsulator_tab.h"

#define F_RAW (1)
#define F_WAV (2)

#define FL_FIXED (0)
#define SMP_COEFF (1)

static volatile int _nw = 0;

static inline unsigned int read1(FILE *f) {
  unsigned char v = fgetc(f);
  return v;
}

static inline unsigned int read2(FILE *f) {
  unsigned int r = 0;
  unsigned char v;
  _nw = fscanf(f, "%c", &v);
  r = v << 8;
  _nw = fscanf(f, "%c", &v);
  r = (r >> 8) | (v << 8);
  return r;
}

static inline int read2s(FILE *f) {
  int r = 0;
  unsigned char v;
  _nw = fscanf(f, "%c", &v);
  r = v << 8;
  _nw = fscanf(f, "%c", &v);
  r = (r >> 8) | (v << 8);
  return (r << 16) >> 16;
}

static inline unsigned int read4(FILE *f) {
  unsigned int r = 0;
  unsigned char v;
  _nw = fscanf(f, "%c", &v);
  r = v << 24;
  _nw = fscanf(f, "%c", &v);
  r = (r >> 8) | (v << 24);
  _nw = fscanf(f, "%c", &v);
  r = (r >> 8) | (v << 24);
  _nw = fscanf(f, "%c", &v);
  r = (r >> 8) | (v << 24);
  return r;
}

static inline void write2(unsigned int v, FILE *f) {
  fputc(v >> 8, f);
  fputc(v, f);
}

static inline void write2le(unsigned int v, FILE *f) {
  fputc(v, f);
  fputc(v >> 8, f);
}

static inline void write4(unsigned int v, FILE *f) {
  fputc(v >> 24, f);
  fputc(v >> 16, f);
  fputc(v >> 8, f);
  fputc(v, f);
}

static inline void write4le(unsigned int v, FILE *f) {
  fputc(v, f);
  fputc(v >> 8, f);
  fputc(v >> 16, f);
  fputc(v >> 24, f);
}

int get_unsigned_bitlen(unsigned int v) {
  unsigned int bits = 0;
  unsigned int one = 1;
  while ((one << bits) <= v)
    bits++;

  return bits;
}

int get_signed_bitlen(int v) {
  unsigned int r = (unsigned int)v;
  unsigned int bits = 0;
  unsigned int one = 1;
  if (v < 0) {
    r = (unsigned int)-v;
    r = r - 1;
  }

  while ((one << bits) <= r)
    bits++;

  return bits + 1;
}

static void read_text(FILE *f, char *txt) {
  int i;
  for (i = 0; i < 4; i++)
    _nw = fscanf(f, "%c", &txt[i]);
  txt[4] = 0;
  fprintf(stderr, "TEXT: '%s'\n", txt);
}

static inline int sati(int v, int min, int max) {
  if (v < min)
    v = min;
  if (v > max)
    v = max;
  return v;
}

static inline int absi(int v) {
  if (v < 0)
    v = -v;
  return v;
}

static inline int maxi(int a, int b) { return (a > b) ? a : b; }

static inline int _broundup(int v, int bits) {
  int m = (1 << bits) - 1;
  return (v + m) & ~m;
}

unsigned int codeswizz(unsigned int v) {
  int i;
  unsigned int r = 0;
  for (i = 0; i < 15; i++)
    r |= ((v >> (i * 2)) & 1) << i;
  return r;
}

typedef struct {
  signed short *res_l;
  unsigned short *res_l_encoded;
  unsigned char *res_o;  // order
  int res_l_encoded_len; // WARNING: num. of "shorts"
} enc_t;

static int _residuals[BLOCK_LEN];

static int _res_h_len = 0;
static int _max_encoded = 0;

static void enc_chann_alloc(enc_t *chann, int num_samples) {
  int num_blocks = num_samples / BLOCK_LEN;
  _res_h_len = num_samples * 3 * sizeof(signed char);
  _max_encoded = (num_blocks * 1.5 * BLOCK_LEN); // bitlen is variable

  chann->res_o = (unsigned char *)calloc(num_blocks, 1);
  chann->res_l = (signed short *)calloc(num_samples * sizeof(signed short), 1);
  chann->res_l_encoded =
      (unsigned short *)calloc(_max_encoded * sizeof(unsigned short), 1);
}

static unsigned int _subsegment_maxbits[BLOCK_LEN]; // oversized

int bit_encode_block(bstream_t *bs, signed short *src) {
  int starti = bs->idx, startb = bs->nbits; // for bit len calc
  int j, k;
  int subsegment = 8;

  int blockmax = 0, blockmin = 8;
  for (j = 0; j < BLOCK_LEN; j += subsegment) {
    // get max bit len on span
    int maxbits = 1;
    int minbits = 16;
    for (k = 0; k < subsegment; k++) {
      int v = src[j + k];
      int bits = get_signed_bitlen(v);
      if (bits > maxbits)
        maxbits = bits;
      if (bits < minbits)
        minbits = bits;
    }
    // statistics: for high compressible signal, block max and min shows close
    // ranges (3-4, 6-7, 1-4, 3-6, etc...) thre is a posibility for reduced len
    // using subsegment len 4 + 1/2 bits len codes
    assert(maxbits <= 16);
    if (maxbits > blockmax)
      blockmax = maxbits;
    if (maxbits < blockmin)
      blockmin = maxbits; // the subsegment bit min. len

    _subsegment_maxbits[j] = maxbits; // array underused, so what!? :D
  }
  int len_len = get_unsigned_bitlen(blockmax - blockmin);
  // Block header
  bstream_write(bs, len_len, 4); // bitlen of subsegment len codes
  bstream_write(bs, blockmin - 1, 4);

  for (j = 0; j < BLOCK_LEN; j += subsegment) {
    int nbits = _subsegment_maxbits[j];
    // bit len of this subsegment of residuals
    if (len_len > 0)
      bstream_write(bs, nbits - blockmin, len_len);

    for (k = 0; k < subsegment; k++) {
      bstream_write(bs, src[j + k], nbits);
    }
  }

  return ((bs->idx - starti) * 16) - startb + bs->nbits;
}

static int estimate_encoded_bitlen(int *resid) {
  int i;
  static unsigned short tmp[BLOCK_LEN];
  static signed short resl[BLOCK_LEN];
  bstream_t bs_test;
  bstream_init(&bs_test, tmp);
  for (i = 0; i < BLOCK_LEN; i++) {
    resl[i] = resid[i];
  }

  int blen = bit_encode_block(&bs_test, resl);
  return blen;
}

// Custom coefficients experiment

static inline void coeff_residuals(int *block, int c0, int c1, int c2) {
  int i;
  for (i = 0; i < BLOCK_LEN; i++) {
    int pred =
        ((block[i - 1] * c0) + (block[i - 2] * c1) + (block[i - 3] * c2)) >> 3;
    _residuals[i] = block[i] - pred;
  }
}

static inline void coeffpack_residuals(int *block, unsigned int coeff) {
  int c0, c1, c2;
  unpack_coeff(coeff, &c0, &c1, &c2);

  coeff_residuals(block, c0, c1, c2);
}

static unsigned int find_best_coeffs(int *block, float *bavg) {
  static unsigned short tmp[BLOCK_LEN * 3];
  bstream_t bs_test;
  bstream_init(&bs_test, tmp);

  // dumb space exploration
  int bestlen = 10000000;
  unsigned int coeffs = 0;
  int exp;
  int space = BC0 + BC1 + BC2;
  for (exp = 0; exp < (1 << space); exp++) {
    bstream_rewind(&bs_test);
    int br0 = exp & _dmask(BC0);
    int br1 = (exp >> BC0) & _dmask(BC1);
    int br2 = (exp >> (BC0 + BC1)) & _dmask(BC2);
    int c0 = (br0 - _dcent(BC0));
    int c1 = (br1 - _dcent(BC1));
    int c2 = (br2 - _dcent(BC2));

    coeff_residuals(block, c0, c1, c2);

    int bitlen = estimate_encoded_bitlen(_residuals);
    if (bitlen < bestlen) {
      coeffs = br0 | (br1 << 8) | (br2 << 16);
      bestlen = bitlen;
    }
  }

  *bavg = ((float)bestlen) / BLOCK_LEN;
  return coeffs;
}

#define COEFF_SAMPLES (512)

static TDataFour _fir_coeffs[NUM_COEFF_SETS];

static void coefficients_extraction(int *inl, int *inr, int num_samples) {
  static TDataFour _sample_coeffs[COEFF_SAMPLES];
  static unsigned char
      _sample_coeffs_idx[COEFF_SAMPLES]; // for quantization mapping

  int i;
  float f = 0.0f, fa = 0.0f;
  int sampling = COEFF_SAMPLES / 2;
  for (i = 0; i < num_samples; i += num_samples / sampling) {
    int n = i / (num_samples / sampling);
    _sample_coeffs[n] = find_best_coeffs(inl + i, &f);
    fprintf(stderr, "-- coeff sampling %d avg %.2f coeff 0x%x ---- \n", n, f,
            _sample_coeffs[n]);
    fa += f;
  }
  for (i = 0; i < num_samples; i += num_samples / sampling) {
    int n = i / (num_samples / sampling);
    _sample_coeffs[n + sampling] = find_best_coeffs(inr + i, &f);
    fprintf(stderr, "-- coeff sampling %d avg %.2f coeff 0x%x ---- \n", n, f,
            _sample_coeffs[n]);
    fa += f;
  }
  fprintf(stderr, "coeff est. average %.2f\n", fa / (float)COEFF_SAMPLES);

  AQuantizer *qua = Quantizer_Prepare(COEFF_SAMPLES, NUM_COEFF_SETS, 3);
  QuantizeAndIndex8(qua, _sample_coeffs, COEFF_SAMPLES, NUM_COEFF_SETS,
                    _sample_coeffs_idx, _fir_coeffs);
  fprintf(stderr, "const unsigned int _coeff_pal[%d] = \n{\n", NUM_COEFF_SETS);
  for (i = 0; i < NUM_COEFF_SETS; i++)
    fprintf(stderr, "    0x%x, //%d\n", _fir_coeffs[i], i);
  fprintf(stderr, "};\n");
  Quantizer_Free(qua);
}

// ---------------------------------------------------------------------------

static void encode_channel(enc_t *enc, int model, int *in, int num_samples,
                           unsigned int *coeffs_pal) {
  int infoO[NUM_COEFF_SETS] = {};
  int i;
  int stat_linear_ok = 0;

  bstream_t bs_low;
  bstream_init(&bs_low, enc->res_l_encoded);

  for (i = 0; i < num_samples; i += BLOCK_LEN) {
    int *block = (int *)&in[i];

    // Test simple linear predictor first
    fixed_compute_residual(block, BLOCK_LEN, 0, _residuals);
    int linlen = estimate_encoded_bitlen(_residuals);

    // Find best predictor
    int bestlen = 1000000, bestO = 0;
    int j = (model == SMP_COEFF) ? NUM_COEFF_SETS : 2;
    for (; j >= 0; j--) // Search reversed for best in len+speed
    {
      if (model == SMP_COEFF)
        coeffpack_residuals(block, coeffs_pal[j]);
      else
        fixed_compute_residual(block, BLOCK_LEN, j, _residuals);
      int blen = estimate_encoded_bitlen(_residuals);
      if (blen < bestlen)
        bestlen = blen, bestO = j;
    }
    infoO[bestO]++;
    unsigned int order = bestO;
    enc->res_o[i / BLOCK_LEN] = order; // recoder on the bit stream also

    int linear_ok =
        ((linlen * 96) / 100) <=
        bestlen; // a 4% compression is allowed for speed up on decompression

    // Residuals from best pred
    if (linear_ok) {
      stat_linear_ok++;
      fixed_compute_residual(block, BLOCK_LEN, 0, _residuals);
    } else {
      if (model == SMP_COEFF)
        coeffpack_residuals(block, coeffs_pal[order]);
      else
        fixed_compute_residual(block, BLOCK_LEN, order, _residuals);
    }

    for (j = 0; j < BLOCK_LEN; j++)
      enc->res_l[i + j] = _residuals[j];

    if (linear_ok) {
      // Mark as a linear block
      bstream_write(&bs_low, 1, 1);
    } else {
      // Mark as a fir block
      bstream_write(&bs_low, 0, 1);
      // Block head: order
      if (model == SMP_COEFF)
        bstream_write(&bs_low, order, 7);
      else
        bstream_write(&bs_low, order, 2);
    }

    // Bit-code residuals
    bit_encode_block(&bs_low, &enc->res_l[i]);

    assert((bs_low.idx < _max_encoded) && "encoding buffer overflow");
  }

  bstream_flush(&bs_low);

  enc->res_l_encoded_len = bs_low.idx;

  assert(i == num_samples);
  fprintf(stderr, "Bit encoding: average %.1f bits\n",
          ((float)enc->res_l_encoded_len * 16) / (float)num_samples);
}

static void output_channel(enc_t *enc, int num_samples, FILE *out) {
  int i;
  int ll = _broundup(enc->res_l_encoded_len,
                     1); // buffer of "shorts"; round size to 4 bytes
  write4(ll * sizeof(short), out);
  for (i = 0; i < ll; i++)
    write2(enc->res_l_encoded[i], out);
}

static unsigned int test_channel(short *test, enc_t *enc, int model, int *orig,
                                 int num_samples, int addbits,
                                 unsigned int *pal) {
  int i;
  unsigned int checksum = 0;

  bstream_t bs_low;
  bstream_init(&bs_low, enc->res_l_encoded);

  de_encapsulate_channel(test, DE_ENCAPSULE_16, enc->res_l_encoded, num_samples,
                         addbits, pal);

  for (i = 0; i < num_samples; i++) {
    checksum += test[i];
    int deco = (test[i] + 32768) >> addbits;
    assert(orig[i] == deco);
  }

  return checksum;
}

static signed short _raw_buff[1024 * 1024 * 128];

typedef struct {
  int audio_format;
  int num_channels;
  int sample_rate;
  int byte_rate;
  int block_align;
  int bits_per_sample;
} wav_info_t;

static int read_wav(FILE *in, wav_info_t *wav) {
  int i;
  int full_size = read4(in);
  fprintf(stderr, "Full data size %d\n", full_size);

  char form[5];
  read_text(in, form);

  char sub_id1[5];
  read_text(in, sub_id1);

  if (strcmp(sub_id1, "fmt ") != 0) {
    fprintf(stderr, "Only uncompressed supported\n");
    exit(1);
  }

  int sub_size1 = read4(in);
  fprintf(stderr, "%s chunk size %d\n", sub_id1, sub_size1);

  full_size -= sub_size1;

  wav->audio_format = read2(in);
  wav->num_channels = read2(in);
  wav->sample_rate = read4(in);
  wav->byte_rate = read4(in);
  wav->block_align = read2(in);
  wav->bits_per_sample = read2(in);

  fprintf(stderr, "audio_format %d\n", wav->audio_format);
  fprintf(stderr, "num_channels %d\n", wav->num_channels);
  fprintf(stderr, "sample_rate %d\n", wav->sample_rate);
  fprintf(stderr, "byte_rate %d\n", wav->byte_rate);
  fprintf(stderr, "block_align %d\n", wav->block_align);
  fprintf(stderr, "bits_per_sample %d\n", wav->bits_per_sample);

  assert((wav->bits_per_sample == 16) || (wav->bits_per_sample == 8));
  assert((wav->num_channels == 1) || (wav->num_channels == 2));

  int sub_size2 = 0;

  for (;;) {
    char sub_id2[5];
    read_text(in, sub_id2);
    sub_size2 = read4(in);
    fprintf(stderr, "%s chunk size %d\n", sub_id2, sub_size2);

    if (strcmp(sub_id2, "data") != 0) {
      fprintf(stderr, "Non-data chunk '%s'. Skipping...\n", sub_id2);
      fseek(in, sub_size2, SEEK_CUR);
    } else {
      break;
    }

    // Give up if we reached the end of file.
    if (feof(in)) {
      fprintf(stderr,
              "Fatal: End of file reached and no data chunk found. Exiting.\n");
      exit(1);
    }
  }

  // READ SAMPLES
  int num_samples = sub_size2 / (wav->num_channels * 2);
  if (wav->bits_per_sample == 16) {
    for (i = 0; i < num_samples; i++) {
      int s0 = read2s(in);
      int s1 = read2s(in);
      _raw_buff[i * 2 + 0] = s0;
      _raw_buff[i * 2 + 1] = s1;
    }
  } else {
    num_samples = sub_size2 / wav->num_channels;
    for (i = 0; i < num_samples; i++) {
      int s0 = read1(in);
      int s1 = read1(in);
      // fprintf (stderr, "%d %d\n", s0, s1);
      _raw_buff[i * 2 + 0] = (s0 - 128) << 8;
      _raw_buff[i * 2 + 1] = (s1 - 128) << 8;
    }
  }

  return num_samples;
}

static int check_param(char *str, char opt) {
  return (str[0] == '-') && (str[1] == opt);
}

int main(int argc, char **argv) {
  int rembits = 6;
  int bake_fir_coeffs = 0;
  int encode = 1;
  FILE *in = stdin;
  int i;

  if (argc > 1) {
    for (i = 1; i < argc; i++) {
      if (check_param(argv[i], 'h')) {
        fprintf(stderr,
                "Use:\n  cat song.wav | flencoder [args] > output.cps\n");
        fprintf(stderr, "Args:\n  -s -> simulation; encodes and decodes the "
                        "song, result is .wav\n");
        fprintf(
            stderr,
            "  -b -> bakes a custom set of fir coefficients for this song\n");
        fprintf(stderr, "  -rNUM -> removes bits from a 16 bit source track "
                        "(no dithering)\n");
        exit(1);
      } else if (check_param(argv[i], 's')) {
        fprintf(stderr,
                "Simulating encoding/decoding result, output is .wav\n");
        encode = 0;
      } else if (check_param(argv[i], 'b')) {
        fprintf(stderr, "Baking a custom fir coefficiente palette\n");
        bake_fir_coeffs = 1;
      } else if (check_param(argv[i], 'r')) {
        rembits = atoi(&argv[i][2]);
        fprintf(stderr, "Removing %d bits from 16 bit signal\n", rembits);
      } else {
        fprintf(stderr, "Unrecognized param\n");
      }
    }
  }

  wav_info_t wav;
  char id[5];
  read_text(in, id);

  int format = F_RAW;
  if (strcmp(id, "RIFF") == 0)
    format = F_WAV;

  int num_samples = 0;
  if (format == F_WAV)
    num_samples = read_wav(in, &wav);
  else {
    int i = 2, c0 = 0, c1 = 0;
    fprintf(stderr, "Input is not a WAV/AIFF file, RAW (16 bits stereo little "
                    "endian) assumed\n");
    _raw_buff[0] = id[0] | (id[1] << 8);
    _raw_buff[1] = id[2] | (id[3] << 8);
    do {
      c0 = fgetc(in);
      c1 = fgetc(in);
      _raw_buff[i] = c0 | (c1 << 8);
      // fprintf (stderr, "%d\n", _raw_buff [i]);
      i++;
    } while (c0 != EOF);
    num_samples = i / 2;

    wav.audio_format = 1;
    wav.num_channels = 2;
    wav.sample_rate = 32000;
    wav.byte_rate = wav.sample_rate * 4;
    wav.block_align = 4;
    wav.bits_per_sample = 16;
    fprintf(stderr,
            "Stream has %d bytes, 2 cahnnels assumed, samples assumed %d\n",
            i * 2, i / 2);
  }

  // Round size
  int new_num_samples = (num_samples + (BLOCK_LEN - 1)) & (~(BLOCK_LEN - 1));
  fprintf(stderr, "num_samples %d, rounded to %d\n", num_samples,
          new_num_samples);

  int *chann_l =
      (int *)calloc((new_num_samples + MAX_FIXED_ORDER + 1) * sizeof(int), 1);
  int *chann_r =
      (int *)calloc((new_num_samples + MAX_FIXED_ORDER + 1) * sizeof(int), 1);
  short *test_chann_l = (short *)calloc(
      (new_num_samples + MAX_FIXED_ORDER + 1) * sizeof(short), 1);
  short *test_chann_r = (short *)calloc(
      (new_num_samples + MAX_FIXED_ORDER + 1) * sizeof(short), 1);

  if ((format == F_WAV) && (wav.bits_per_sample == 8))
    rembits = 8;

  fprintf(stderr, "Compressing signal for %d bits\n", 16 - rembits);

  // Prolog 0's for predictors
  chann_l += MAX_FIXED_ORDER + 1;
  chann_r += MAX_FIXED_ORDER + 1;
  test_chann_l += MAX_FIXED_ORDER + 1;
  test_chann_r += MAX_FIXED_ORDER + 1;

  for (i = 0; i < num_samples; i++) {
    int s0 = _raw_buff[i];
    int s1 = s0;
    if (wav.num_channels == 2) {
      s0 = _raw_buff[i * 2 + 0];
      s1 = _raw_buff[i * 2 + 1];
    }

    s0 += 32768; // signed to unsigned
    s1 += 32768;
    s0 >>= rembits;
    s1 >>= rembits;

    chann_l[i] = s0;
    chann_r[i] = s1;
  }

  for (i = num_samples; i < new_num_samples; i++) {
    chann_l[i] = 0;
    chann_r[i] = 0;
  }

  enc_t out_chann_l = {};
  enc_t out_chann_r = {};

  enc_chann_alloc(&out_chann_l, new_num_samples);
  if (wav.num_channels == 2)
    enc_chann_alloc(&out_chann_r, new_num_samples);

  num_samples = new_num_samples;

  int model = SMP_COEFF;

  unsigned int *pal = (unsigned int *)_coeff_pal;
  if ((model == SMP_COEFF) && (bake_fir_coeffs)) {
    pal = _fir_coeffs;
    coefficients_extraction(chann_l, chann_r, num_samples);
  }

  encode_channel(&out_chann_l, model, chann_l, num_samples, pal);
  if (wav.num_channels == 2)
    encode_channel(&out_chann_r, model, chann_r, num_samples, pal);

  unsigned int checksum = test_channel(test_chann_l, &out_chann_l, model,
                                       chann_l, num_samples, rembits, pal);
  if (wav.num_channels == 2)
    checksum += test_channel(test_chann_r, &out_chann_r, model, chann_r,
                             num_samples, rembits, pal);
  fprintf(stderr, "\nLossless test passed, checksum 0x%x\n\n", checksum);

  // OUTPUT
  // -----------------------
  FILE *out = stdout;

  if (encode) {
    // Emit TNDO header.
    // Delta header is stored on disk in Network order.
    TndoHeader *th = malloc(sizeof(TndoHeader));
    memcpy(&th->magic, TNDO_MAGIC, sizeof(uint32_t));
    th->type = htonl(TNDO_TYPE_AUDIO);
    th->compression = htonl(TNDO_COMPRESSION_CAPS);
    th->sampleRate = htonl(wav.sample_rate);
    th->numChannels = htonl(wav.num_channels);
    th->numSamples = htonl(num_samples);
    th->bitsPerSample = htonl(wav.bits_per_sample);
    th->addBits = htonl(rembits);
    th->capsModel = model;

    fwrite(th, sizeof(TndoHeader), 1, out);

    for (i = 0; i < NUM_COEFF_SETS; i++)
      write4(pal[i], out);
    output_channel(&out_chann_l, num_samples, out);
    if (wav.num_channels == 2)
      output_channel(&out_chann_r, num_samples, out);
  } else {
    fwrite("RIFF", 1, 4, out);
    write4le(num_samples * 4 + 16, out);
    fwrite("WAVE", 1, 4, out);
    fwrite("fmt ", 1, 4, out);
    write4le(16, out);

    write2le(1, out); // audio format
    write2le(wav.num_channels, out);
    write4le(wav.sample_rate, out);
    write4le(wav.sample_rate * wav.num_channels * 2, out);
    write2le(4, out); // block align
    write2le(16, out);

    fwrite("data ", 1, 4, out);
    write4le(num_samples * 4, out);

    for (i = 0; i <= num_samples; i++) {
      write2le(test_chann_l[i], out);
      if (wav.num_channels == 2)
        write2le(test_chann_r[i], out);
    }
  }

  return 1;
}
