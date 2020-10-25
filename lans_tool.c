/*
Copyright (C) 2020 Benjamin Larsson <banan@ludd.ltu.se>

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/** CRC-16/EN-13757
 * CRC-16/EN-13757	0xD8F6	0xC2B7	0x3D65	0x0000	false	false	0xFFFF
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <getopt.h>
#include <ctype.h>

#define TTYDEV "/dev/ttyUSB0"
#define BAUDRATE B115200

#define ENC_DISABLE 1
#define ENC_ENABLE 2

void print_usage() {
    printf("lans_tool version 0.2, Copyright (c) 2020 Benjamin Larsson\n");
    printf("Usage: lans_tool [OPTION]...\n");
    printf("\t -d Set serial device\n");
    printf("\t -s Get autolock status\n");
    printf("\t -s Restart autolock status <HEX>\n");
    printf("\t -t Get transmit interval\n");
    printf("\t -T Set transmit interval <Interval>\n");
    printf("\t -i Get sensor info\n");
    printf("\t -D Disable sensor encryption\n");
    printf("\t -E Enable sensor encryption\n");
    printf("\t -m Get wmbus mode\n");
    printf("\t -M Set wmbus mode\n");
    printf("\n\n ./lans_tool -M FF0101 to set wmbus mode to S1\n");
}

static int decode_hex(char ch)
{
  if (ch <= '9') {
    return ch - '0';
  } else if (ch <= 'F') {
    return ch - 'A' + 10;
  } else {
    return ch - 'a' + 10;
  }
}


/* Convert n hex digits in string s to bin */
static int hex2bin(char* s, unsigned char* bin, int n)
{
  int i;

  /* Check that we have n hex digits, followed by \0 */
  for (i=0;i<n;i++) {
    if (!isxdigit(s[i]))
      return -1;
  }
  if (s[n] != 0)
    return -1;

  while (*s) {
    *bin++ = (decode_hex(s[0]) << 4) | decode_hex(s[1]);
    s += 2;
  }
  return 0;
}


int open_device(char* tty_device) {
    struct termios tio;
    struct termios stdio;
    struct termios old_stdio;
    int tty_fd_l;

    memset(&stdio,0,sizeof(stdio));
    stdio.c_iflag=0;
    stdio.c_oflag=0;
    stdio.c_cflag=0;
    stdio.c_lflag=0;
    stdio.c_cc[VMIN]=1;
    stdio.c_cc[VTIME]=0;

    memset(&tio,0,sizeof(tio));
    tio.c_iflag=0;
    tio.c_oflag=0;
    tio.c_cflag=CS8|CREAD|CLOCAL;           // 8n1, see termios.h for more information
    tio.c_lflag=0;
    tio.c_cc[VMIN]=1;
    tio.c_cc[VTIME]=5;
    if((tty_fd_l = open(tty_device , O_RDWR | O_NONBLOCK)) == -1){
        printf("Error while opening %s\n", tty_device); // Just if you want user interface error control
        exit(EXIT_FAILURE);
    }
    cfsetospeed(&tio,BAUDRATE);
    cfsetispeed(&tio,BAUDRATE);            // baudrate is declarated above
    tcsetattr(tty_fd_l,TCSANOW,&tio);

    return tty_fd_l;
}


