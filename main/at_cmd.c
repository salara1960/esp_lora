
#include "function.h"

#define ack_ok "OK\r\n"
#define wait_ack_def 3000


typedef struct {
    char *cmd;
    char *ack;
    TickType_t wait;
} s_at_cmd;

s_at_cmd at_cmd[] = {
    {//0
        .cmd = "AT",
        .ack = ack_ok,
        .wait = 5000,
    },
    {//1//set speed //0--1200bps 5--19200bps 1--2400bps 6--38400bps 2--4800bps 7--56000bps 3--9600bps 8--57600bps 4--14400bps 9--115200bps
        .cmd = "AT+SPR=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//2//set Serial Port Check // 0--none 1--even 2--old
        .cmd = "AT+SPC=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//3//set POWER to 20dbm //0—20dbm 5—8dbm, 1—17dbm 6—5dbm, 2—15dbm 7—2dbm, 3—10dbm
        .cmd = "AT+POWER=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//4//set Channel Select to 10 //0..F — 0..15 channel
        .cmd = "AT+CS=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//5//set // Less than 16 characters Used 0..9,A..F // for example: AT+SYNW=1234ABEF\r\n (if sync word is 0x12,0x34,0xAB,0xEF)
        .cmd = "AT+SYNW=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//6//set sync word len // 0..8
        .cmd = "AT+SYNL=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//7//In FSK mode. The node function can be set.//AT+NODE=n,m -> n: 0—disable, 1—enable; mode: 0—only match NID, 1-match NID and BID
        .cmd = "AT+NODE=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//8//In FSK mode. The node ID can be set 0..255
        .cmd = "AT+NID=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//9//In FSK mode, AT+BID can be set 0..255
        .cmd = "AT+BID=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//10//In LoRA mode,CRC Function enable or disable //0‐disabe CRC function, 1‐enable CRC function
        .cmd = "AT+LRCRC=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//11//In LoRa mode, According to the demand to set signal band width. //6—62.5KHZ, 7—125KHZ, 8—250KHZ, 9—500KHZ
        .cmd = "AT+LRSBW=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//12//In Lora mode set spreading factor //7—SF=7, A—SF=10, 8—SF=8, B—SF=11, 9—SF=9, C—SF=12
        .cmd = "AT+LRSF=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//13//In LoRa mode.Data transfer adopt Forward Error Correction Code, this command is choose it Coding Rate.//0—CR4/5, 1—CR4/6, 2—CR4/7, 3—CR4/8
        .cmd = "AT+LRCR=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//14//In LoRa mode, module has FHSS function.//0‐disable HFSS function, 1‐enable HFSS function,note: if SBW=500 and SF=7,HFSS function disable
        .cmd = "AT+LRHF=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//15//In LoRa mode.Data transmission in the form of subcontract, this command to set the length of data packet.//1..127 (>16)
        .cmd = "AT+LRPL=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//16//In LoRa mode, the hopping period value can be set. //0..255 (>5)
        .cmd = "AT+LRHPV=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//17//In Lora mode, Frequency Step Value can be set.//0..65535
        .cmd = "AT+LRFSV=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//18//Set mode. The modulation of HM-TRLR can be changed,//0-LoRa, 1-OOK, 2-FSK, 3-GFSK, In the OOK mode，baudrate no more than 9600 bps
        .cmd = "AT+MODE=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//19//Frequence band can be changed.//0--434MHZ Band, 1--470MHZ Band, 2--868MHZ Band, 3--915MHZ Band
        .cmd = "AT+BAND=",
        .ack = ack_ok,
        .wait = wait_ack_def,
    },
    {//20//save profile to dataflash
        .cmd = "AT&V",
        .ack = ack_ok,
        .wait = 5000,
    }//,
//   {//21//save profile to dataflash
//        .cmd = "AT&W",
//        .ack = ack_ok,
//        .wait = wait_ack_def,
//    }
};
#define TotalCmd  (sizeof(at_cmd) / sizeof(s_at_cmd))

/*
HM‐TRLR‐S‐868 Module default setting

Command		default		remark
AT+SPR=n 	n=3 		Baud rate 9600pbs
AT+SPC=n 	n=0 		None check
AT+POWER=n 	n=0 		Power 20dbm
AT+CS=n 	n=A 		868MHz
AT+SYNL=n 	n=6 		6 bytes
AT+NODE=n,mode 	n=0,mode=0 	Disable ID Node function
AT+LRCRC=n 	n=1 		Lora mode,CRC enable
AT+LRSBW=n 	n=7 		SBW = 125KHz
AT+LRSF=n 	n=9 		SF = 9
AT+LRCR=n 	n=0 		CodeRate=4/5
AT+LRHF=n 	n=0 		FHSS is disable
AT+LRPL=n 	n=32 		Package lenth 32bytes
AT+LRHPV=n 	n=10 		Hopping period
AT+LRFSV=n 	n=1638 		Frequence step 100KHz
AT+MODE=n 	n=0 		LoRa mode
AT+BAND=n 	n=2 		868MHz band
*/
