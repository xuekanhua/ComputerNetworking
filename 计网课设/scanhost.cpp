#include <stdio.h>
#include <winsock.h>
// #include <iostream>
#include <string>
#pragma comment(lib, "Ws2_32.lib")
int count = 0;
USHORT checksum(USHORT *buff, int size)
{
	unsigned long cksum = 0;
	while (size > 1)
	{
		cksum += *buff++;
		size -= sizeof(USHORT);
	}
	if (size)
	{
		cksum += *(UCHAR *)buff;
	}
	// 将32位的chsum高16位和低16位相加，然后取反
	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >> 16);
	return (USHORT)(~cksum);
}

// // IP报文
// typedef struct iphdr
// {							  
// 	unsigned int headlen : 4; // IP头长度
// 	unsigned int version : 4; // IP版本号
// 	unsigned char tos;		  //服务类型
// 	unsigned short id;		  // ID号
// 	unsigned short flag;	  //标记
// 	unsigned char ttl;		  //生存时间
// 	unsigned char prot;		  //协议
// 	unsigned short checksum;  //效验和
// 	unsigned int sourceIP;	  //源IP
// 	unsigned int destIP;	  //目的IP
// } IpHeader;


// ICMP报文
typedef struct icmp_hdr
{
	unsigned char icmp_type;	  // 消息类型
	unsigned char icmp_code;	  // 代码
	unsigned short icmp_checksum; // 校验和
	unsigned short icmp_id;		  // 用来惟一标识此请求的ID号，通常设置为进程ID
	unsigned short icmp_sequence; // 序列号
	unsigned long icmp_timestamp; // 时间戳
}  ICMPHeader;

//设置超时时间
int SetTimeout(SOCKET s, int nTime, BOOL bRecv)
{
	int ret = setsockopt(s, SOL_SOCKET, bRecv ? SO_RCVTIMEO : SO_SNDTIMEO, (char *)&nTime, sizeof(nTime));
	return ret != SOCKET_ERROR;
}


//扫描主机是否存活
int SacnComputer(char szDestIP[30]) 
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	//启动网络库
	if (WSAStartup(wVersionRequested, &wsaData))
	{
		printf("Winsock初始化失败.\n");
		exit(1);
	}

	//创建原始套接字
	//AF_INET表示地址族为IPV4
	//SOCK_RAW表示创建的为原始套接字，在Windows环境下使用管理员权限运行程序
	SOCKET sRaw =  socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

	SetTimeout(sRaw, 1000, TRUE);
	
	/*
	处理网络通信的地址。
	struct sockaddr_in {
	short	        sin_family;
	u_short	        sin_port;
	struct in_addr	sin_addr;
	char	        sin_zero[8];
	};
	*/
	SOCKADDR_IN dest;

	//填写目的主机相关信息，不需要填写端口号，因为ICMP是网络层协议
	dest.sin_family = AF_INET;//IPv4
	// htons是将整型变量从主机字节顺序转变成网络字节顺序
	dest.sin_port = htons(0);//将端口号转换为网络字节序
	dest.sin_addr.S_un.S_addr = inet_addr(szDestIP);//目标IP

	//创建ICMP数据包
	char buff[sizeof( ICMPHeader) + 32];
	 ICMPHeader *pIcmp = ( ICMPHeader *)buff;

	//初始化ICMP包  
	pIcmp->icmp_type = 8;
	pIcmp->icmp_code = 0;
	pIcmp->icmp_id = (USHORT) GetCurrentProcessId();
	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_sequence = 0;

	memset(&buff[sizeof( ICMPHeader)], 'A', 32);


	USHORT nSeq = 0;
	char revBuf[1024];
	SOCKADDR_IN from;

	int nLen = sizeof(from);
	static int nCount = 0;
	int nRet;

	pIcmp->icmp_checksum = 0;
	pIcmp->icmp_timestamp =  GetTickCount();
	pIcmp->icmp_sequence = nSeq++;
	//计算checksum
	pIcmp->icmp_checksum = checksum((USHORT *)buff, sizeof( ICMPHeader) + 32);
	//发送数据
	nRet =  sendto(sRaw, buff, sizeof( ICMPHeader) + 32, 0, (SOCKADDR *)&dest, sizeof(dest));

	if (nRet == SOCKET_ERROR)
	{
		printf("发送失败:%d\n",  WSAGetLastError());
		return -1;
	}
	//接收数据
	nRet =  recvfrom(sRaw, revBuf, 1024, 0, (sockaddr *)&from, &nLen);
	//没有接受到数据即发送失败
	if (nRet == SOCKET_ERROR)
	{
		// printf("%s 主机没有存活！\n",szDestIP);
		return -1;
	}
	printf("活动主机%d:%s \n", ++ count, szDestIP);
	closesocket(nRet);
	WSACleanup();
	return 0;
}

