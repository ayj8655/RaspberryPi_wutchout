#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <error.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>



void* GPS_send(void* Para)

{
	FILE *fp2;
	int serial_port; 
	char dat,buff[100],GGA_code[3];
	unsigned char IsitGGAstring=0;
	unsigned char GGA_index=0;
	unsigned char is_GGA_received_completely = 0;
	float lat,log;
	int fd, i=0, j=0;
	char data[21];
	char gps_lat[10];
	char gps_log[11];
	

	if ((serial_port = serialOpen ("/dev/ttyUSB0", 9600)) < 0)		/* 시리얼 포트 확인 */
  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    return 0 ;
  }
  
  if (wiringPiSetup () == -1)							/* wiringPi 초기 설정 확인*/
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
    return 0 ;
  }
	
  
if((fd = serialOpen("/dev/ttyAMA0",115200))<0)
 {
   fprintf(stderr,"Unable to open serial device:%s\n",strerror(errno));
   return 0;
 }
	
  while(1){
	  
		if(serialDataAvail (serial_port) )		//포트로 들어오는 데이터 유무 파악
		  { 
			dat = serialGetchar(serial_port);		// 연속적으로 데이터 받음	
			if(dat == '$'){
				IsitGGAstring = 0;
				GGA_index = 0;
			}
			else if(IsitGGAstring ==1){
				buff[GGA_index++] = dat;
				if(dat=='\r')
					is_GGA_received_completely = 1;	//데이터 수신 완료
				}
			else if(GGA_code[0]=='G' && GGA_code[1]=='G' && GGA_code[2]=='A'){
				IsitGGAstring = 1;
				GGA_code[0]= 0; 
				GGA_code[0]= 0;
				GGA_code[0]= 0;		
				}
			else{
				GGA_code[0] = GGA_code[1];
				GGA_code[1] = GGA_code[2];
				GGA_code[2] = dat;
				}
		  }
		if(is_GGA_received_completely==1){		 //데이터 수신 완료시 실행
			
	//		printf("GGA: %s\n",buff);
			
			char *sArr[10] = { NULL, };
		    char *ptr = strtok(buff, ",");
		    sArr[0] = ptr; 					// 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
		    ptr = strtok(NULL, ",");  		 // 다음 문자열을 잘라서 포인터를 반환
		    sArr[1] = ptr;		  
		    ptr = strtok(NULL, ",");
		    sArr[2] = ptr;		  
		    ptr = strtok(NULL, ",");
		    sArr[3] = ptr;		  
		    ptr = strtok(NULL, ",");
		      
		      
		    float lat, log, a, b, mm_mmmm, mm_mmmm2, lat_v, log_v;	//gps값 변경 및 계산을 위한 변수
		    
		    int degrees, degrees2;
		    lat = atof(sArr[1]);
		    log = atof(sArr[3]);
		    a = lat/100;
		    b = log/100;
		    degrees = (int)a;
		    degrees2 = (int)b;
		    mm_mmmm = (a - degrees)/0.6;
		    mm_mmmm2 = (b -degrees2)/0.6;
		    lat_v = degrees + mm_mmmm;
		    log_v = degrees2 + mm_mmmm2;		//위도 경도 계산
		      
		    sprintf(gps_lat, "%f", lat_v);
		    sprintf(gps_log, "%f", log_v);
		      
		    for (i=0; i<10;i++) {
				data[i] = gps_lat[i];
		     }
		     
		     data[9] = ',';						//문자열 분리를 위한 값 추가
		     
		      for (i=10; i<21;i++) {
				data[i] = gps_log[i-10];
		      }
		      
		      data[20] = ',';					//문자열 분리를 위한 값 추가
	//	      printf("밑에꺼가 전송데이터임\n", data[j]);
		      for(j=0; j<21; j++) {
			  printf("%c", data[j]);		//전송 전 데이터 확인 
			  }
			  
			  printf("\n");
	
		      i= 0;
				for (i=0; i<21; i++){		//gps 변수 배열을 한 바이트씩 전송
				  serialPutchar(fd,data[i]);
				  fflush(stdout);
				}
				
				
			fp2 = fopen("/var/www/html/location.txt", "w");	//현재 위치를 확인하기 위한 값을 txt로 저장
			fprintf(fp2, "%f\n%f\n", lat_v, log_v);
			fclose(fp2); 
				
			is_GGA_received_completely = 0;				//다시 수신 플래그
		}
	}
}


