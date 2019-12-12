#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>		//opendir(),readdir()
#include <error.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>		//스레드



void* GPS_send(void* Para)

{
	FILE *fp2;		//파일입출력 이용을 위한 변수
	int serial_port; 	//시리얼 포트 확인을 위한 변수
	char dat,buff[100],GGA_code[3];	//GGA데이터 수신을 위한 변수
	unsigned char IsitGGAstring=0;
	unsigned char GGA_index=0;
	unsigned char is_GGA_received_completely = 0;
	float lat,log;
	int fd, i=0, j=0;	
	char data[21];		//데이터 전송용 배열
	char gps_lat[10];	//중간 저장용 배열
	char gps_log[11];	//중간 저장용 배열
	

	if ((serial_port = serialOpen ("/dev/ttyUSB0", 9600)) < 0)	// 시리얼 포트 확인
  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    return 0 ;
  }
  
  if (wiringPiSetup () == -1)	// wiringPi 초기 설정 확인 (gpio 통신을 위해)
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
    return 0 ;
  }
	
if((fd = serialOpen("/dev/ttyAMA0",115200))<0)	//데이터 전송 위한 시리얼 포트 확인
 {
   fprintf(stderr,"Unable to open serial device:%s\n",strerror(errno));
   return 0;
 }
	
  while(1){	//무한반복함
	  
		if(serialDataAvail (serial_port) )	//포트로 들어오는 데이터 유무 파악
		  { 
			dat = serialGetchar(serial_port);	// 연속적으로 데이터 받음	
			if(dat == '$'){
				IsitGGAstring = 0;
				GGA_index = 0;
			}
			else if(IsitGGAstring ==1){
				buff[GGA_index++] = dat;		//버퍼에 데이터 저장
				if(dat=='\r')				//완료시
					is_GGA_received_completely = 1;	//데이터 수신 완료 플래그
				}
			else if(GGA_code[0]=='G' && GGA_code[1]=='G' && GGA_code[2]=='A'){	//GGA 데이터를 받았을 경우
				IsitGGAstring = 1;
				GGA_code[0]= 0; 
				}
			else{
				GGA_code[0] = GGA_code[1];
				GGA_code[1] = GGA_code[2];
				GGA_code[2] = dat;
				}
		  }
		if(is_GGA_received_completely==1){	 //데이터 수신 완료시 실행
			
	//		printf("GGA: %s\n",buff);
			
		    char *sArr[10] = { NULL, };
		    char *ptr = strtok(buff, ",");
		    sArr[0] = ptr; 			// 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
		    ptr = strtok(NULL, ",");  	 	// 다음 문자열을 잘라서 포인터를 반환
		    sArr[1] = ptr;		  
		    ptr = strtok(NULL, ",");
		    sArr[2] = ptr;		  
		    ptr = strtok(NULL, ",");
		    sArr[3] = ptr;		  
		    ptr = strtok(NULL, ",");
		      
		      
		    float lat, log, a, b, mm_mmmm, mm_mmmm2, lat_v, log_v;	//gps값 변경 및 계산을 위한 변수
		    int degrees, degrees2;
		    lat = atof(sArr[1]);	//float 형식으로 변환
		    log = atof(sArr[3]);	//float 형식으로 변환
		    a = lat/100;
		    b = log/100;
		    degrees = (int)a;
		    degrees2 = (int)b;
		    mm_mmmm = (a - degrees)/0.6;
		    mm_mmmm2 = (b - degrees2)/0.6;
		    lat_v = degrees + mm_mmmm;			//위도 경도 계산
		    log_v = degrees2 + mm_mmmm2;		//위도 경도 계산
		      
		    sprintf(gps_lat, "%f", lat_v);		//문자열로 저장
		    sprintf(gps_log, "%f", log_v);		//문자열로 저장
		      
		    for (i=0; i<10;i++) {
				data[i] = gps_lat[i];		//데이터배열에 저장
		     }
		     
		     data[9] = ',';			//문자열 분리를 위한 값 추가
		     
		      for (i=10; i<21;i++) {
				data[i] = gps_log[i-10];	//데이터배열에 저장
		      }
		      
		      data[20] = ',';			//문자열 분리를 위한 값 추가
	//	      printf("밑에 전송데이터\n");
		      for(j=0; j<21; j++) {
				printf("%c", data[j]);		//전송 전 데이터 확인 
			  }
		      printf("\n");
		      i= 0;
				for (i=0; i<21; i++){		
				  serialPutchar(fd,data[i]);	//gps 변수 배열을 문자 하나씩 전송(단일 바이트)
				  fflush(stdout);		//스트림을 비운다.
				}
					
									//현재 위치를 확인하기 위한 값을 txt로 저장
			fp2 = fopen("/var/www/html/location.txt", "w");	//(w모드 -> 파일이 존재하면 내용을 삭제하고 처음부터 이어서 기록)
			fprintf(fp2, "%f\n%f\n", lat_v, log_v);		//위도, 경도 저장
			fclose(fp2); 
				
			is_GGA_received_completely = 0;			//다시 수신 플래그
		}
	}
}


