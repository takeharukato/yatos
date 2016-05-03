/* -*- mode: C; coding:utf-8 -*- */
/**********************************************************************/
/*  Yet Another Teachable Operating System                            */
/*  Copyright 2016 Takeharu KATO                                      */
/*                                                                    */
/*  String operation definitions                                      */
/*                                                                    */
/**********************************************************************/
#if !defined(__KERN_STRING_H)
#define __KERN_STRING_H
#include <stddef.h>

size_t strnlen(char const *, size_t );
void *memcpy(void *, const void *, size_t );
void *memmove(void *, const void *, size_t );
void *memset(void *, int , size_t );
int memcmp(const void *, const void *, size_t );
int strcmp(char const *, char const *);
char *strncpy(char *, char const *, size_t );
char *strcpy(char *, char const *);
char *strchr(const char *, int );
char *strrchr(const char *, int );
char *strstr(const char *, const char *);
char *strncat(char *, char const *, size_t );
int strncmp(const char *, const char *, size_t );
size_t strlen(char const *);
size_t strnlen(char const *, size_t );
#endif  /*  __KERN_STRING_H  */