int main ()
{
	float lat,log;
	int fd, i=0, j=0;
	char data[21];
	 char *sArr2[20] = { NULL, };
	 char *sArr3[20] = { NULL, };
	 char *sArr4[20] = { NULL, };
	 char *sArr5[20] = { NULL, };
	  FILE *fp;
	 char aaa[20]={'.'};
	 char bbb[20]={".."};
	 int k=0;
	 char *ptr = NULL;
	 float lat_arr[20];
	 float log_arr[20];
	 int count= 0;
 
	pthread_t child_thread;
	int thread_rst;
	void* s;
	thread_rst = pthread_create(&child_thread, NULL, GPS_send, NULL);
	 
	

	 while (1) {
		 
	 system("sudo rm -r 211.229.241.115/");	
	 
		system("sudo wget -r -l 0 ftp://5678:56785678@211.229.241.115/*");		//wget으로 ftp 서버의 폴더 전체 다운로드
		
		i=0, count = 0;
		DIR *dir;
		struct dirent *ent;
		dir = opendir ("/home/pi/211.229.241.115");								//경로 변수에 저장
	 	

	 if (dir != NULL) {
		
		// print all the files and directories within directory 
		
	while ((ent = readdir (dir)) != NULL) {
	   if (!strcmp(ent -> d_name, aaa)){										//검색시 . 과 ..이 존재해서 지우기 위해 추가
		 continue;
	   } else if (!strcmp(ent -> d_name, bbb)) {
		  continue;
	  }
	  
//	  printf ("%s\n", ent->d_name);												//검색 결과 출력
      sArr2[i] = ent->d_name;													//검색 결과 변수에 저장
	  i++;
	  count++;
    }
    
    closedir (dir);
    printf ("%d\n\n", count);
    
    i=0;
    
//    for (i=0;i<count;i++){
//		printf ("%s\n\n", sArr2[i]);
//   	}
    
    for (i=0;i<count;i++){
      
	lat = 0;
	log= 0;
	
	
	char *ptr = strtok(sArr2[i], ",");
	sArr3[0] = ptr; 														// 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
	printf ("%s\n\n", sArr3[0]);
	ptr = strtok(NULL, ",");   												// 다음 문자열을 잘라서 포인터를 반환
	sArr3[1] = ptr;		  
	printf ("%s\n\n", sArr3[1]);
	ptr = strtok(NULL, ",");
	sArr3[2] = ptr;		  
	printf ("%s\n\n", sArr3[2]);
    
    
    lat = atof(sArr3[1]);													//문자열을 실수로 변환
    log = atof(sArr3[2]);

    lat_arr[i]=lat;															
    log_arr[i]=log;

   
  //  printf ("%f %f\n", lat,lat_arr[i]);
  //  printf ("%f %f\n", log,log_arr[i]);
   
	}
    
    } else {
         //could not open directory 
         perror ("폴더없음");
        return EXIT_FAILURE;
    }
    
	for (k=0;k<count;k++) {													
	    fp = fopen("/var/www/html/data.txt", "a");
	    fprintf(fp, "%f\n%f\n", lat_arr[k], log_arr[k]);					//파일명으로부터 구한 값을 data.txt파일에 순차적으로 저장
	    fclose(fp); 
	  }
	  
	 sleep(10);
	   
	 system("sudo rm -r 211.229.241.115/");									//업데이트를 위해 폴더 삭제
	 system("sudo mkdir 211.229.241.115");	   
	 system("sudo chmod 777 211.229.241.115");	   
	 system("sudo rm /var/www/html/data.txt");
}  
	pthread_join(child_thread, &s);
	
	return 0;
}