int get_wmbus_mode(int tty_fd_l) {
    int cnt, i,j;
    unsigned char res_arr[100] = {0};
    unsigned char gm_cmd[] = {0x7E,0x14,0x02,0x7E};
    unsigned char answer;

    fprintf(stdout, "-> ");
    for (i=0 ; i<4 ; i++)
        fprintf(stdout, "%02X",gm_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(tty_fd_l, gm_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    fprintf(stdout, "<- ");
    for (i=0 ; i<8 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\n\n");

    fprintf(stdout, "WMBus input mode: ");
    /* Parse wmbus input mode */
    answer = res_arr[3];
    switch(answer) {
        case 0x1: fprintf(stdout, "S1\n"); break;
        case 0x2: fprintf(stdout, "T1 and C1\n"); break;
        case 0xFF: fprintf(stdout, "Not present\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }

    fprintf(stdout, "WMBus output mode: ");
    /* Parse wmbus output mode */
    answer = res_arr[4];
    switch(answer) {
        case 0x1: fprintf(stdout, "S1\n"); break;
        case 0x2: fprintf(stdout, "T1\n"); break;
        case 0x3: fprintf(stdout, "C1\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }

    fprintf(stdout, "WMBus output frame format: ");
    /* Parse wmbus output mode */
    answer = res_arr[5];
    switch(answer) {
        case 0x1: fprintf(stdout, "A\n"); break;
        case 0x2: fprintf(stdout, "B\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }
    return 0;
}

int set_wmbus_mode(int tty_fd_l, unsigned char* set_mode) {
    int cnt, i,j;
    unsigned char res_arr[100] = {0};
    unsigned char sm_cmd[] = {0x7E,0x15,0x05,0x00,0x00,0x00,0x7E};
    unsigned char answer;

    /* prepare command */
    sm_cmd[3] = set_mode[0];
    sm_cmd[4] = set_mode[1];
    sm_cmd[5] = set_mode[2];
    fprintf(stdout, "-> ");
    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",sm_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(tty_fd_l, sm_cmd, 7);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    fprintf(stdout, "<- ");
    for (i=0 ; i<9 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\n\n");
}


// Decode two bytes into three letters of five bits
static void m_bus_manuf_decode(unsigned int m_field, char* three_letter_code) {
    three_letter_code[0] = (m_field >> 10 & 0x1F) + 0x40;
    three_letter_code[1] = (m_field >> 5 & 0x1F) + 0x40;
    three_letter_code[2] = (m_field & 0x1F) + 0x40;
    three_letter_code[3] = 0;
}

int get_block1_wmbus_info(int tty_fd_l) {
    int cnt, i,j;
    unsigned char res_arr[100] = {0};
    unsigned char answer;
    unsigned char b1wi_cmd[] = {0x7E,0x40,0x02,0x7E};
    char manufacturer[4] = {0};
    unsigned int id = 0;

    memset(res_arr,0,100);
    sleep(1);
//    gak_cmd[1] = j;
    fprintf(stdout, "-> ");
    for (i=0 ; i<4 ; i++)
        fprintf(stdout, "%02X",b1wi_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(tty_fd_l, b1wi_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);
    
    fprintf(stdout, "<- ");
    for (i=0 ; i<0x0A ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\n\n");

    m_bus_manuf_decode((unsigned int)(res_arr[4] << 8 | res_arr[3]), manufacturer);
    id = res_arr[8] << 24 | res_arr[7] << 16 | res_arr[6] << 8 | res_arr[5];
    fprintf(stdout, "Manufacturer: %s\n", manufacturer);
    fprintf(stdout, "ID: %08X\n", id);
    fprintf(stdout, "Version: %02X\n", res_arr[9]);
    fprintf(stdout, "DevType: %02X\n", res_arr[10]);
    fprintf(stdout, "Label: %s.%08X.%02X.%02X\n",manufacturer, id, res_arr[10], res_arr[9]);
    return 0;
}

int set_tx_interval(int tty_fd_l, unsigned int interval) {
    int cnt, i,j;
    unsigned char res_arr[100] = {0};
    unsigned char stxi_cmd[] = {0x7E,0x46,0x04,0x00,0x00,0x7E};

    stxi_cmd[3] = (unsigned char)  interval & 0x00FF;
    stxi_cmd[4] = (unsigned char) ((interval & 0xFF00) >> 8);
    fprintf(stdout, "[%d]-> ", interval);
    for (i=0 ; i<6 ; i++)
        fprintf(stdout, "%02X",stxi_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(tty_fd_l, stxi_cmd, 6);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    fprintf(stdout, "<- ");
    for (i=0 ; i<8 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\n\n");

    fprintf(stdout, "Set TX Interval: %d seconds\n", res_arr[4] << 8 | res_arr[3]);
    return 0;
}

int get_tx_interval(int tty_fd_l) {
    int cnt, i,j;
    unsigned char res_arr[100] = {0};
    unsigned char txi_cmd[] = {0x7E,0x47,0x02,0x7E};

    fprintf(stdout, "-> ");
    for (i=0 ; i<4 ; i++)
        fprintf(stdout, "%02X",txi_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(tty_fd_l, txi_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);

    fprintf(stdout, "<- ");
    for (i=0 ; i<8 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\n\n");

    fprintf(stdout, "TX Interval: %d seconds\n", res_arr[4] << 8 | res_arr[3]);
    return 0;
}


int get_encryption(int tty_fd_l) {
    int cnt, i;
    unsigned char res_arr[100] = {0};
    unsigned char answer;
    unsigned char ge_cmd[] = {0x7E,0x24,0x02,0x7E};
    
    /* Send command to device */
    write(tty_fd_l, ge_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);
    
    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\nDevice encryption status: ");
    /* Parse answer */
    answer = res_arr[3];
    switch(answer) {
        case 0: fprintf(stdout, "Encryption Off\n"); break;
        case 1: fprintf(stdout, "Encryption On\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }
    return 0;
}

int set_encryption(int tty_fd_l, int encryption_state) {
    int cnt, i;
    unsigned char res_arr[100] = {0};
    unsigned char answer;
    unsigned char se_cmd[] = {0x7E,0x23,0x03,0x00,0x7E};
    
    se_cmd[3] = (unsigned char) encryption_state;
    
    /* Send command to device */
    write(tty_fd_l, se_cmd, 5);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);
    
    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\nDevice encryption status: ");
    /* Parse answer */
    answer = res_arr[3];
    switch(answer) {
        case 0: fprintf(stdout, "Encryption Off\n"); break;
        case 1: fprintf(stdout, "Encryption On\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }
    return 0;
}

int get_autolock_status(int tty_fd_l) {
    int cnt, i;
    unsigned char res_arr[100] = {0};
    unsigned char answer;
    unsigned char gas_cmd[] = {0x7E,0x45,0x02,0x7E};
    
    /* Send command to device */
    write(tty_fd_l, gas_cmd, 4);
    sleep(1);
    cnt = read(tty_fd_l,&res_arr,100);
    
    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\nDevice status: ");
    /* Parse answer */
    answer = res_arr[3];
    switch(answer) {
        case 0: fprintf(stdout, "Unlocked\n"); break;
        case 1: fprintf(stdout, "Locked\n"); break;
        case 2: fprintf(stdout, "Locked, wrong AES-key\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }
    return 0;
}

int restart_autolock(int fd, unsigned char* data)
{
    unsigned char ra_cmd[] = {0x7E,0x44,0x05,0x00,0x00,0x00,0x7E};
    unsigned char res_arr[100] = {0};
    unsigned char answer;
    int cnt, i;

    /* prepare command */
    ra_cmd[3] = data[0];
    ra_cmd[4] = data[1];
    ra_cmd[5] = data[2];
    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",ra_cmd[i]);
    fprintf(stdout, "\n");

    /* Send command to device */
    write(fd, ra_cmd, 7);
    sleep(1);
    cnt = read(fd,&res_arr,100);

    for (i=0 ; i<7 ; i++)
        fprintf(stdout, "%02X",res_arr[i]);
    fprintf(stdout, "\nDevice status: ");
    /* Parse answer */
    answer = res_arr[3];
    switch(answer) {
        case 0: fprintf(stdout, "Unlocked\n"); break;
        case 1: fprintf(stdout, "Locked\n"); break;
        case 2: fprintf(stdout, "Locked, wrong AES-key\n"); break;
        default: fprintf(stdout, "Unknown\n"); break;
    }
    return 0;
}


int main(int argc,char** argv)
{
    int option = 0;
    int tty_fd, flags;

    char* tty_device = TTYDEV;
    unsigned char c[16];
    int g_encryption = 0;
    int s_encryption = 0;
    int g_aes_key = 0;
    int g_autolock_status = 0;
    int g_info=0;
    int g_tx_interval = 0;
    int g_wmbus_mode = 0;
    char*  s_wmbus_mode_string = NULL;
    unsigned int s_tx_interval = 0;
    char* r_auto_lock_string = NULL;

    while ((option = getopt(argc, argv,"M:md:sr:eb:DEkitT:")) != -1) {
        switch (option) {
            case 'd' : tty_device = optarg; 
                break;
            case 'e' : g_encryption = 1;
                break;
            case 'r' : r_auto_lock_string = optarg;
                break;
            case 's' : g_autolock_status = 1;
                break;
            case 'D' : s_encryption = ENC_DISABLE;
                break;
            case 'E' : s_encryption = ENC_ENABLE;
                break;
            case 'k' : g_aes_key = 1;
                break;
            case 'i' : g_info = 1;
                break;
            case 't' : g_tx_interval = 1;
                break;
            case 'T' : s_tx_interval = atoi(optarg);
                break;
            case 'm' : g_wmbus_mode = 1;
                break;
            case 'M' : s_wmbus_mode_string = optarg;
                break;
            default: print_usage(); 
                 exit(EXIT_FAILURE);
        }
    }
    if (argc < 2) {
        print_usage();
        goto exit;
    }

    tty_fd = open_device(tty_device);

    if (g_autolock_status)
        get_autolock_status(tty_fd);

    if (r_auto_lock_string) {
        if (hex2bin(r_auto_lock_string,c,6) < 0) {
          fprintf(stderr,"ERROR parsing hex string (should be 6 hex digits)\n");
          return 1;
        }
        restart_autolock(tty_fd, c);
    }

    if (g_encryption)
        get_encryption(tty_fd);

    if (s_encryption) {
        if (s_encryption == ENC_DISABLE)
            set_encryption(tty_fd, 0x00);
        else if (s_encryption == ENC_ENABLE)
            set_encryption(tty_fd, 0x01);
    }

    if (g_info)
        get_block1_wmbus_info(tty_fd);

    if (g_tx_interval)
        get_tx_interval(tty_fd);
    
    if (s_tx_interval)
        set_tx_interval(tty_fd, s_tx_interval);

    if (g_wmbus_mode)
        get_wmbus_mode(tty_fd);

    if (s_wmbus_mode_string) {
        if (hex2bin(s_wmbus_mode_string,c,6) < 0) {
          fprintf(stderr,"ERROR parsing hex string (should be 6 hex digits)\n");
          return 1;
        }
        set_wmbus_mode(tty_fd,c);
    }
exit:
    close(tty_fd);
    return EXIT_SUCCESS;
}