int main ()
{
	float lat,log;
	int fd, i=0, j=0;
	char *sArr2[20] = { NULL, };
	char *sArr3[20] = { NULL, };
	FILE *fp;
	char aaa[20]={'.'};	//문자열 비교를 위해
	char bbb[20]={".."};	
	int k=0;
	char *ptr = NULL;
	float lat_arr[20];
	float log_arr[20];
	int count= 0;
//--------------------------------------------------------------------------
	pthread_t child_thread;
	int thread_rst;
	void* s;
	thread_rst = pthread_create(&child_thread, NULL, GPS_send, NULL);	//스레드 실행
	 
	

	while (1) {
		 
		system("sudo rm -r 211.229.241.115/");		//폴더 삭제
		system("sudo wget -r -l 0 ftp://5678:56785678@211.229.241.115/*");	//wget으로 ftp 서버의 폴더 전체 다운로드
		i=0, count = 0;
		DIR *dir;					//opendir()에서 열기한 디렉토리 정보
		struct dirent *ent;				//반환할 정보 저장
		dir = opendir ("/home/pi/211.229.241.115");	//그 안에 있는 모든 파일과 디렉토리 정보를 구한다.
	 							//폴더의 경로 변수에 저장
		
		if (dir != NULL) {		// 폴더 내 모든 파일과 폴더 목록 표현
		
		while ((ent = readdir (dir)) != NULL) {	//readdir 함수는 더 이상 읽을 디렉토리 항(파일)이 없거나 오류가 났을경우 NULL반환
			if (!strcmp(ent -> d_name, aaa)){	//검색시 . 과 ..이 존재해서 지우기 위해 추가
			 continue;				//.은 자신을 ..은 부모 디렉토리를 뜻함
			} else if (!strcmp(ent -> d_name, bbb)) {//문자열 비교를 위해 strcmp 사용
				continue;
			}
	//	  printf ("%s\n", ent->d_name);		//검색 결과 출력
		  sArr2[i] = ent->d_name;		//검색 결과 변수에 저장
		  i++;
		  count++;
		}
		closedir (dir);		

//		printf ("%d\n\n", count);	//	파일 갯수 확인
		i=0;
	//    for (i=0;i<count;i++){
	//		printf ("%s\n\n", sArr2[i]);
	//   	}
    
		for (i=0;i<count;i++){
			lat = 0;
			log= 0;
			char *ptr = strtok(sArr2[i], ",");
			sArr3[0] = ptr; 			// 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
			printf ("%s\n\n", sArr3[0]);
			ptr = strtok(NULL, ",");   		// 다음 문자열을 잘라서 포인터를 반환
			sArr3[1] = ptr;		  
			printf ("%s\n\n", sArr3[1]);
			ptr = strtok(NULL, ",");
			sArr3[2] = ptr;		  
			printf ("%s\n\n", sArr3[2]);
			lat = atof(sArr3[1]);			//문자열을 실수로 변환
			log = atof(sArr3[2]);			//문자열을 실수로 변환
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
			fp = fopen("/var/www/html/data.txt", "a");		//a 모드 -> 추가쓰기모드 -> 파일이 존재하면 뒤에부터 이어서 기록
			fprintf(fp, "%f\n%f\n", lat_arr[k], log_arr[k]);	//파일명으로부터 구한 값을 data.txt파일에 순차적으로 저장
			fclose(fp); 
		}
	  
		sleep(10);
	   	   
		system("sudo rm /var/www/html/data.txt");
	}  
	
	pthread_join(child_thread, &s);	//스레드 종료까지 대기

	return 0;
}
