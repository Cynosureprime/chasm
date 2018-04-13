#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <Judy.h>

#ifdef ICONV
#include <iconv.h>
#endif
#ifdef WIN
#include <fcntl.h>
#endif

#define MAXLINE 20000    /* maximum line length */

/*
 *$Log: chasm.c,v $
 *Revision 1.6  2016/11/18 14:42:30  waffle
 *Add new options from blazer, clean code, remove zero-length files.
 *
 *Revision 1.3  2016/11/18 02:05:10  waffle
 *Add rule generation
 *
 *Revision 1.2  2016/11/18 01:33:04  waffle
 *Added option processing, sorting, etc.
 *
 *Revision 1.1  2016/11/17 15:31:54  waffle
 *Initial revision
 *
 *
 */


Pvoid_t Left[256], Right[256], Lefthex[256], Righthex[256];
Pvoid_t Words;
struct Sort {
  Word_t count;
  char *line;
} *Tsort;

unsigned char trhex[] = {
    17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 16, 16, 17, 16, 16, /* 00-0f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 10-1f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 20-2f */
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 16, 16, 16, 16, 16, 16, /* 30-3f */
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 40-4f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 50-5f */
    16, 10, 11, 12, 13, 14, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 60-6f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 70-7f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 80-8f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* 90-9f */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* a0-af */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* b0-bf */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* c0-cf */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* d0-df */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, /* e0-ef */
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};/* f0-ff */

static int xsort(const void *p1, const void *p2) {
  struct Sort *pa = (struct Sort *) p1;
  struct Sort *pb = (struct Sort *) p2;
  return (pb->count - pa->count);
}

static int get32(char *iline, unsigned char *dest, int len) {
  unsigned char c, c1, c2, *line = iline;
  int cnt;
  unsigned char *tdest;
  uint64_t *curi, i;

  cnt = 0;
  while ((c = *line++)) {
    c1 = trhex[c];
    c2 = trhex[*line];
    if (c1 > 16 || c2 > 16)
      break;
    if (c1 < 16 && c2 < 16) {
      tdest = dest;
      cnt = 1;
      *tdest++ = (c1 << 4) + c2;
      line++;
      curi = (uint64_t *) line;
      while (1) {
        i = *curi++;
        c1 = trhex[(i & 255)];
        c2 = trhex[(i >> 8) & 255];
        if (c1 > 15 || c2 > 15 || cnt >= len)
          goto get32_exit;
        *tdest++ = (c1 << 4) + c2;
        cnt++;
        i >>= 16;
        c1 = trhex[(i & 255)];
        c2 = trhex[(i >> 8) & 255];
        if (c1 > 15 || c2 > 15 || cnt >= len)
          goto get32_exit;
        *tdest++ = (c1 << 4) + c2;
        cnt++;
        i >>= 16;
        c1 = trhex[(i & 255)];
        c2 = trhex[(i >> 8) & 255];
        if (c1 > 15 || c2 > 15 || cnt >= len)
          goto get32_exit;
        *tdest++ = (c1 << 4) + c2;
        cnt++;
        i >>= 16;
        c1 = trhex[(i & 255)];
        c2 = trhex[(i >> 8) & 255];
        if (c1 > 15 || c2 > 15 || cnt >= len)
          goto get32_exit;
        *tdest++ = (c1 << 4) + c2;
        cnt++;
      }
    }
  }
  get32_exit:
  return (cnt);
}

char *prhex(unsigned char *hex, char *out, int len) {
  char *ob;
  static char hextab[16] = "0123456789abcdef";
  int x;
  unsigned char v;

  ob = out;
  for (x = 0; x < len / 2; x++) {
    v = *hex++;
    *ob++ = hextab[(v >> 4) & 0xf];
    *ob++ = hextab[v & 0xf];
  }
  *ob = 0;
  return (out);
}

