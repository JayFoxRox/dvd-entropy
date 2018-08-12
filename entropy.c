/*
 * entropy.c
 *
 * Copyright 2012, Stefan Beller, stefanbeller@googlemail.com
 *
 * This file is part of the entropy utility.
 *
 * The output of different lines are piped to some special file descriptors,
 * if given (i.e. if they can be opened).
 * This piping lets you get each line into a bash variable for instance, so using
 * this program within a script might be easier.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


#define ERRORMESSAGE(output) printf("%s failed in function %s in file %s line %d with:\n%s\n", \
  output,__FUNCTION__,__FILE__,__LINE__, strerror(errno));

#define CATCHERROR(condition, output) { if (condition) { ERRORMESSAGE(output) exit(1);} }

uint64_t readByte[256];
uint64_t allReadBytes;

void reset() {
  memset(readByte, 0, sizeof(readByte));
  allReadBytes = 0;
}

void addData(const uint8_t* buffer, size_t size) {
  allReadBytes += size;
  while(size--) {
    readByte[*buffer++]++;
  }
}

double getResult() {
  uint64_t count = 0;
  double plogp = 0;
  double entropy = 0;
  double all = allReadBytes;

  unsigned int i;
  for (i = 0; i < 256; i++) {
    if (readByte[i] == 0) {
      continue;
    }
    double p = readByte[i] / all;
    plogp -= p * log(p);
    count += readByte[i];
  }
  printf("%" PRIu64 " / %" PRIu64 "\n", count, allReadBytes);
  CATCHERROR(count != allReadBytes, "internal error");

  plogp /= log(2);
  entropy = allReadBytes * plogp; // measured in bit
  return entropy;
}

void printusage(void) {
  printf("entropy <filename>\n");
  printf("<filename>\tfile to inspect\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printusage();
  }

  char* fname = argv[1];

  FILE * pFile;
  long lSize;


  pFile = fopen(fname, "rb");
  CATCHERROR( !pFile, "open");

  unsigned int sector = 0;
  while (!feof(pFile)) {
#define BUFFER_SIZE 2048
    uint8_t buffer[BUFFER_SIZE];
    size_t count = fread(buffer, 1, BUFFER_SIZE, pFile);
    CATCHERROR(count != BUFFER_SIZE, "read failed");
    CATCHERROR(ferror(pFile), "fread");

    reset();
    addData(buffer, count);
    double entropy = getResult();

    if (entropy > (0.98 * (BUFFER_SIZE * 8))) {
      printf("Looks random! ");
    }

    printf("Sector %d: entropy = %f\n", sector, entropy);

    sector += 1;
  }
  // terminate
  fclose (pFile);

  return 0;
}


