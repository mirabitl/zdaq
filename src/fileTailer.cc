#include <stdio.h>
#include <stdlib.h>
#include "fileTailer.hh"

fileTailer::fileTailer(uint32_t mb) : _MAXBUFF(mb){}
/* findTail stores characters from stdin in the buffer 'buff'. When it finds
 * the end of a line, it stores the pointer for the beginning of that line in
 * 'lines'. once nlines have been found, pointers to previous lines are shifted
 * off of the end of 'lines'. If there is space at the start of 'buff' not
 * pointed to by 'lines', then the end of a line that hits the end of the
 * buffer can continue its storage at the beginning of the buffer. This makes
 * the best use of a fixed-sized buffer for long input. */

int fileTailer::fileTail(FILE* s,char *lines[][2], int nlines, char buff[], int maxbuff)
{
  char *buffp, *linestart;
  int i, c, wrap, nfound;

  for (i=0; i < nlines; ++i) {
    lines[i][0] = NULL; // [0] for storing line, or beginning of wrapped line
    lines[i][1] = NULL; // [1] for storing second half of a wrapped line
  }

  nfound = 0;
  wrap = 0;
  linestart = buffp = buff;
  while ((c=getc(s)) != EOF) {
    if (buffp == linestart && wrap == 0) {
      if (nfound < nlines)
        ++nfound;
      shift(lines, nlines);
    }

    if (buffp - buff == maxbuff - 1) {
      *buffp = '\0';
      lines[nlines - 1][0] = linestart;
      wrap = 1;
      linestart = buffp = buff;
    }

    if (!testForRoom(lines, nlines - nfound, buffp)) break;

    *buffp++ = c;
    if (c == '\n') {
      *buffp++ = '\0';
      lines[nlines - 1][wrap] = linestart;
      wrap = 0;
      if (buffp - buff >= maxbuff - 1)
        buffp = buff;
      linestart = buffp;
    }

  }
  // this is in case the input ended without a newline.
  if (c == EOF && buffp != buff && buffp[-1] != '\0') {
    testForRoom(lines, nlines - nfound, buffp);
    *buffp = '\0';
    lines[nlines - 1][wrap] = linestart;
  }

}


int fileTailer::findTail(char *lines[][2], int nlines, char buff[], int maxbuff)
{
  char *buffp, *linestart;
  int i, c, wrap, nfound;

  for (i=0; i < nlines; ++i) {
    lines[i][0] = NULL; // [0] for storing line, or beginning of wrapped line
    lines[i][1] = NULL; // [1] for storing second half of a wrapped line
  }

  nfound = 0;
  wrap = 0;
  linestart = buffp = buff;
  while ((c=getchar()) != EOF) {
    if (buffp == linestart && wrap == 0) {
      if (nfound < nlines)
        ++nfound;
      shift(lines, nlines);
    }

    if (buffp - buff == maxbuff - 1) {
      *buffp = '\0';
      lines[nlines - 1][0] = linestart;
      wrap = 1;
      linestart = buffp = buff;
    }

    testForRoom(lines, nlines - nfound, buffp);

    *buffp++ = c;
    if (c == '\n') {
      *buffp++ = '\0';
      lines[nlines - 1][wrap] = linestart;
      wrap = 0;
      if (buffp - buff >= maxbuff - 1)
        buffp = buff;
      linestart = buffp;
    }

  }
  // this is in case the input ended without a newline.
  if (c == EOF && buffp != buff && buffp[-1] != '\0') {
    if (testForRoom(lines, nlines - nfound, buffp))
      {
	*buffp = '\0';
	lines[nlines - 1][wrap] = linestart;
      }
  }

}

/* shift is used upon finding a character that starts a new line. It shifts
 * line pointers in the pointer array to the left, making room for new line
 * pointer(s) and forgetting the pointer(s) for the oldest line in memory. */

void fileTailer::shift(char *lines[][2], int nlines)
{
  int i;
  for (i=0; i < nlines - 1; ++i) {
    lines[i][0] = lines[i + 1][0];
    lines[i][1] = lines[i + 1][1];
  }
  lines[nlines - 1][0] = NULL;
  lines[nlines - 1][1] = NULL;
}

/* testForRoom tests to see if the location for (or the location following the)
 * next character that would be placed in the buffer is pointed to by a line in
 * the lines pointer array. */

bool fileTailer::testForRoom(char *lines[][2], int index, char *buffp) {
  if (buffp == lines[index][0]
      || buffp + 1 == lines[index][0]) {
    printf("error: not enough room in buffer.\n");
    return false;
  }
  return true;
}



void fileTailer::tail(std::string name,uint32_t nlines, char* buf)
{
  char *lines[nlines][2],buff[_MAXBUFF];

  FILE* f=fopen(name.c_str(),"r");

  fileTail(f,lines, nlines, buff,_MAXBUFF);
  int ib=0,nb=0;
  for (int i=0; i < nlines; ++i) {
    if (lines[i][0] != NULL && ib<_MAXBUFF)
      {nb=sprintf(&buf[ib],lines[i][0]);ib+=nb;	
      }
    if (lines[i][1] != NULL && ib<_MAXBUFF)
      {
	nb=sprintf(&buf[ib],lines[i][0]);ib+=nb;
      }
  }
}
 

 
/* main() processes optional cli argument '-n', where n is a number of lines.
 * The default is 10.  findTail finds the last n lines from the input so that
 * they can be printed. */
#ifdef FILETAILER_MAIN
#include <iostream>
int main(int argc, char *argv[])
{
  fileTailer t((uint32_t) 500*1024);
  char buf[500*1024];
  t.tail("/tmp/dimjcPID31503.log",2000,&buf[0]);
  std::cout<<buf<<std::endl;
}
#endif