int main(int argc, char *argv[]) {
  FILE *FO1, *FO2;
  char buf[MAXLINE + 128], c, dbuf[(MAXLINE + 128) * 5], obuf[(MAXLINE + 128) * 5], *s;
  char *in, *out, *progname;
  size_t linecnt = 0;
#ifdef ICONV
  size_t inlen, outlen;
#endif
  unsigned char v;
  int len, hexflag, midflag, lenflag, x, opt, analfreq, dosort, curlen, dorule, ordflag, ordchar, maxlen, keepOccur;
  size_t tlen;
  Word_t *PV, rc, memsum;

#ifdef ICONV
  iconv_t utf, cp1252;
#endif

#ifdef WIN
  setmode (0, O_BINARY);
#endif

  midflag = 1;
  ordflag = 0;
  memsum = 0;
  lenflag = 0;
  maxlen = 0;
  keepOccur = 0;
  dorule = analfreq = ordchar = dosort = 0;

  progname = argv[0];
  while ((opt = getopt(argc, argv, "acsm:r:o:l:k:")) != -1) {
    switch (opt) {
      case 'a':
        printf("Analyze frequency of word parts\n");
        analfreq = 1;
        break;
      case 'c':
        printf("Create rule files for each left/right word lists\n");
        dorule = 1;
        break;
      case 's':
        printf("Sort by analyzed frequency of parts\n");
        dosort = 1;
        break;
      case 'o':
        if (*optarg >= '0' && *optarg <= '9')
          ordchar = atoi(optarg);
        else
          ordchar = *optarg;
        if (ordchar < 0 || ordchar > 255) {
          fprintf(stderr, "Invalid decimal or character specified for ordchar: %s\n", optarg);
          exit(1);
        }
        printf("Only output specific ordinal split character: %c (%d)\n", ordchar, ordchar);
        ordflag = 1;
        break;
      case 'l':
        maxlen = atoi(optarg);
        if (maxlen <= 0 || maxlen > MAXLINE) {
          fprintf(stderr, "Invalid maximum length: %s\n", optarg);
          exit(1);
        }
        printf("Skip processing all strings longer than %s characters\n", optarg);

        break;
      case 'k':
        printf("Ignore splits with occurrence less than %s\n", optarg);
        keepOccur = atoi(optarg);
        break;
      case 'm':
        midflag = 0;
        lenflag = atoi(optarg);
        if (lenflag <= 0 || lenflag > MAXLINE) {
          fprintf(stderr, "Invalid midpoint specified: %s\n", optarg);
          exit(1);
        }
        printf("Midpoint set to %d characters\n", lenflag);
        break;
      case 'r':
        midflag = 1;
        lenflag = atoi(optarg);
        if (lenflag <= 0 || lenflag > MAXLINE) {
          fprintf(stderr, "Invalid range specified: %s\n", optarg);
          exit(1);
        }
        printf("Midpoint range is +/- %d characters\n", lenflag);
        break;
      default:
      usage:
        printf("Usage %s [-a] [-s] [-m splitpoint] [-r range] [-o charcode] [-l maxLen] [-k minOccur] basefilename\n\n", progname);
        printf("-a will output the frequency analysis of each word part to the output\nfile.  Primarily used for detailed analysis of data\n\n");
        printf("-c creates rule files out of each of the left/right files.  Use with\nany of the other switches.\n\n");
        printf("-s sorts the output data by frequency - highest frequency first\n\n");
        printf("-m changes from half of each word to an absolute character position\n\n");
        printf("-r allows a range around the half-word point\n\n");
        printf("-o only split and output on this character (ascii code/ordinal)\n\n");
        printf("-l limit the max string length, strings longer than this value will not be split\n\n");
        printf("-k keep split half if there are more than X instances\n\n");
        printf("The base file name/path is used for each of the output files, by\nappending .L### and .R###, where ### is the ordinal value of the last\ncharacter of the left-hand word\n");
        exit(0);
    }
  }
  argc -= optind;
  argv += optind;
  if (argc < 1) {
    goto usage;
  }
  if (argc > 1) {
    lenflag = atoi(argv[1]);
    if (lenflag > 0 && lenflag < MAXLINE)
      midflag = 0;
    if (argv[1][0] == '+')
      midflag = 1;
  }

  if (strlen(argv[0]) > MAXLINE / 2) {
    fprintf(stderr, "Invalid basename specified: %s\n", argv[0]);
    exit(1);
  }
  strcpy(dbuf, "$HEX[");
#ifdef ICONV
  utf = iconv_open("windows-1252","utf8//IGNORE");
  if (utf == (iconv_t)-1 ) {
    perror("iconv to utf8");
    exit(1);
  }
  cp1252 = iconv_open("utf8","windows-1252//IGNORE");
  if (cp1252 == (iconv_t)-1 ) {
    perror("iconv to windows-1252");
    exit(1);
  }
#endif

  while (fgets(buf, MAXLINE, stdin)) {
    linecnt++;
    len = 0;
    s = buf;
    hexflag = 0;

    while (len < MAXLINE && (c = *s)) {
      if (c == 10 || c == 13) {
        *s = 0;
        break;
      }
      if (c < ' ')
        hexflag |= 1;
      s++;
      len++;
    }
    if (len == 0) continue;


    if (len > 8 && strncmp(buf, "$HEX[", 5) == 0) {
      len = get32(buf + 5, buf, len - 6);
      hexflag = 1;
    }
#ifdef ICONV
  /* Ignore this for now */
  if (hexflag) {
    in = buf; out = obuf; inlen = len; outlen = 40000;
    tlen = iconv(utf,&in,&inlen,&out,&outlen);
    outlen = 40000-outlen;
    if (tlen != -1 && outlen) {
      prhex(obuf,&dbuf[5],outlen*2);
      dbuf[5+outlen*2] = ']';
      dbuf[6+outlen*2] = '\n';
      dbuf[7+outlen*2] = 0;
      fwrite(dbuf,outlen*2+7,1,FO1);
    }

    in = buf; out = obuf; inlen = len; outlen = 40000;
    tlen = iconv(cp1252,&in,&inlen,&out,&outlen);
    outlen = 40000-outlen;
    if (tlen != -1 && outlen) {
      prhex(obuf,&dbuf[5],outlen*2);
      dbuf[5+outlen*2] = ']';
      dbuf[6+outlen*2] = '\n';
      dbuf[7+outlen*2] = 0;
      fwrite(dbuf,outlen*2+7,1,FO2);
    }
  }
#endif
    if (maxlen && len > maxlen)
      continue;
    if (midflag) {

      tlen = len / 2;
      for (curlen = len / 2 - abs(lenflag); curlen <= len / 2 + abs(lenflag); curlen++) {
        tlen = curlen;
        if (tlen < 0 || tlen > len)
          continue;
        if (tlen > 0)
          v = buf[tlen - 1];
        else
          v = buf[0];
        if (ordflag && v != ordchar)
          continue;
        if (hexflag) {
          prhex(buf, &dbuf[5], tlen * 2);
          JSLI(PV, Lefthex[v], &dbuf[5]);
          if (PV)
            (*PV)++;
          in = buf + tlen;
          tlen = len - tlen;
          prhex(in, &dbuf[5], tlen * 2);
          JSLI(PV, Righthex[v], &dbuf[5]);
          if (PV)
            (*PV)++;
        } else {
          strncpy(obuf, buf, tlen);
          obuf[tlen] = 0;
          JSLI(PV, Left[v], obuf);
          if (PV)
            (*PV)++;

          strncpy(obuf, buf + tlen, len - tlen);
          tlen = len - tlen;
          obuf[tlen] = 0;
          JSLI(PV, Right[v], obuf);
          if (PV)
            (*PV)++;
        }
      }
    } else {
      tlen = lenflag;
      if (tlen > len)
        tlen = len;
      if (tlen > 0)
        v = buf[tlen - 1];
      else
        v = buf[0];
      if (ordflag && v != ordchar)
        continue;
      if (hexflag) {
        prhex(buf, &dbuf[5], tlen * 2);
        JSLI(PV, Lefthex[v], &dbuf[5]);
        if (PV)
          (*PV)++;
        in = buf + tlen;
        tlen = len - tlen;
        if (tlen > 0) {
          prhex(in, &dbuf[5], tlen * 2);
          JSLI(PV, Righthex[v], &dbuf[5]);
          if (PV)
            (*PV)++;
        }
      } else {
        strncpy(obuf, buf, tlen);
        obuf[tlen] = 0;
        JSLI(PV, Left[v], obuf);
        if (PV)
          (*PV)++;

        if (tlen < len) {
          strncpy(obuf, buf + tlen, len - tlen);
          tlen = len - tlen;
          obuf[tlen] = 0;
          JSLI(PV, Right[v], obuf);
          if (PV)
            (*PV)++;
        }
      }
    }
  }
  printf("%ld lines processed\n", linecnt);
  if (dosort) {
    Tsort = calloc(linecnt, sizeof(struct Sort));
    if (!Tsort) {
      fprintf(stderr, "Can't sort, out of memory\n");
      exit(1);
    }
  }
  linecnt = 0;
  for (x = 0; x < 256; x++) {
    int cursize, y;

    if (Left[x] == NULL && Lefthex[x] == NULL) continue;
    sprintf(obuf, "%s.%03dL", argv[0], x);
    FO1 = fopen(obuf, "wb");
    if (!FO1) {
      perror(obuf);
      exit(1);
    }
    if (dorule) {
      sprintf(obuf, "%s.%03dL.rule", argv[0], x);
      FO2 = fopen(obuf, "wb");
      if (!FO2) {
        perror(obuf);
        exit(1);
      }
    }
    obuf[0] = 0;
    if (dosort) {
      cursize = 0;
      JSLF(PV, Left[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          Tsort[cursize].count = *PV;
          Tsort[cursize].line = strdup(obuf);
          if (!Tsort[cursize].line) {
            fprintf(stderr, "Out of memory allocating string\n");
            exit(1);
          }
          cursize++;
        }
        JSLN(PV, Left[x], obuf);
      }
      obuf[0] = 0;
      JSLF(PV, Lefthex[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          in = obuf;
          out = dbuf + 5;
          tlen = 0;
          while ((*out = *in)) {
            tlen++;
            out++;
            in++;
          }
          *out++ = ']';
          *out++ = '\n', *out++ = 0;
          Tsort[cursize].count = *PV;
          Tsort[cursize].line = strdup(dbuf);
          if (!Tsort[cursize].line) {
            fprintf(stderr, "Out of memory allocating string\n");
            exit(1);
          }
          cursize++;
        }
        JSLN(PV, Lefthex[x], obuf);
      }
      qsort(Tsort, cursize, sizeof(struct Sort), xsort);
      linecnt += cursize;
      for (y = 0; y < cursize; y++) {
        if (analfreq)
          fprintf(FO1, "%ld ", Tsort[y].count);
        fputs(Tsort[y].line, FO1);
        fputc('\n', FO1);
        if (dorule) {
          int llen, olen;;
          if (strncmp(Tsort[y].line, "$HEX[", 5) != 0) {
            olen = llen = strlen(Tsort[y].line);
            for (out = buf; olen; olen--) {
              *out++ = '^';
              *out++ = Tsort[y].line[olen - 1];
            }
            *out++ = '\n';
            *out = 0;
            fputs(buf, FO2);
          }
        }
        free(Tsort[y].line);
      }
    } else {
      JSLF(PV, Left[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          if (analfreq)
            fprintf(FO1, "%ld ", *PV);
          fputs(obuf, FO1);
          fputc('\n', FO1);
          if (dorule) {
            int llen, olen;;
            olen = llen = strlen(obuf);
            for (out = buf; olen; olen--) {
              *out++ = '^';
              *out++ = obuf[olen - 1];
            }
            *out++ = '\n';
            *out = 0;
            fputs(buf, FO2);
          }
          linecnt++;
        }
        JSLN(PV, Left[x], obuf);
      }
      obuf[0] = 0;
      JSLF(PV, Lefthex[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          in = obuf;
          out = dbuf + 5;
          tlen = 0;
          while ((*out = *in)) {
            tlen++;
            out++;
            in++;
          }
          *out++ = ']';
          *out++ = '\n', *out++ = 0;
          if (analfreq) fprintf(FO1, "%ld ", *PV);
          fputs(dbuf, FO1);
          linecnt++;
        }
        JSLN(PV, Lefthex[x], obuf);
      }
    }
    if (ftell(FO1))
      fclose(FO1);
    else {
      fclose(FO1);
      sprintf(obuf, "%s.%03dL", argv[0], x);
      unlink(obuf);
    }
    if (dorule) {
      if (ftell(FO2))
        fclose(FO2);
      else {
        fclose(FO2);
        sprintf(obuf, "%s.%03dL.rule", argv[0], x);
        unlink(obuf);
      }
    }

    sprintf(obuf, "%s.%03dR", argv[0], x);
    FO1 = fopen(obuf, "wb");
    if (!FO1) {
      perror(obuf);
      exit(1);
    }
    if (dorule) {
      sprintf(obuf, "%s.%03dR.rule", argv[0], x);
      FO2 = fopen(obuf, "wb");
      if (!FO2) {
        perror(obuf);
        exit(1);
      }
    }
    obuf[0] = 0;
    if (dosort) {
      cursize = 0;
      JSLF(PV, Right[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          Tsort[cursize].count = *PV;
          Tsort[cursize].line = strdup(obuf);
          if (!Tsort[cursize].line) {
            fprintf(stderr, "Out of memory allocating string\n");
            exit(1);
          }
          cursize++;
        }
        JSLN(PV, Right[x], obuf);
      }
      obuf[0] = 0;
      JSLF(PV, Righthex[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          in = obuf;
          out = dbuf + 5;
          tlen = 0;
          while ((*out = *in)) {
            tlen++;
            out++;
            in++;
          }
          *out++ = ']';
          *out++ = '\n', *out++ = 0;
          Tsort[cursize].count = *PV;
          Tsort[cursize].line = strdup(dbuf);
          if (!Tsort[cursize].line) {
            fprintf(stderr, "Out of memory allocating string\n");
            exit(1);
          }
          cursize++;
        }
        JSLN(PV, Righthex[x], obuf);
      }
      qsort(Tsort, cursize, sizeof(struct Sort), xsort);
      linecnt += cursize;
      for (y = 0; y < cursize; y++) {
        if (analfreq)
          fprintf(FO1, "%ld ", Tsort[y].count);
        fputs(Tsort[y].line, FO1);
        fputc('\n', FO1);
        if (dorule) {
          int llen, olen;;
          if (strncmp(Tsort[y].line, "$HEX[", 5) != 0) {
            llen = strlen(Tsort[y].line);
            for (out = buf, olen = 0; olen < llen; olen++) {
              *out++ = '$';
              *out++ = Tsort[y].line[olen];
            }
            *out++ = '\n';
            *out = 0;
            fputs(buf, FO2);
          }
        }
        free(Tsort[y].line);
      }
    } else {
      JSLF(PV, Right[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          if (analfreq) fprintf(FO1, "%ld ", *PV);
          fputs(obuf, FO1);
          fputc('\n', FO1);
          if (dorule) {
            int llen, olen;;
            llen = strlen(obuf);
            for (out = buf, olen = 0; olen < llen; olen++) {
              *out++ = '$';
              *out++ = obuf[olen];
            }
            *out++ = '\n';
            *out = 0;
            fputs(buf, FO2);
          }
          linecnt++;
        }
        JSLN(PV, Right[x], obuf);
      }
      obuf[0] = 0;
      JSLF(PV, Righthex[x], obuf);
      while (PV) {
        if (keepOccur == 0 || *PV > keepOccur) {
          in = obuf;
          out = dbuf + 5;
          tlen = 0;
          while ((*out = *in)) {
            tlen++;
            out++;
            in++;
          }
          *out++ = ']';
          *out++ = '\n', *out++ = 0;
          if (analfreq) fprintf(FO1, "%ld ", *PV);
          fputs(dbuf, FO1);
          linecnt++;
        }
        JSLN(PV, Righthex[x], obuf);
      }
    }
    if (ftell(FO1))
      fclose(FO1);
    else {
      fclose(FO1);
      sprintf(obuf, "%s.%03dR", argv[0], x);
      unlink(obuf);
    }
    if (dorule) {
      if (ftell(FO2))
        fclose(FO2);
      else {
        fclose(FO2);
        sprintf(obuf, "%s.%03dR.rule", argv[0], x);
        unlink(obuf);
      }
    }
    JSLFA(rc, Left[x]);
    memsum += rc;
    JSLFA(rc, Lefthex[x]);
    memsum += rc;
    JSLFA(rc, Right[x]);
    memsum += rc;
    JSLFA(rc, Righthex[x]);
    memsum += rc;
  }
  printf("%lu lines written\n", linecnt);
  printf("%lu total memory used\n", memsum);

  return (0);
}

