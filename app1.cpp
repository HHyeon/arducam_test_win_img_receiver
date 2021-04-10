#include <stdio.h>
#include <windows.h>
#include <conio.h>

#define COMPORT "COM3"
HANDLE hComm;
DWORD dwEventMask;
DWORD rlen = 0;


unsigned char buffer[512];

int main()
{
   hComm = CreateFile(COMPORT, GENERIC_READ | GENERIC_WRITE ,0,NULL, OPEN_EXISTING,0,NULL);

   if(hComm == INVALID_HANDLE_VALUE)
   {
      printf("Error open FileTarget\n");
      return 1;
   }
   DCB SerialParams = {0};
   SerialParams.DCBlength = sizeof(SerialParams);

   GetCommState(hComm, &SerialParams);

   SerialParams.BaudRate   = 921600;
   SerialParams.ByteSize   = 8;
   SerialParams.StopBits   = ONESTOPBIT;
   SerialParams.Parity     = NOPARITY;

   SetCommState(hComm, &SerialParams);

   COMMTIMEOUTS timeouts = { 0 };
   timeouts.ReadIntervalTimeout         = 500;
   timeouts.ReadTotalTimeoutConstant    = 500;
   timeouts.ReadTotalTimeoutMultiplier  = 500;
   timeouts.WriteTotalTimeoutConstant   = 50;
   timeouts.WriteTotalTimeoutMultiplier = 10;

   SetCommTimeouts(hComm, &timeouts);

   SetCommMask(hComm, EV_RXCHAR);

   EscapeCommFunction(hComm, SETDTR);
   EscapeCommFunction(hComm, CLRDTR);






   unsigned int buffered_fifosize = 0;
   unsigned int image_data_received = 0;

//   WaitCommEvent(hComm, &dwEventMask, NULL);

   FILE *file;


   while(1)
   {

      ReadFile(hComm, buffer, 16, &rlen, NULL);

      if(rlen == 0) continue;

      if(rlen == 16)
      {
         buffered_fifosize  = 0;
         buffered_fifosize |= buffer[2];
         buffered_fifosize <<= 8;
         buffered_fifosize |= buffer[1];
         buffered_fifosize <<= 8;
         buffered_fifosize |= buffer[0];

         if(buffer[15] == 0xAA)
         {
            printf("incoming : %d\n", buffered_fifosize);

            image_data_received = 0;

            if((file = fopen("mini-2mp-plus.jpg", "wb")) == NULL)
            {
               printf("fopen error\n");
               CloseHandle(hComm);
               return 1;
            }

            int flag = 0;

            const int buflen = 128;

            while(image_data_received < buffered_fifosize)
            {
               ReadFile(hComm, buffer, buflen, &rlen, NULL);

               int endcheck;
               for(endcheck=0;endcheck<buflen;endcheck+=2)
               {
                  if( buffer[endcheck+0] == 0xFF && (buffer[endcheck+1] == 0xD9))
                  {
                     flag = 1;
                     printf("0xFF%02x found\n", buffer[endcheck+1]);
                     break;
                  }
               }

               if(flag == 1)
               {
                  endcheck+=2;
                  fwrite(buffer, endcheck, 1, file);
                  fflush(file);
                  image_data_received += endcheck;
                  printf("\r%d\t\t", image_data_received);
                  break;
               }
               else
               {
                  fwrite(buffer, buflen, 1, file);
                  fflush(file);
                  image_data_received += rlen;
                  printf("\r%d\t\t", image_data_received);
               }


               if(rlen != buflen)
               {
                  printf("exit\n");
                  break;
               }
            }


            printf("\n");

            fflush(file);
            fclose(file);

            printf("capture end\n");

            Sleep(100);
         }

      }
//      if(rlen)
//      {
//         buffered_fifosize = ((buffer[2]<<16) | (buffer[1]<<8) | (buffer[0])) & 0x07fffff;
//
//         printf("%d\n", buffered_fifosize);
//      }


   }


   return 0;
}
