# RaspberryPi_wutchout

순서도



![순서도](
https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FbjFjm9%2FbtqAlQXpkaI%2FbmhfXHOC9Ukv19SISQSQT0%2Fimg.png
)


GPS 모듈 사용하기

GPIO 포트를 사용하는 방법과 USB포트를 사용하는 방법중 USB포트를 사용하는 방법으로 진행하였습니다. 


1. 라즈베리파이 업데이트 및 재부팅

```
    sudo apt-get update
    sudo apt-get upgrade
    sudo reboot
```
2. GPS 사용을 위한 설정
```
    sudo raspi-config
```


Interfacing options -> Serial -> No -> Yes 이후 재부팅

3. GPS 데이터 표시 툴(gpsd) 설치
```
sudo apt-get install gpsd-clients gpsd -y
```

4. 초기 설정 변경
```
sudo nano /etc/default/gpsd
```
Look for

DEVICES=""

and change it to

DEVICES="/dev/ttyUSB0" 

이후 직렬 인터페이스 설정을 수정합니다.
```
sudo nano /boot/cmdline.txt
```
아래 라인이 있을경우 삭제하고 저장합니다. 없을경우 그대로 진행 합니다

console=ttyAMA0,115200 kgdboc=ttyAMA0,115200

재부팅합니다.
이제 모든 준비가 완료되었습니다.
모듈의 전송속도는 9600으로 설정합니다.

```
stty -F /dev/ttyUSB0 9600
```
GPS 데이터 보기 
```
gpsmon 또는 cgps -s
```


![gpsmon](https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FMtE0Q%2FbtqAnZk0gGG%2FKQeJEV4GKdG3DK1cNuggs1%2Fimg.png)

gpsmon

![cgps](
https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FbYUfjy%2FbtqAkTUhj6m%2Fz1z94kPw5AwAV70PZ9yo61%2Fimg.png)

cgps


유의 사항 : 좌표를 찾기까지 10분내외의 시간이 걸릴 수 있습니다.

5. 기타 기능

정지하기
```
sudo killall gpsd
```
부팅시 자동 실행을 하고싶지 않을때

```
sudo systemctl stop gpsd.socket
sudo systemctl disable gpsd.socket
```
부팅시 자동실행 하고싶을때 
```
sudo systemctl enable gpsd.socket
sudo systemctl start gpsd.socket
```
부팅시 gpsd가 자동으로 시작되지 않도록 하거나 정지한 후에는 gpsmon 또는 cpgs를 실행하기 전에 밑의 명령을 실행해야 합니다.
```
sudo gpsd /dev/ttyUSB0 -F /var/run/gpsd.sock
```
---
gps값 파싱

