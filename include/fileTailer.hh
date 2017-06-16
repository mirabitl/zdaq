#ifndef _FILETAILER_H
#define _FILETAILER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string>



class fileTailer
{
 public:
  fileTailer(uint32_t maxbuff);
  void tail(std::string name,uint32_t nl, char* buf);
 private:
  int findTail(char *lines[][2], int nlines, char buff[], int maxbuff);
  int fileTail(FILE* s,char *lines[][2], int nlines, char buff[], int maxbuff);
  void shift(char *lines[][2], int nlines);
  bool testForRoom(char *lines[][2], int index, char *buffp);
  uint32_t _MAXBUFF;
};  
#endif 
