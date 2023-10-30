#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <unistd.h>
//#include "player.h"
#include "vs1053.h"

#include <wiringPi.h>
#include <wiringPiSPI.h>
#include "player1053.h"


#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <fcntl.h>

#define FILE_BUFFER_SIZE 512
#define SDI_MAX_TRANSFER_SIZE 32
#define SDI_END_FILL_BYTES       2050
#define LISTENPORT 12345		//port listening on at this end


socklen_t cliLen;
struct sockaddr_in caller;
int sd;
char rxNetPkt[4096];


int main_vol;


/* How many transferred bytes between collecting data.
   A value between 1-8 KiB is typically a good value.
   If REPORT_ON_SCREEN is defined, a report is given on screen each time
   data is collected. */
#define REPORT_INTERVAL 4096
#define REPORT_INTERVAL_MIDI 512
#if 1
#define REPORT_ON_SCREEN
#endif

#define min(a,b) (((a)<(b))?(a):(b))

enum AudioFormat {
  afUnknown,
  afRiff,
  afMp3,
  afMidi,
} audioFormat = afUnknown;

const char *afName[] = {
  "AAC",
  "RIFF",
  "MP3",
  "MIDI",
};

enum PlayerStates {
  psPlayback = 0,
  psUserRequestedCancel,
  psCancelSentToVS10xx,
  psStopped
} playerState;

//---

int sock_listen(int port)
{
int i, sd;

struct sockaddr_in servAddr;

sd=socket(AF_INET, SOCK_DGRAM, 0);  		//create socket to listen
if(sd < 0){ 
  return(-1);
}

else{
  fcntl(sd, F_SETFL, O_NONBLOCK);		//make socket non-blocking
}
 
servAddr.sin_family = AF_INET;
servAddr.sin_addr.s_addr = htonl(INADDR_ANY);	//use local IP address
servAddr.sin_port = htons(port);				//port to bind to (i.e. listen on)

if((i=bind(sd, (struct sockaddr *) &servAddr, sizeof(servAddr))) < 0){
  return(-1);									//failed to bind to port
}
else{
  return sd;									//all good, return socket descriptor
}
}

//---

void VS1003PlayFile(FILE *readFp) 
{
static u_int8 playBuf[FILE_BUFFER_SIZE];
u_int32 bytesInBuffer;        // How many bytes in buffer left
u_int32 pos=0;                // File position
long nextReportPos=0; // File pointer where to next collect/report
int i,n;

//int volLevel = ReadSci(SCI_VOL) & 0xFF; // Assume both channels at same level

playerState = psPlayback;             // Set state to normal playback
WriteSci(SCI_DECODE_TIME, 0);         // Reset DECODE_TIME

// Main playback loop 
while ((bytesInBuffer = fread(playBuf, 1, FILE_BUFFER_SIZE, readFp)) > 0 && playerState != psStopped)
	{
    u_int8 *bufP = playBuf;

    while (bytesInBuffer && playerState != psStopped) 
		{
   
        int t = min(SDI_MAX_TRANSFER_SIZE, bytesInBuffer);

        // send data to VS10xx.
        WriteSdi(bufP, t);

        bufP += t;
        bytesInBuffer -= t;
        pos += t;

      /* If the user has requested cancel, set VS10xx SM_OUTOFWAV bit */
      if (playerState == psUserRequestedCancel) {
        if (audioFormat == afMp3 || audioFormat == afUnknown) {
          playerState = psStopped;
        } else {
          unsigned short oldMode;
          playerState = psCancelSentToVS10xx;
          printf("\nSetting SM_OUTOFWAV at file offset %ld\n", pos);
          oldMode = ReadSci(SCI_MODE);
          WriteSci(SCI_MODE, oldMode | SM_OUTOFWAV);
        }
      }

      /* If VS10xx SM_OUTOFWAV bit has been set, see if it has gone
         through. If it is, it is time to stop playback. */
      if (playerState == psCancelSentToVS10xx) {
        unsigned short mode = ReadSci(SCI_MODE);
        if (!(mode & SM_OUTOFWAV)) {
          printf("SM_OUTOFWAV has cleared at file offset %ld\n", pos);
          playerState = psStopped;
        }
      }


	//see if we need to collect and report
	if (playerState == psPlayback && pos >= nextReportPos) 
		{

        u_int16 sampleRate;
        u_int16 h1 = ReadSci(SCI_HDAT1);
        nextReportPos += (audioFormat == afMidi || audioFormat == afUnknown) ?
          REPORT_INTERVAL_MIDI : REPORT_INTERVAL;

        if (h1 == 0x7665) {
          audioFormat = afRiff;
        } else if (h1 == 0x4d54) {
          audioFormat = afMidi;
        } else if ((h1 & 0xffe6) == 0xffe2) {
          audioFormat = afMp3;
        } else {
          audioFormat = afUnknown;
        }

        sampleRate = ReadSci(SCI_AUDATA);

//printf(" HERE?\n");
        printf("\r%ldKiB "
               "%1ds %dHz %s %s"
               " %04x   ",
               pos/1024,
               ReadSci(SCI_DECODE_TIME),
               sampleRate & 0xFFFE, (sampleRate & 1) ? "stereo" : "mono",
               afName[audioFormat], h1
               );
         
        fflush(stdout);
      }
    } /* if (playerState == psPlayback && pos >= nextReportPos) */
 

//usleep(100);
n = recvfrom(sd, &rxNetPkt, sizeof(rxNetPkt), 0, (struct sockaddr *) &caller, &cliLen);

if(n > 0){
 printf("\n print sommmat is %d  \n",n);//check it's the number of bytes expected
//ntohl(caller.sin_addr.s_addr) & ntohs(caller.sin_port)		will show who sent the packet 
//main_vol += 257;
//WriteSci(SCI_VOL, main_vol);

WriteSci(SCI_BASS,0x00fa);

  }
 
  } //playback loop /* while ((bytesInBuffer = fread(...)) > 0 && playerState != psStopped) */

  printf("\nSending %d footer %d's... ", SDI_END_FILL_BYTES, 0);
  fflush(stdout);

  /* Earlier we collected endFillByte. Now, just in case the file was
     broken, or if a cancel playback command has been given, write
     lots of endFillBytes. */
  memset(playBuf, 0, sizeof(playBuf));
  for (i=0; i<SDI_END_FILL_BYTES; i+=SDI_MAX_TRANSFER_SIZE) {
    WriteSdi(playBuf, SDI_MAX_TRANSFER_SIZE);
  }

  /* If SM_OUTOFWAV is on at this point, there is some weirdness going
     on. Reset the IC just in case. */
  if (ReadSci(SCI_MODE) & SM_OUTOFWAV) {
    VSTestInitSoftware();
  }
  printf("ok\n");
}


