/*

ogg2mogg
by Michael Tolly (onyxite)
built on research by xorloser and maxton

Code is in the public domain.
https://creativecommons.org/publicdomain/zero/1.0/

*/

#include <stdio.h>
#include <stdint.h>
#include "vorbis/vorbisfile.h"

#define SEEK_INCREMENT 0x8000
#define FRAME_INCREMENT 20000

void write_uint32_LE(FILE *f, uint32_t n) {
  uint8_t bytes[4];
  bytes[0] = n;
  bytes[1] = n >> 8;
  bytes[2] = n >> 16;
  bytes[3] = n >> 24;
  fwrite(&bytes[0], 1, 4, f);
}

int main(int argc, char const *argv[])
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s in.ogg out.mogg\n", argv[0]);
    return 1;
  }

  OggVorbis_File ov;
  int err = ov_fopen(argv[1], &ov);
  if (err) {
    fprintf(stderr, "ov_fopen failed to open the OGG file (code %d).\n", err);
    return 1;
  }

  int64_t total_samples = ov_pcm_total(&ov, -1);
  int64_t total_bytes = ov_raw_total(&ov, -1);
  // ov_raw_total gives a lower number than actual file size,
  // I think because it doesn't include an extra OGG stream at the end?

  // first, start from each 0x8000-byte increment in the OGG
  // and record which sample position is the next to decode
  int seek_entries = (total_bytes + (SEEK_INCREMENT - 1)) / SEEK_INCREMENT;
  // ^ this does "total_bytes / SEEK_INCREMENT" but rounds up
  uint32_t seek_table[seek_entries];
  for (int i = 0; i < seek_entries; i++) {
    err = ov_raw_seek(&ov, i * SEEK_INCREMENT);
    if (err) {
      seek_entries = i; // cut off table early, not sure if needed
      break;
    } else {
      seek_table[i] = ov_pcm_tell(&ov);
    }
  }
  ov_clear(&ov);

  // then, for each 20000-sample up to the audio length,
  // compute the correct position from seek_table in order to
  // get that sample. this is the last entry in the table whose
  // sample position is earlier than the sample we want
  int mogg_entries = (total_samples + (FRAME_INCREMENT - 1)) / FRAME_INCREMENT;
  // ^ again, int division that rounds up
  uint32_t mogg_table_bytes[mogg_entries];
  uint32_t mogg_table_samples[mogg_entries];
  // this could be done in a single pass but whatever
  for (int i = 0; i < mogg_entries; i++) {
    uint32_t desired_position = i * FRAME_INCREMENT;
    uint32_t current_bytes = 0;
    uint32_t current_samples = 0;
    for (int j = 0; j < seek_entries; j++) {
      if (seek_table[j] <= desired_position) {
        current_bytes = j * SEEK_INCREMENT;
        current_samples = seek_table[j];
      } else {
        break;
      }
    }
    mogg_table_bytes[i] = current_bytes;
    mogg_table_samples[i] = current_samples;
  }

  FILE *mogg = fopen(argv[2], "wb");
  if (!mogg) {
    fprintf(stderr, "Couldn't open %s for writing.\n", argv[2]);
    return 1;
  }

  write_uint32_LE(mogg, 0xA); // unencrypted mogg
  write_uint32_LE(mogg, 20 + 8 * mogg_entries); // offset the ogg starts at
  write_uint32_LE(mogg, 0x10);
  write_uint32_LE(mogg, FRAME_INCREMENT);
  write_uint32_LE(mogg, mogg_entries);
  for (int i = 0; i < mogg_entries; i++) {
    write_uint32_LE(mogg, mogg_table_bytes[i]);
    write_uint32_LE(mogg, mogg_table_samples[i]);
  }

  // copy the OGG contents into the MOGG
  FILE *ogg = fopen(argv[1], "rb");
  if (!ogg) {
    fprintf(stderr, "Couldn't open %s for reading.\n", argv[1]);
    return 1;
  }
  // file copy courtesy of https://stackoverflow.com/a/5263102
  size_t n, m;
  unsigned char buff[8192];
  do {
    n = fread(buff, 1, sizeof buff, ogg);
    if (n) m = fwrite(buff, 1, n, mogg);
    else   m = 0;
  } while ((n > 0) && (n == m));
  if (m) {
    fprintf(stderr, "There was an error copying the OGG contents.\n");
    return 1;
  }
  fclose(ogg);

  fclose(mogg);

  return 0;
}
