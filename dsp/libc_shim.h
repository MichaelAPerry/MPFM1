/* SPDX-License-Identifier: MIT. Freestanding device builds have no libc: base/bsp/chip/libc.c supplies memset/memcpy/memcmp;
 * this adds the few others the app uses. Host and WASM builds use the real C library. */
#ifndef SC_LIBC_SHIM_H
#define SC_LIBC_SHIM_H
#ifndef __PI32V2__
#include <string.h>
#include <stdio.h>
#else
#include <stdarg.h>
void *memset(void *d, int c, unsigned n); void *memcpy(void *d, const void *s, unsigned n); int memcmp(const void *a, const void *b, unsigned n);
static void *memmove(void *d, const void *s, unsigned n) { unsigned char *o=d; const unsigned char *i=s; if(o<i)while(n--)*o++=*i++; else while(n--)o[n]=i[n]; return d; }
static int strcmp(const char *a, const char *b) { while(*a&&*a==*b){a++;b++;} return (unsigned char)*a-(unsigned char)*b; }
/* %s %d %u with optional '+', '0' and width: all the app's format strings need. */
static int snprintf(char *b, unsigned n, const char *f, ...) {
    va_list ap; unsigned o=0; va_start(ap,f);
    #define SC_PUT(c) do{ char c_=(char)(c); if(o+1<n)b[o]=c_; o++; }while(0)
    for(;*f;f++){
        if(*f!='%'){SC_PUT(*f);continue;}
        int plus=0,zero=0,w=0; f++;
        if(*f=='+'){plus=1;f++;} if(*f=='0'){zero=1;f++;} while(*f>='0'&&*f<='9')w=w*10+*f++-'0';
        if(*f=='s'){const char *s=va_arg(ap,const char*); while(*s)SC_PUT(*s++);continue;}
        char t[12]; int k=0,neg=0; unsigned v;
        if(*f=='d'){int x=va_arg(ap,int); neg=x<0; v=neg?0u-(unsigned)x:(unsigned)x;} else v=va_arg(ap,unsigned);
        do{t[k++]=(char)('0'+v%10);v/=10;}while(v);
        int sign=neg||(plus&&*f=='d'); if(sign)w--;
        if(!zero)while(w-->k)SC_PUT(' ');
        if(sign)SC_PUT(neg?'-':'+');
        if(zero)while(w-->k)SC_PUT('0');
        while(k)SC_PUT(t[--k]);
    }
    #undef SC_PUT
    if(n)b[o<n?o:n-1]=0; va_end(ap); return (int)o;
}
#endif
#endif