/* Note: code SS_VER=2 is used for both VS1002 and VS1011e */
const u_int16 chipNumber[16] = {
  1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
  0, 0, 0, 0, 0, 0, 0, 0
};

//---

int VSTestInitSoftware(void) 
{
u_int16 ssVer;
//u_int32 test;
int n;
//Start initialization with a dummy read, so VS10xx's SCI bus is in a known state.
ReadSci(SCI_MODE);

while(0)
	{
for(n=0;n<15;n++)
	{
//test = ReadSci(n);
//printf(" %d : 0x%4.4x \n",n,test);
	}
usleep(10);
}	


//<<< I don't know whether we need SM_SDISHARE, or not ??? >>>
WriteSci(SCI_MODE, SM_SDINEW|SM_SDISHARE|SM_TESTS|SM_RESET);
//WriteSci(SCI_MODE, SM_SDINEW|SM_TESTS|SM_RESET);

  /* A quick sanity check: write to two registers, then test if we
     get the same results. Note that if you use a too high SPI
     speed, the MSB is the most likely to fail when read again. */
  WriteSci(SCI_AICTRL1, 0xABAD);
  WriteSci(SCI_AICTRL2, 0x7E57);
//  if (ReadSci(SCI_AICTRL1) != 0xABAD || ReadSci(SCI_AICTRL2) != 0x7E57) {
//    printf("There is something wrong with VS10xx SCI registers\n");
//    return 1;
//  }
	printf("Passed SPI test \n");
  WriteSci(SCI_AICTRL1, 0);
  WriteSci(SCI_AICTRL2, 0);

  /* Check VS10xx type */
  ssVer = ((ReadSci(SCI_STATUS) >> 4) & 15);
printf("chip # %d \n",chipNumber[ssVer]);

  if (chipNumber[ssVer]) {
    printf("Chip is VS%d\n", chipNumber[ssVer]);
    if (chipNumber[ssVer] != 1053) {
      printf("Incorrect chip\n");
      return 1;
    }
  } else {
    printf("Unknown VS10xx SCI_MODE field SS_VER = %d\n", ssVer);
    return 1;
  }

// Set the clock. Until this point we need to run SPI slow so that
//     we do not exceed the maximum speeds
WriteSci(SCI_CLOCKF, HZ_TO_SC_FREQ(12288000) | SC_MULT_03_30X | SC_ADD_03_10X);
//Now when we have upped the VS10xx clock speed, the microcontroller
//     SPI bus can run faster. Do that before you start playing. 

// Set volume level
  WriteSci(SCI_VOL, main_vol) ; //0x0f0f);

//	WriteSci(SCI_BASS,0x00ff);

  /* We're ready to go. */
  return 0;
}

//---

