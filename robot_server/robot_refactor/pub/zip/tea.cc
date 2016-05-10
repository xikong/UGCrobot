/*
# Copyright (c) 2015, Kunyan Tech.Co.,Ltd.
# FileName: tea.c
# Author : luoxudong
# Date : 20151211
# Description : TEA jiami and jiemi
# Others : 
# Functoin List : 
 1.void encrypt(int *v1, int *v2,int *k,int rounds);
 2.void decrypt(int *v1,int *v2, int *k,int rounds);
 3.char* str_en(char *clearmsg,int rounds);
 4.char* str_de(char *encryptedmsg,unsigned long rounds);
# History : 
*/

#include <stdio.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "tea.h"
namespace en_de {

unsigned int *key = (unsigned int *)"1234567890abcdek";  // NOLINT //key

// jiami
void encrypt(unsigned int *v1, unsigned int *v2, unsigned int *k, int rounds) {
    unsigned int y = *v1, z = *v2, sum = 0, i = 0;
    unsigned int delta = 0x9e3779b9;
    int a = k[0], b = k[1], c = k[2], d = k[3];
    for (i=0; i < rounds; i++) {
      sum += delta;
      y += ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
      z += ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
    }
    *v1 = y;
    *v2 = z;
}

// jiemi
void decrypt(unsigned int *v1, unsigned int *v2, unsigned int *k, int rounds) {
    unsigned int y = *v1, z = *v2, i = 0, sum;
    if (rounds == 32)
        sum = 0xc6ef3720;
    else
        sum = 0xe3779b90;
        // sum = 0x9e3779b9;
    unsigned int delta = 0x9e3779b9;
    int a = k[0], b = k[1], c = k[2], d = k[3];
    for (i = 0; i < rounds; i++) {
      z -= ((y << 4) + c) ^ (y + sum) ^ ((y >> 5) + d);
      y -= ((z << 4) + a) ^ (z + sum) ^ ((z >> 5) + b);
      sum -= delta;
    }
    *v1 = y;
    *v2 = z;
}

// 16 or 32 rounds jiami
char* str_en_8byte(char *clearmsg, int rounds) {
    int len = strlen(clearmsg);
    int n = len/8;
    int len1 = (n+1)*8;
    int  i;
    char *en = (char*)malloc(len1*sizeof(char));  // NOLINT
    memcpy(en, clearmsg, strlen(clearmsg));
    for (i = len; i < len1-len; i++)
        *(en+i) = '\0';
    unsigned int *y1, *y2;
    for (i = 0; i < len1; i += 8) {
        y1 = (unsigned int *)&en[i];  // NOLINT
        y2 = (unsigned int *)&en[i+4];  // NOLINT
        encrypt(y1, y2, key, rounds);
    }
    en[len1] = '\0';
    return en;
}


// 16 or 32 rounds jiemi
char* str_de_8byte(char *encryptedmsg, int rounds) {
    int i;
    int len = strlen(encryptedmsg);
    char *de = (char*)malloc(len*sizeof(char));  // NOLINT
    memcpy(de, encryptedmsg, sizeof(encryptedmsg));
    unsigned int *y1, *y2;
    for (i = 0; i < len; i += 8) {
        y1 = (unsigned int *)&de[i];  // NOLINT
        y2 = (unsigned int *)&de[i+4];  // NOLINT
        decrypt(y1, y2, key, rounds);
    }
    *(de+len) = '\0';
    return de;
}

string str_en(string a, int rounds) {
    char destCh[100];
    memset(destCh, '\0', sizeof(destCh));
	int start = 0, end = 0, num = 0, i;
	string temp[100];
	for (i=0; i<a.length(); i+=8) {
	    if (i+8 > a.length()) {
	        temp[num] = a.substr(i, a.length()-i+1);
	        num++;
	    } else {
	        temp[num] = a.substr(i, 8);
	        num++;
	    }
	}
	int j;
	char* tempCh = NULL;
	for (j=0; j<num; j++)
	{
	    tempCh = str_en_8byte((char *)temp[j].c_str(), 16);
	    strcat(destCh, tempCh);
	}
    string strEn = destCh;
    return strEn;
}

string str_de(string strEn, int rounds) {
    char* tempCh = NULL;
    int num = 0, i, j;
    char destCh[100];
	memset(destCh, '\0', sizeof(destCh));
	string tempDe[100];
    for (i=0; i<strEn.length(); i+=8) {
	    if (i+8 > strEn.length()) {
	        tempDe[num] = strEn.substr(i, strEn.length()-i+1);
	        num++;
	    } else {
	        tempDe[num] = strEn.substr(i, 8);
	        num++;
	    }
	}
	for (j=0; j<num; j++)
	{
	    tempCh = str_de_8byte((char *)tempDe[j].c_str(), 16);
	    strcat(destCh, tempCh);
	}
	string strDe = destCh;
	return strDe;
}

void dayin(void) {
    printf("--------------------\n");
    //string a = "123,09876543210123456789123452342342342342342342346789hello";
    string a = "1283784816481helo!";
	string b = str_en(a, 16);
	cout << "miwen : " << b << endl;
	string c = str_de(b, 16);
	cout << "yuanwen : " << c << endl;
	return;
}

}
/*int main()
{
	//string a = "123,09876543210123456789123452342342342342342342346789";
	string a = "123,09876543210123456789123452342342342342342342346789hello";
	string b = str_en(a, 16);
	cout << "miwen : " << b << endl;
	string c = str_de(b, 16);
	cout << "yuanwen : " << c << endl;

	return 0;
}*/
