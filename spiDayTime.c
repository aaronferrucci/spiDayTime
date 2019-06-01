#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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

int main(void)
{
  int fd;
  int spi_chan = 0;
  unsigned char daytime[3];

  wiringPiSetup();
  fd = spiSetup(spi_chan, 1);
  int fake_datetime = 0;
  while (1) {
    // MS byte first
    daytime[0] = (fake_datetime & 0xFF0000) >> 16;
    daytime[1] = (fake_datetime & 0xFF00) >> 8;
    daytime[2] = fake_datetime & 0xFF;

    // the buffer is transmitted index-0-first
    // bytes are transmitted MSB first.
    if (wiringPiSPIDataRW(spi_chan, daytime, 3) == -1) {
      printf("SPI failure: %s\n", strerror(errno));
      break;
    }

    // delay(100);
    ++fake_datetime;
  }

  close(fd);
  return 0;
}
