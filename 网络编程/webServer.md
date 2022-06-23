# webServer

*   第一版

    ```c++
    #include<stdio.h>
    #include<stdlib.h>
    #include<sys/socket.h>
    #include<sys/types.h>
    #include<sys/stat.h>
    #include<sys/sendfile.h>
    #include<fcntl.h>
    #include<netinet/in.h>
    #include<arpa/inet.h>
    #include<assert.h>
    #include<unistd.h>
    #include<string.h>
    #include"HttpParser.h"
    #include<string>
    const int port = 8000;
    int main(int argc,char *argv[])
    {
        if(argc<0)
        {
            printf("need two canshu\n");
            return 1;
        }
        int sock;
        int connfd;
        struct sockaddr_in sever_address;
        bzero(&sever_address,sizeof(sever_address));
        sever_address.sin_family = PF_INET;
        sever_address.sin_addr.s_addr = htons(INADDR_ANY);
        sever_address.sin_port = htons(8000);
     
        sock = socket(AF_INET,SOCK_STREAM,0);
     
        assert(sock>=0);
     
        int ret = bind(sock, (struct sockaddr*)&sever_address,sizeof(sever_address));
        assert(ret != -1);
     
        ret = listen(sock,1);
        assert(ret != -1);
        while(1)
        {
            struct sockaddr_in client;
            socklen_t client_addrlength = sizeof(client);
            connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);
            if(connfd<0)
            {
                printf("errno\n");
            }
            else{
                    char request[4096];
                    memset(request, 0, sizeof(request));
                    recv(connfd,request,sizeof (request),0);
                    HttpParser hp(request);
                    // printf("%s\n",request);
                    // printf("ok ++++++++++++++++++\n");
                    char rpath[4096];
                    strcpy(rpath, hp.http["path"].c_str());
                    printf("%s", rpath + 1);
                    // printf("\n+++++++++++++++\n");
                    // printf("successeful!\n");
                    char buf[4096];
                    int fd = open(rpath+1,O_RDONLY);//消息体
                    printf("%d\n", fd);
                    if(fd == -1)
                    {
                        strcpy(buf, "HTTP/1.1 404 Not Found\r\nconnection: close\r\n\r\n");
                        fd = open("NotFound.html",O_RDONLY);
                    }
                    else
                    {
                        strcpy(buf, "HTTP/1.1 200 ok\r\nconnection: close\r\n\r\n");
                    }
                    int s = send(connfd,buf,strlen(buf),0);//发送响应

                    sendfile(connfd,fd,NULL,2500);//零拷贝发送消息体
                    close(fd);
                    close(connfd);
            }
        }
        return 0;
    }
    ```
