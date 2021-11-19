#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#define BUFSIZE 8096

static void sigchld_handler() {
  pid_t PID;
  int status;

  while (PID = waitpid(-1, &status, WNOHANG) > 0)
  {
    printf("子程序 %d 結束.\n", PID);
  }
  /* Re-install handler */
  signal(SIGCHLD, sigchld_handler);
}

struct file_info ///檔案資訊
{
    int file_length; 
    char file_name[1000];
};

struct file_info file_in;

struct {
    char *ext;
    char *filetype;
} extensions [] = {
    {"gif", "image/gif" },
    {"jpg", "image/jpeg"},
    {"jpeg","image/jpeg"},
    {"png", "image/png" },
    {"zip", "image/zip" },
    {"gz",  "image/gz"  },
    {"tar", "image/tar" },
    {"htm", "text/html" },
    {"html","text/html" },
    {"exe","text/plain" },
    {0,0} };

void get_file_info(const char *response)
{
    int i=0;
    char size_char[100];
    char file_namec[100];
    int size_count=0;
    
    char *pos ;
    pos = strstr(response, "filename=");
    if (pos)
        {
            //sscanf(pos, "%*s %ld", &file_in.file_length);
            size_count=0;
            i=0;
            //printf("%c %c %c",pos[9],pos[10],pos[11]);
            while(pos[i+10]!='\"')
            {
                file_in.file_name[i]=pos[i+10];
                size_count++;
                i++;
            }
        }
    pos = strstr(response, "Content-Length:");
    if (pos)
        {
                //sscanf(pos, "%*s %ld", &file_in.file_length);
            int size_count=0;
            i=0;
            //printf("%c %c %c",size_char[16],size_char[17],size_char[18]);
            while(pos[i+16]!='\r')
            {
                size_char[i]=pos[i+16];
                size_count++;
                i++;
            }
            file_in.file_length=atoi(size_char);
            /*for(i=0;i<20;i++)
            {
                printf("%c",pos[i]);
            }*/
            }
        //printf("\n");
        char boundary_char[70];
        char *boundary1;
        char *boundary2;
        char *boundary3;
        size_count=0;
        int k;
        int size__=0;
        pos = strstr(response, "boundary"); //找出boundary字串
        if (pos)
        {
            i=0;
             while(pos[i+9]!='\r')
            {
                if(pos[i+9]!='-')
                {
                    boundary_char[size_count]=pos[i+9];
                    size_count++;
                    
                }
                if(pos[i+9]=='-')
                {
                    size__++;
                }
                i++;
            }
            printf("\n");
            //printf("%p\n",pos);
            pos = strstr(pos, boundary_char);
            pos=pos+20;
            pos = strstr(pos, boundary_char);
            boundary1=pos;
            int newline=0;
            int count_useless=0;
            i=0;
            while(newline!=4)
            {
                if(newline!=0)
                {
                    count_useless++;
                }
                if(pos[i]=='\n')
                {
                    newline++;
                }
                i++;
            }
            //file_in.file_length=file_in.file_length-count_useless-size__-size_count;
            pos=pos+20;
            //printf("%p\n",pos);
            pos = strstr(pos, boundary_char);
            boundary2=pos;
    
            k=boundary2-boundary1-count_useless-size__-size_count-6;
            file_in.file_length=k;
            //printf("%d",k);
         
            }

    //return file_in;
}

