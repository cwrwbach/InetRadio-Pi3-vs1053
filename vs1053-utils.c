#include <stdint.h>
#include <wiringPi.h>
#include <wiringPiSPI.h>
#include <unistd.h>
#include <stdio.h>
#include "vs1053.h"
#include "player1053.h"

char unsigned cntl_buf[32];


char data_buf[256];



void gpio_set_output(char pin)
{
pinMode (pin, OUTPUT) ;
usleep(1000);
}


void gpio_set_input(char pin)
{
pinMode (pin, INPUT);
usleep(1000);
}

void pin_set_high(int pin)
{
//usleep(4);
digitalWrite(pin,HIGH);
//usleep(4);
}

void pin_set_low(int pin)
{
//usleep(4);
digitalWrite(pin,LOW);
//usleep(4);
}

char xxxpin_get_val(int pin)
{
int ret;
ret = digitalRead(pin);
return ret;
}


void vs1053_configure()
{
//int result;

wiringPiSetup () ;
wiringPiSPISetup(0,500000);

usleep(100000);

gpio_set_output(XRST);
gpio_set_output(XCS);
gpio_set_output(XDCS);
gpio_set_input(DREQ);
gpio_set_output(STROBE);

pin_set_high(XRST);
usleep(10000);
pin_set_low(XRST);
usleep(10000);
pin_set_high(XRST);
usleep(100000);
//result = wiringPiSPISetup (0,500000) ; //chan,speed
}






int WriteSdi(const uint8_t *data, uint8_t bytes)
{
//int i;
//int ret;
//int busy;

digitalWrite(STROBE,HIGH);
while( digitalRead(DREQ) != 1) usleep(10000);
	{
	digitalWrite(XCS,HIGH);
	digitalWrite(XDCS,LOW);
	wiringPiSPIDataRW (0,data,bytes) ;
	digitalWrite(XDCS,HIGH);
	}
digitalWrite(STROBE,LOW);	
return 0;
}



uint16_t ReadSci(uint8_t addr)
{
uint16_t ret=0;
addr &=0x0f;
cntl_buf[1] = addr;
cntl_buf[0] = READ_CMD;

pin_set_high(XDCS);
pin_set_low(XCS);	
usleep(1000);
wiringPiSPIDataRW (0,cntl_buf,4) ;
usleep(1000);
pin_set_high(XCS);
ret = cntl_buf[2] << 8 | cntl_buf[3];
return ret;
}

void WriteSci(uint8_t addr,uint16_t data)
{
addr &=0x0f;
cntl_buf[1] = addr;
cntl_buf[0] = WRITE_CMD;
cntl_buf[2] = data >>8;
cntl_buf[3] = data & 0x00ff;

pin_set_high(XDCS);
pin_set_low(XCS);	
usleep(10);
wiringPiSPIDataRW (0,cntl_buf,4) ;
usleep(10);
pin_set_high(XCS);
}

/*
void SaveUIState(void)
{
}
void RestoreUIState(void)
{
}
*/

int GetUICommand(void)
{
return -1;
}

#if(0)
#ifdef PLAYER_USER_INTERFACE
    /* GetUICommand should return -1 for no command and -2 for CTRL-C */
    c = GetUICommand();

   switch (c) {

      /* Volume adjustment */
    case '-':
      if (volLevel < 255) {
        volLevel++;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;
    case '+':
      if (volLevel) {
        volLevel--;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;

      /* Show some interesting registers */
    case 42:
      printf("\nvol %1.1fdB, MODE %04x, ST %04x, "
             "HDAT1 %04x HDAT0 %04x\n",
             -0.5*volLevel,
             ReadSci(SCI_MODE),
             ReadSci(SCI_STATUS),
             ReadSci(SCI_HDAT1),
             ReadSci(SCI_HDAT0));
      break;

      /* Ask player nicely to stop playing the song. */
    case 'q':
      if (playerState == psPlayback)
        playerState = psUserRequestedCancel;
      break;

      /* Forceful and ugly exit. For debug uses only. */
    case 'Q':
    //  RestoreUIState();
      printf("\n");
      exit(EXIT_SUCCESS);
      break;

      /* Toggle differential mode */
    case 'd':
      {
        u_int16 t = ReadSci(SCI_MODE) ^ SM_DIFF;
        printf("\nDifferential mode %s\n", (t & SM_DIFF) ? "on" : "off");
        WriteSci(SCI_MODE, t);
      }
      break;

      /* Show help */
    case '?':
      printf("\nInteractive VS1003 file player keys:\n"
             "- +\tVolume down / up\n"
             "_\tShow current settings\n"
             "q Q\tQuit current song / program\n"
             "d\tToggle Differential\n"
             );
      break;

      /* Unknown commands or no command at all */
    default:
      if (c < -1) {
        printf("Ctrl-C, aborting\n");
        fflush(stdout);
    //    RestoreUIState();
        exit(EXIT_FAILURE);
      }
      if (c >= 0) {
        printf("\nUnkkknown char '%c' (%d)\n", isprint(c) ? c : '.', c);
      }
      break;
    } /* switch (c) */
#endif /* PLAYER_USER_INTERFACE */
#endif

/*sine_test(){

ccc = 0x4820; //test command
WriteSci(0,ccc);
usleep(100000);
trial[0] = 0x53; 
trial[1] = 0xEF;
trial[2] = 0x6E;
trial[3] = 0x01; 
trial[4] = 0x00;
trial[5] = 0x00;
trial[6] = 0x00;
trial[7] = 0x00;
WriteSdi(trial,8);
* }
*/
