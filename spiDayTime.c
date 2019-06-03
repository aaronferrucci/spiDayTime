#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

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

unsigned int get_seconds_in_day()
{
  unsigned int secs = 0;
  time_t rawtime;
  struct tm *info;
  time(&rawtime);
  info = localtime(&rawtime);

  secs += info->tm_sec;
  secs += info->tm_min * 60;
  secs += info->tm_hour * 60 * 60;
  
  return secs;
}

void spi_encode(unsigned int seconds_in_day, unsigned char daytime[3])
{
  daytime[0] = (seconds_in_day & 0xFF0000) >> 16;
  daytime[1] = (seconds_in_day & 0xFF00) >> 8;
  daytime[2] = seconds_in_day & 0xFF;
}

int main(void)
{
  int fd;
  int spi_chan = 0;
  unsigned char daytime[3];
  // update interval, in seconds
  // one update is made per interval. Example:
  // update_interval = 3600 means one update per hour, on the hour.
  // (But upon program start, one update is done right away.)
  const unsigned int update_interval = 10 * 60;

  wiringPiSetup();
  fd = spiSetup(spi_chan, 1);
  while (1) {
    unsigned int seconds_in_day = get_seconds_in_day();
    // compute delay until the start of the next interval
    unsigned int delay_val = update_interval - (seconds_in_day % update_interval);
    // MS byte first
    spi_encode(seconds_in_day, daytime);

    // the buffer is transmitted index-0-first
    // bytes are transmitted MSB first.
    if (wiringPiSPIDataRW(spi_chan, daytime, 3) == -1) {
      printf("SPI failure: %s\n", strerror(errno));
      break;
    }
    printf("txdata: %u (delay_val: %u)\n", seconds_in_day, delay_val);

    // delay_val in seconds, delay expects milliseconds.
    delay(delay_val * 1000);
  }

  close(fd);
  return 0;
}
