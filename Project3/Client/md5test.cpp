#include <string.h>
#include <stdio.h>
#include <openssl/md5.h>
#include <iostream>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[]) {
    unsigned char digest[16];
    const char* string = "Hello World";
    MD5_CTX context;
    MD5_Init(&context);
    MD5_Update(&context, string, strlen(string));
    MD5_Final(digest, &context);

    for(int i = 0; i < 16;i++){
        //cout << isprint(digest[i]);
        printf("%02x", digest[i]);
    }
    cout << endl;

    return 0;
}

int GetMD5(string filename){
    //TODO: test and compile
}