$GPGGA( Global Positioning System Fix Data) -> 시간, 위도,경도, 고도 등의 데이터가 들어온다.
```
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <wiringPi.h>
#include <wiringSerial.h>

int main ()
{
  int serial_port; 
  char dat,buff[100],GGA_code[3];
  unsigned char IsitGGAstring=0;
  unsigned char GGA_index=0;
  unsigned char is_GGA_received_completely = 0;
  
  if ((serial_port = serialOpen ("/dev/ttyUSB0", 9600)) < 0)		//사용할 시리얼 포트 지정
  {
    fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
    return 1 ;
  }

  if (wiringPiSetup () == -1)							/* initializes wiringPi setup */
  {
    fprintf (stdout, "Unable to start wiringPi: %s\n", strerror (errno)) ;
    return 1 ;
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
					is_GGA_received_completely = 1; //데이터 수신 완료
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
		if(is_GGA_received_completely==1){      //데이터 수신 완료시 출력
			printf("GGA: %s",buff);
			is_GGA_received_completely = 0;     //다시 수신 플래그
		}
	}
	return 0;
}
```
NMEA(National Marine Electronics Association) 프로토콜 분석
GPS 모듈을 연결하면 문자열로 된 GPS 데이터가 넘어오는데 이를 NMEA 데이터라 하고,
우리는 이 포맷을 파싱하여 필요한 데이터만 가져다 사용하면 된다.
```
Ex) $GPGGA,103022.132,3735.0079,N,12701.6446,E,1,04,5.4,50.1,M,20.6,M,0.0,0000*48
       - 103022.132: 시간
       - 3735.0079 : 위도
       - N은 북위
       - 12701.6446 : 경도
       - E : 동경
       - 1 : Fix의 종류 [0 : 위성이 안 잡혀 Invalid, 1 : GPS에서 제공하는 기본 위성을 가지고만 계산할 경우, 2 : DGPS를 이용하여 보정하여 계산할 경우)
       - '04':  계산에 사용한 위성을 개수
       - '5.4': horizontal dilution of Precision
       - '50.1M' : 해수면 기준 고도
       - '20.6M' : WGS-84에서 정해놓은 타원체로서 모델링한 지구와 구체로서 모델링된 지구의 고도차이
       - '0.0'과 '0000' : DGPS 사용시 마지막으로 update한 시간과 DGPS 기지국의 ID
       - '48': Check Sum

```
![gga](
https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FbLmyKm%2FbtqAnwJ7xgO%2FjHvU7Iqy7ha4Z5kWH2bWC0%2Fimg.png)

---
문자열 자르기
```
#define _CRT_SECURE_NO_WARNINGS    // strtok 보안 경고로 인한 컴파일 에러 방지
#include <stdio.h>
#include <string.h>    // strtok 함수가 선언된 헤더 파일

int main()
{
    char s1[30] = "SMU Embedded System";    // 크기가 30인 char형 배열을 선언하고 문자열 할당
    char *sArr[10] = { NULL, };    // 크기가 10인 문자열 포인터 배열을 선언하고 NULL로 초기화
    int i = 0;                     // 문자열 포인터 배열의 인덱스로 사용할 변수

    char *ptr = strtok(s1, " ");   // 공백 문자열을 기준으로 문자열을 자름

    while (ptr != NULL)            // 자른 문자열이 나오지 않을 때까지 반복
    {
        sArr[i] = ptr;             // 문자열을 자른 뒤 메모리 주소를 문자열 포인터 배열에 저장
        i++;                       // 인덱스 증가

        ptr = strtok(NULL, " ");   // 다음 문자열을 잘라서 포인터를 반환
    }

    for (int i = 0; i < 10; i++)
    {
        if (sArr[i] != NULL)           // 문자열 포인터 배열의 요소가 NULL이 아닐 때만
            printf("%s\n", sArr[i]);   // 문자열 포인터 배열에 인덱스로 접근하여 각 문자열 출력
    }

    return 0;
}
```
---
실행결과 
```
SMU
Embedded
System
```

먼저 char *sArr[10] = { NULL, };와 같이 자른 문자열을 보관할 문자열 포인터 배열을 선언하고 NULL로 초기화했습니다. 여기서 NULL 뒤에 , (콤마)를 붙여주면 배열의 모든 요소가 NULL로 초기화됩니다.

while 반복문 안에서는 sArr[i] = ptr;과 같이 자른 문자열의 메모리 주소를 배열에 저장하고, 배열의 인덱스를 증가시킵니다.

포인터 ptr은 반복문을 반복하면서 문자열을 자를 때마다 안에 저장된 메모리 주소가 계속 바뀌므로 나중에 다시 사용할 수 없습니다. 하지만 예제처럼 ptr에 저장된 메모리 주소가 바뀌기 전에 다른 곳에 보관해두면 자른 문자열을 나중에도 계속 사용할 수 있습니다.

---
데이터 전송 

전처리과정을 통해 구분을 위한 ','첨가 및 계산, 문자열과 float 변환을 통해 나온 결과를 시리얼 통신을 통해 전송한다.

