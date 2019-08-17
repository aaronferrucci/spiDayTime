#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include <wiringPi.h>
#include <wiringPiSPI.h>

int spiSetup(int chan, int speed)
{
  int fd;
  if ((fd = wiringPiSPISetup(chan, speed)) < 0) {
    fprintf(stderr, "Can't open the SPI bus: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }
  return fd;
}

// block until new second
void wait_for_time_boundary()
{
  struct timeval te;
  suseconds_t te_last = -1;
  gettimeofday(&te, NULL);
  while (1) {
    gettimeofday(&te, NULL);
    if (te.tv_usec < te_last) break;
    te_last = te.tv_usec;
  }
}

unsigned int get_seconds_in_day()
{
  unsigned int secs = 0;
  time_t rawtime;
  struct tm *info;

  wait_for_time_boundary();
  time(&rawtime);
  info = localtime(&rawtime);

  secs += info->tm_sec;
  secs += info->tm_min * 60;
  secs += info->tm_hour * 60 * 60;
  
  return secs;
}

void spi_encode(unsigned int seconds_in_day, unsigned char daytime[3])
{
  // MS byte is at index 0
  unsigned char checksum = 0;
  daytime[1] = (seconds_in_day & 0xFF0000) >> 16;
  daytime[2] = (seconds_in_day & 0xFF00) >> 8;
  daytime[3] = seconds_in_day & 0xFF;

  for (int i = 1; i <= 3; ++i) {
    checksum += daytime[i];
  }
  checksum ^= 0xFF;
  daytime[0] = checksum;
}

// convert data payload to the byte ordering seen on the other
// end of the SPI interface.
unsigned int to_int(unsigned char bytes[4])
{
  unsigned int val = 0;

  for (int i = 0; i < 4; ++i) {
    val <<= 8;
    val += bytes[i];
  }

  return val;
}

int main(void)
{
  int fd;
  int spi_chan = 0;
  unsigned char daytime[4];
  // update interval, in seconds
  // one update is made per interval. Example:
  // update_interval = 3600 means one update per hour, on the hour.
  // (But upon program start, one update is done right away.)
  const unsigned int update_interval = 10 * 60;

  // set stdout to be unbuffered, so the occasional log file messages
  // go out straightaway.
  setbuf(stdout, NULL);

  wiringPiSetup();
  fd = spiSetup(spi_chan, 1);
  while (1) {
    unsigned int seconds_in_day = get_seconds_in_day();
    // compute delay until the start of the next interval
    unsigned int delay_val = update_interval - (seconds_in_day % update_interval);
    spi_encode(seconds_in_day, daytime);
    // if (!!(daytime[3] & 1) ^ !!(daytime[3] & 2) ^ !!(daytime[3] & 4) ^ !!(daytime[3] & 8)) {
    //   daytime[0] ^= 0x80;
    //   printf("  wrecking checksum\n");
    // }

    printf("txdata: %u [0x%08X] (delay_val: %u)\n", seconds_in_day, to_int(daytime), delay_val);

    // the buffer is transmitted index-0-first
    // bytes are transmitted MSB first.
    if (wiringPiSPIDataRW(spi_chan, daytime, 4) == -1) {
      printf("SPI failure: %s\n", strerror(errno));
      break;
    }

    // delay_val in seconds, delay expects milliseconds.
    delay(delay_val * 1000);
  }

  close(fd);
  return 0;
}
