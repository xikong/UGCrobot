#ifndef TEA_H
#define TEA_H
#include <string>
#include <iostream>
namespace en_de {
using namespace std;

//jiami
char* str_en_8byte(char *clearmsg,int rounds);
//jiemi
char* str_de_8byte(char *encryptedmsg,unsigned long rounds);

//fengzhuang
string str_en(string a, int rounds);

string str_de(string strEn, int rounds);

void dayin(void);

}

#endif/*TEA_H*/