void handle_socket(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char * fstr;
    static char buffer[10000];

    ret = read(fd,buffer,BUFSIZE);   /* 讀取瀏覽器要求 */

    if (ret==0||ret==-1) {
     /* 網路連線有問題，所以結束行程 */
        exit(3);
    }
    
    /* 在讀取到的字串結尾補空字元，方便後續程式判斷結尾 */
    if (ret>0&&ret<BUFSIZE)
        buffer[ret] = 0;
    else
        buffer[0] = 0;

     //**********print**********
    for (i=0;i<ret;i++)
        printf("%c",buffer[i]);
    //************************



    if ((strncmp(buffer,"GET ",4)==0)||(strncmp(buffer,"get ",4))==0)
    {
        //printf("*************");
        /*我們要把 GET /index.html HTTP/1.0 後面的 HTTP/1.0 用空字元隔開 */
        for(i=4;i<BUFSIZE;i++) {
            if(buffer[i] == ' ') {
                buffer[i] = 0;
                break;
            }
        }


        /* 要求根目錄時讀取 index.html */
        if (!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
            strcpy(buffer,"GET /index.html\0");


        /* 開啟檔案 */
        //printf("%s\n",&buffer[5]);
        if((file_fd=open(&buffer[5],O_RDONLY))==-1)
            write(fd, "Failed to open file", 19);

        /* 傳回瀏覽器成功碼 200 和內容的格式 */
        //sprintf(buffer,"HTTP/1.0 200 OK\r\n");
        //sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
        sprintf(buffer,"HTTP/1.0 200 OK \r\n\r\n");
        write(fd,buffer,strlen(buffer));

        /* 讀取檔案內容輸出到客戶端瀏覽器 */
        while ((ret=read(file_fd, buffer, BUFSIZE))>0) {
            write(fd,buffer,ret);
        }
    }
    //printf("as");
    if ((strncmp(buffer,"POST ",5)==0)||(strncmp(buffer,"post ",5))==0)
    {
        get_file_info(buffer);
        int length = 0;
        int mem_size = 4096; //mem_size might be enlarge, so reset it
        int buf_len = mem_size; //read 4k each time
        int len;

        int write_fd = open(file_in.file_name, O_CREAT | O_WRONLY|O_TRUNC, S_IRWXG | S_IRWXO | S_IRWXU);
        if (write_fd < 0)
        {
            printf("Create file failed\n");
            exit(0);
        }

        //算出檔案開始位置
        char *pos = strstr(buffer, "Content-Disposition:");
        int file_start_pos=0;
        if (pos)
        {
            int newline=0;
            i=0;
            //printf("%c %c %c",size_char[16],size_char[17],size_char[18]);
            while(newline!=3)
            {
                if(pos[file_start_pos]=='\n')
                {
                    newline++;
                }
                file_start_pos++;
            }
        }
        char *write_file;
        write_file=pos+file_start_pos;
        //printf("dd");
       if (write(write_fd,write_file,file_in.file_length))
            printf("Download successful ^_^\n\n");
        else
        {
            printf("write:fail");
        }

        //perror("open(\"index.html\",O_RDONLY)");
        file_fd=open("index.html",O_CREAT|O_RDONLY);
        if(file_fd==-1)
        {
            write(fd, "Failed to open file", 19);
            printf("qq");
        }
        //perror("open(\"index.html\",O_RDONLY)");
        /* 傳回瀏覽器成功碼 200 和內容的格式 */
        sprintf(buffer,"HTTP/1.0 200 OK \r\n\r\n");
        write(fd,buffer,strlen(buffer));

        //printf("dd");

        /* 讀取檔案內容輸出到客戶端瀏覽器 */
        while (ret=read(file_fd, buffer, BUFSIZE)) {
            //printf("%s",buffer);
            write(fd,buffer,ret);
        }
    }
    exit(1);
}

int main(int argc, char **argv)
{
    int i, pid, listenfd, socketfd;
    int length;
    static struct sockaddr_in cli_addr;
    static struct sockaddr_in serv_addr;

    /*if(chdir("/home/myubuntu/testserver/demo") == -1){ 
        printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }*/
    if(chdir("/home/myubuntu/demo/networkprogrammind1") == -1){ 
        printf("ERROR: Can't Change to directory %s\n",argv[2]);
        exit(4);
    }

    signal(SIGCHLD, sigchld_handler);
    
    /* 開啟網路 Socket */
    if ((listenfd=socket(AF_INET, SOCK_STREAM,0))<0)
        exit(3);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(8080);

    if (bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr))<0)
    { 
        perror("bind error");
        exit(3);
    }
        
    /* 開始監聽 */
    if (listen(listenfd,64)<0)
        exit(3);

    while(1) {
        length = sizeof(cli_addr);
        socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length);
        if (socketfd <0)
            exit(3);

        /* 分出子行程處理要求 */
        if ((pid = fork()) < 0) {
            exit(3);
        } else {
            if (pid == 0) {  /* 子行程 */
                close(listenfd);
                handle_socket(socketfd);
            } else { /* 父行程 */
                close(socketfd);
            }
        }
    }
}