```
if((fd = serialOpen("/dev/ttyAMA0",115200))<0)  //데이터 전송에 필요한 포트 사용 가능한지 확인
 {
   fprintf(stderr,"Unable to open serial device:%s\n",strerror(errno));
   return 0;
 }

	for (i=0; i<21; i++){		//gps 변수 배열을 한 바이트씩 전송
				  serialPutchar(fd,data[i]);    //
				  fflush(stdout);
				}

```

---
파일 저장

파이의 웹 서버에서 사용 할 위도 경도를 추가하기 위한 데이터파일의 저장에 이용된다.
```
#include <stdio.h>    

int main()
{

    FILE *fp = fopen("hello.txt", "w");    // hello.txt 파일을 쓰기 모드(w)로 열기.
                                           // 파일 포인터를 반환
    fprintf(fp, "Hello");    // 파일에 문자열 저장
    fclose(fp);    // 파일 포인터 닫기

    return 0;
}
```
hello.txt 출력 결과
```
Hello
```

파일 처리 모드의 종류
```
r = 읽기 모드 / 파일이 없을 경우 에러 발생 
w = 쓰기 모드 / 파일이 없을 경우 새로 만들고, 파일이 존재하면 내용을 삭제하고 처음부터 기록 
a = 추가 쓰기 모드 / 파일이 없을 경우 새로 만들고, 파일이 존재하면 뒤에부터 이어서 기록
```

---
c언어 pthread 

동시에 2가지의 작업을 처리하기 위해 스레드를 사용한다.
```
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

void* My(void* Para)    //스레드로 사용할 내용 지정

{
	int i;
	for(i=0;i<10;i++)
	{
		printf("I'm in thread \n");
		sleep(2);
	}
}



int main()

{
	pthread_t child_thread;
	int thread_rst;
	void* s;
	thread_rst = pthread_create(&child_thread, NULL, My, NULL); //스레드 시작

	while(1)
	{
		printf("I'm in main\n");
		sleep(1);
	}

	pthread_join(child_thread, &s); //스레드 종료를 기다린다.
	printf("%s\n",s);
	return 0;
}
```
main thread는 1초마다 "I'm in main"을 찍고, child thread 는 1개가 생성이 되어 2초마다 "I'm in child" 를 찍는다.


출력 결과

![gga](
https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FbS7NRU%2FbtqAlZfs0as%2FZbTtGdhU9PTmq7kBdB8Q2K%2Fimg.png)

GCC컴파일 -> 주의사항 맨뒤 옵션에 -lpthread 를 넣어준다.

---


파일 목록 가져오기
```
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <error.h>
  
int main()
{
    DIR *dir;
    struct dirent *ent;
    dir = opendir ("./");
    if (dir != NULL) {
  
    /* print all the files and directories within directory */
    while ((ent = readdir (dir)) != NULL) {
        printf ("%s\n", ent->d_name);
    }
    closedir (dir);
    } else {
         /* could not open directory */
         perror ("");
        return EXIT_FAILURE;
    }
}
```

---
System 명령어 사용

다른 프로그램을 실행하고 종료할 때까지 기다립니다.

헤더 : stdlib.h

형태 : int system (const char * string);

인수 : char string	실행할 프로그램 파일 명

<h4>wget 및 폴더, 파일 제어에 사용된다.</h4>

```
#include <stdio.h>
#include <stdlib.h> //system()

int main()
{
   system( "ls -al");
   printf( "system() 시행 후에, 이 문자열이 출력됩니다.n");
}
```
출력 결과 

![gga](
https://img1.daumcdn.net/thumb/R1280x0/?scode=mtistory2&fname=https%3A%2F%2Fk.kakaocdn.net%2Fdn%2FbjOWBv%2FbtqAlPxjU7x%2FoF22PusKPBMLZVGH4yXkS0%2Fimg.png)


---
ftp 서버 파일 다운로드

wget 사용
```
sudo wget -r -l 0 ftp://5678:56785678@211.229.241.115/*
```

