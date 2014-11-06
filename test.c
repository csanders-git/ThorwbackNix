#include <stdio.h>
#include <locale.h>
#include <sysexits.h>
#include <stdlib.h>
#include <curl/curl.h> // For web request
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

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void build_decoding_table() {
    int i;
    decoding_table = malloc(256);

    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}

char *base64_encode(const unsigned char *data,
                    size_t input_length,
                    size_t *output_length) {
    int i,j;
    *output_length = ((input_length - 1) / 3) * 4 + 4;

    char *encoded_data = malloc(*output_length);
    if (encoded_data == NULL) return NULL;

    for (i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';

    return encoded_data;
}


unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {
    int i,j;
    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (i = 0, j = 0; i < input_length;) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}





void base64_cleanup() {
    free(decoding_table);
}

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
char *rc4(char *str, const char *key){
    char * a;
    unsigned char s[256];
    unsigned char r[256];
    int i,j,x,y,t;
    for(i = 0;i<256;i++){
        r[i] = key[i % (int)strlen(key)];
        s[i] = i;
    }
    j = 0;
    for(i=0;i<256;i++){
        j = (j + s[i] + r[i]) % 256;
        x = s[i];
        s[i] = s[j];
        s[j] = x;
    }
    i = 0;
    j = 0;
    a = (char*) malloc(strlen(str));
    if (a==NULL) exit (1);
    for(y=0;y<strlen(str);y++){
        i = (i+1) % 256;
        j = (j + s[i]) % 256;
        x = s[i];
        s[i] = s[j];
        s[j] = x;

        t = (s[(s[i] + s[j]) % 256]);
        if(t == str[y]){
            a[y] = (str[y]);
        }else{
            a[y] = (t ^ str[y]);
        }
        
    }

    return a;
}
int main(int argc, char *argv[]){
    const char *key = "ZAQwsxcde321";
    char *t = rc4("test",key);
    printf("%s\n",rc4(t,key));
    size_t x;
    printf("%s\n",base64_encode(t,strlen(t),&x));

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl){
           curl_easy_setopt(curl,CURLOPT_URL,"http://172.16.1.15/ThrowbackLP/");
           curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "name=daniel&project=curl");
    }
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        fprintf(stderr, "curl_easy_perform() failed %s\n",curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
    

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