void ScanPort(char adr[20]) //扫描存活主机端口
{
	int mysocket, port = 0; //这里扫描的端口
	struct sockaddr_in my_addr;
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(1, 1);
	while(1)
	{
		printf("请输入要扫描的端口,-1 退出端口扫描,\n");
		scanf("%d",&port);
		if(port == -1)
		{
			printf("退出端口扫描\n");
			break;
		}
		if (WSAStartup(wVersionRequested, &wsaData))
		{
			printf("Winsock初始化失败\n");
			exit(1);
		}
		//TCP
		if ((mysocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
		{
			exit(1);
		}
		
		my_addr.sin_family = AF_INET;
		my_addr.sin_port = htons(port);
		my_addr.sin_addr.s_addr = inet_addr(adr);

		//链接
		if (connect(mysocket, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == SOCKET_ERROR)
		{
			printf("Port %d 关闭\n", port);
			closesocket(mysocket);
		}
		else
		{
			printf("Port %d 打开\n", port);
		}
	
	}
	//关闭连接
	closesocket(mysocket);
	//中止Windows Sockets在所有线程上的操作
	WSACleanup();
}

// IP转换
void change(int a, int b, int c, int d, char IP[20]) 
{
	char IPPort[4][4] = {'\0'};
	char temp[2] = {'.', '\0'};
	//itoa 用于把整数转换成字符串
	itoa(a, IPPort[0], 10);
	itoa(b, IPPort[1], 10);
	itoa(c, IPPort[2], 10);
	itoa(d, IPPort[3], 10);
	//字符串拼接
	strcat(IP, IPPort[0]);
	strcat(IP, temp);
	strcat(IP, IPPort[1]);
	strcat(IP, temp);
	strcat(IP, IPPort[2]);
	strcat(IP, temp);
	strcat(IP, IPPort[3]);
	// std::cout<< IP << std::endl;
}

int main(int argc, char *argv[])
{
	char sa[8][4];
	int a[8];
	int cnt = 0, count = 0;
	for (int i = 1; i < argc; i ++)
	{
		for(int j = 0; argv[i][j] != '\0'; j ++)
		{
			if(argv[i][j] == '.')
			{
				cnt = 0;
				a[count] = std::stoi(sa[count]);
				count ++;
				continue;
			}
			sa[count][cnt ++] = argv[i][j];
		}
		a[count] = std::stoi(sa[count]);
		count ++;
		cnt = 0;
	}
	// std::cout << "\n\n\n";
	for(int i = 0; i < 8; i ++)
	{
		// std::cout << a[i] << "  \n";
		if(a[i] > 255 || a[i] < 0)
		{
			printf("请重新输入ip范围\n");
			return 0;
		}
	}

	while (!(a[0] == a[4] && a[1] == a[5] && a[2] == a[6] && a[3] == (a[7] + 1)))
	{
		char IP[20] = {'\0'};
		change(a[0], a[1], a[2], a[3], IP);
		if ((SacnComputer(IP)) == 0)
		{
			ScanPort(IP);
		}
		a[3]++;
		for(int i = 3; i >= 0; i --)
		{
			if(i == 0 && a[i] >= 255)
			{
				printf("地址溢出！\n");
				break;
			}
			if(a[i] >= 255)
			{
				a[i] = 0;
				a[i - 1] ++;
			}
		}
	}
	return 0;
}
