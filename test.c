#include <stdio.h>
#include <locale.h>
#include <sysexits.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <ifaddrs.h> // Get ifaddrs struct
#include <arpa/inet.h> // Get INET_ADDRSTRLEN
#include <netinet/in.h> //Get AF_INET and AF_INET6 
#include <net/if.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>

void test(){

}
#if defined (SIOCGIFHWADDR) && !defined (HAVE_SIOCGIFHWADDR)
# define HAVE_SIOCGIFHWADDR
#endif
#if defined(HAVE_SIOCGIFHWADDR)
void get_mac_address(int socketHandle){

    struct ifreq s;

    strcpy(s.ifr_name, "eth0");
    if (ioctl(socketHandle, SIOCGIFHWADDR, &s) == 0) {
        int i;
        for (i = 0; i < 6; ++i){
            printf("%02x:", (unsigned char) s.ifr_addr.sa_data[i]);
        }
	puts("\n");
    }

}
#endif
char* getPrimaryIP(){
    char *addressBuffer = malloc(INET_ADDRSTRLEN);
    int socketHandle;
   

    // Craft a UDP socket request to google DNS
    struct sockaddr_in myaddr;
    myaddr.sin_family = AF_INET;
    int retAton = inet_aton("8.8.8.8", &myaddr.sin_addr);
    if( retAton == 0){
        fprintf(stderr,"Invalid address: inet_aton(3)\n");
        exit(1);
    }
    myaddr.sin_port = htons(53);
    
    socketHandle = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if( socketHandle == -1){
        fprintf(stderr,"%s: socket(2)\n",strerror(errno));
        exit(1);
    }
    get_mac_address(socketHandle);
    // Connect to the socket in question
    int retConnect = connect(socketHandle, (struct sockaddr*)&myaddr, (socklen_t)(sizeof(myaddr)));
    if( retConnect == -1){
        fprintf(stderr,"%s: connect(2)\n",strerror(errno));
        exit(1);
    }

    // Grab the information about local bound socket
    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    int retSockname = getsockname(socketHandle, (struct sockaddr*) &name, &namelen);
    if( retSockname == -1){
        fprintf(stderr,"%s: getsockname(2)\n",strerror(errno));
        exit(1);
    }

   // Convert the socket address and store it
   const char* p = inet_ntop(AF_INET, &name.sin_addr, addressBuffer, INET_ADDRSTRLEN);
    if( p == NULL){
        fprintf(stderr,"%s: inet_ntop(3)\n",strerror(errno));
        exit(1);
    }
    close(socketHandle);

    return addressBuffer;    

    
}

int main(int argc, char *argv[]){
    char hostname[HOST_NAME_MAX];
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;
    char *ret;
    char host[] = "127.0.0.1";
    int port = 80;
    int retHostname = gethostname(hostname,sizeof(hostname));
    if( retHostname == -1 ){
        fprintf(stderr, "%s: gethostname(2)\n",strerror(errno));
        exit(1);
    }
    printf("host name = '%s'\n", hostname);
    
    char *internetIP = getPrimaryIP();

    
    int retIP = getifaddrs(&ifAddrStruct);
    if( retIP == -1){
        fprintf(stderr,"%s: getifaddress(3)\n",strerror(errno));
        exit(1);
    }
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next){
        if (!ifa->ifa_addr){
            continue;
        }
        // Iterate through the IPv4 addresses only
        if(ifa->ifa_addr->sa_family == AF_INET){ 
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
	    char addressBuffer[INET_ADDRSTRLEN];
 	    inet_ntop(AF_INET,tmpAddrPtr,addressBuffer,INET_ADDRSTRLEN);
	    if(strcmp (internetIP,addressBuffer) == 0){
		    printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
	    }
        }
    }
    free(internetIP);
    freeifaddrs(ifAddrStruct);

    ret = setlocale(LC_ALL,"");
    if(ret == NULL){
   	fprintf(stderr, "Unable to set locale\n");
	exit(EX_OSERR);
    }
    printf("Hello World");
}
