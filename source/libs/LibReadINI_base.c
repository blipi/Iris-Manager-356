/*
 *
 * LibReadINI by D_Skywalk
 *
 * This code is Based on mcfg library @ ma_muquit@fccc.edu   Apr-09-1998  
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 * 
 * 
**/

/*
**
**
**  Development History:
**      who                  when           why
**      dskywalk@gmail.com   Jul-07-2006    adds for the new user interface & optimizations
**      ma_muquit@fccc.edu   Apr-09-1998    first cut
**
**  Comments:
**      some parts are taken from samba source.
*/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <ctype.h>

#include "LibReadINI-global.h"

#define LIBNAME "LibReadINI - libbase.c : "

#define BUFR_INC    2048

static int    Section        (FILE *fp);
static int    Parameter      (FILE *fp,int style,
                                        int (*pfunc)(const char *,const char *), int c);

static int    Continuation   (char *line,int pos);
static int    eatWhitespace  (FILE *fp);

static void regSection(char * sec_name);
static void checkSection();
static char *getCurrentSection();


/* Memory functions */
static char   *loc_realloc   (char *p,int size);

/* Internal Variables */
static int  isSection=  0;
static char *bufr=      (char *) NULL;
static int  bsize=      0;
static char *cursec=    (char *) NULL;
static char *usersec=   (char *) NULL;
static FILE *cfgFILE=   (FILE *) NULL;
static int  cfgSTYLE=    0;




/*
**  Parameter()
**  scan a parameter name (or name and value pair) and pass the value (or
**  values) to function pfunc().
**
**  Parameters:
**      fp      - open FILE pointer
**      pfunc   - a pointer to the function that will be called to process
**                the parameter, once it has been scanned
**      c       - the first character of the parameter name, which would
**                have been read by Parse(). unlike comment line or a section
**                header, there's no lead-in character can be discarded.
**
**      style   - the style of the config file. it can be MS_STYLE, that is
**                parameter must follows by = and the value. If it is
**                NOT_MS_STYLE, then parameter does not follows by = or the
**                value.
**                  Example of MS_STYLE config file:
**                      [section]
**                          version = 2.4
**                      [foo]
**                          bar=hello
**                          foobar=world
**
**                  Example of NOT_MS_STYLE config file:
**                      [section]
**                           2.4
**                      [foo]
**                          hello
**                          world
**
**  Return Values:
**      0       on success
**      -1      on failure
**
*/

static int Parameter (fp,style,pfunc,c)
FILE
    *fp;
int
    style;
int
    (*pfunc)();
int
    c;
{
    int
        i=0;      /* position withing bufr */
    int
        end=0;    /* bufr[end] is current end-of-string */
    int
        vstart=0; /* starting position of the parameter */

    char
        *func= LIBNAME "Parameter() -";

    if (style == MS_STYLE)
    {
        /*
        ** loop until we found the start of the value 
        */
        while (vstart == 0) 
        {
            /*
            ** ensure there's space for next char 
            */
            if (i > (bsize-2))  
            {
                bsize += BUFR_INC;
                bufr=loc_realloc(bufr,bsize);
                if (bufr == NULL)
                {
                    (void) fprintf(stderr,"%s malloc failed\n",func);
                    return (0);
                }
            }

            switch (c)
            {
                case '=':
                {
                    if (end == 0)
                    {
                        (void) fprintf(stderr,"%s invalid parameter name\n",
                                       func);
                        return (0);
                    }
                    bufr[end++]='\0';
                    i=end;
                    vstart=end;
                    bufr[i]='\0';
                    break;
                }

                case '\n':
                {
                    i=Continuation(bufr,i);
                    if (i < 0)
                    {
                        bufr[end]='\0';
                        (void) fprintf(stderr,
                               "%s ignoring badly formed line in config file\n",
                               func);
                        return(0);
                    }

                    end=((i > 0) && (bufr[i-1] == ' ')) ? (i-1) : (i);
                    c=getc(fp);
                    break;
                }

                case '\0':
                case EOF:
                {
                    bufr[i]='\0';
                    (void) fprintf(stderr,
                                   "%s unexpected end-of-file at %s: func\n",
                                   func,bufr);
                    return (0);
                    break;
                }

                default:
                {
                    if (isspace(c))
                    {
                        bufr[end]=' ';
                        i=end+1;
                        c=eatWhitespace(fp);
                    }
                    else
                    {
                        bufr[i++]=c;
                        end=i;
                        c=getc(fp);
                    }
                    break;
                }
            }
        }

        /*
        ** now parse the value
        */
        c=eatWhitespace(fp);
    }   /* MS_STYLE */

    while ((c != EOF) && (c > 0))
    {
        if (i > (bsize-2))
        {
            bsize += BUFR_INC;
            bufr=loc_realloc(bufr,bsize);
            if (bufr == NULL)
            {
                (void) fprintf(stderr,"%s malloc failed\n",func);
                return(0);
            }
        }

        switch(c)
        {
            case '\r':
            {
                c=getc(fp);
                break;
            }

            case '\n':
            {
                i=Continuation(bufr,i);
                if (i < 0)
                    c=0;
                else
                {
                    for(end=i; 
                        (end >= 0) 
                            && isspace((int)bufr[end]); 
                                end--)
                        ;
                    c=getc(fp);
                }
                break;
            }

            default:
            {
                bufr[i++]=c;
                if (!isspace(c))
                    end=i;
                c=getc(fp);
                break;
            }

        }
    }

    bufr[end]='\0';
    return (pfunc(bufr,&bufr[vstart]));
}


/*
 * Init lib and allocate buffer memory
 */
static int initLib()
{

    char
        *func= LIBNAME "iniLib() -";

    /* Allocate buffer for get the file chars */
    if(bufr==NULL)
    {
            bsize=BUFR_INC;
            bufr=(char *)  memalign(128, bsize); //malloc(bsize);
            memset((void *) bufr, 0, bsize);
            if (bufr == NULL)
            {
                    (void) fprintf(stderr,"%s malloc failed\n",func);
                    return (0);
            }
    
    }else
            (void) fprintf(stderr,"Warning - %s buffer already allocated??\n",func);
            
        setUserSection(""); /* NULL USER SECTION, PREVENTS SEG-FAULTS */

    return (1);
}

void closeConfFile()
{
        (void) fclose(cfgFILE);
        free(bufr);
        bufr=NULL;
        bsize=0;

}

/*
**  openConfFile()
**  open the configuration file
**
**  Parameters:
**      filename    - the pathname of the configuration fle.
**
**  Return Values:
**      1 if file is open & memory is allocated correctly, or
**      0 if the file could not be opened or not memory for bufr.
**
**
*/

int openConfFile(filename, style)
const char
    *filename;
const int
    style;
{
    FILE
        *fp=(FILE *) NULL;

    char
        *func= LIBNAME "openConfFile() -";

        cfgSTYLE=style;

    if ((filename == NULL) || (*filename == '\0'))
    {
        (void) fprintf(stderr,"%s no config file specified.\n",func);
        return (0);
    }

    fp=fopen(filename,"r");
    if (fp == (FILE * ) NULL)
    {
        (void) fprintf(stderr,"%s unable to open config file %s\n",
                       func,filename);
        return (0);
    }


    cfgFILE = fp;

    if(initLib())
        return (1);
    else
    return (0);

}



/*
** scan to the end of a comment
*/
static int eatComment(fp)
FILE
    *fp;
{
    int
        c;
    for(c=getc(fp); ('\n'!=c) && (EOF!=c) && (c>0);c=getc(fp))
            ;

    return(c);
}

static int eatWhitespace(fp)
FILE
    *fp;
{
    int
        c;

    for(c =getc(fp);isspace(c) && ('\n' != c); c=getc(fp))
        ;
    return (c);
}


static int Continuation(line,pos)
char
    *line;
int
    pos;
{
    pos--;
    while ((pos >= 0) && isspace((int)line[pos]))
        pos--;

    return (((pos >= 0) && (line[pos] == '\\')) ? pos : -1);
}


/*
**  
**
**  Parameters:
**      fp      - open FILE pointer
**      style   - the style of the config file. it can be MS_STYLE, that is
**                parameter must follows by = and the value. If it is
**                NOT_MS_STYLE, then parameter does not follows by = or the
**                value.
**                  Example of MS_STYLE config file:
**                      [section]
**                          version = 2.4
**                      [foo]
**                          bar=hello
**                          foobar=world
**
**                  Example of NOT_MS_STYLE config file:
**                      [section]
**                           2.4
**                      [foo]
**                          hello
**                          world
**      pfunc   - function to be called when a parameter is scanned.
**
**  Return Values:
**      0  on success
**      -1 on failure
**
**  Limitations and Comments:
**      from samba source code
**
**
*/

/*
** TODO: Stops when user section is read and reads another section...
*/
int parseConfFile(pfunc)
int
    (*pfunc)();
{
    int c;

    int style = cfgSTYLE;
    FILE *fp = cfgFILE;
    char
        *func= LIBNAME "parseConfFile() -";

    if (bufr == NULL){
        (void) fprintf(stderr,"Critical Error! in %s - buffer is not allocated? \n",func);
    return (0);
    }


    c=eatWhitespace(fp);

    while ((c != EOF) && (c > 0))
    {

        switch(c)
        {
            case '\n':                  /* blank line */
            {
                c=eatWhitespace(fp);
                break;
            }

            case ';':                   /* comment line */
            case '#':
            {
                c=eatComment(fp);
                break;
            }

            case '[':                   /* section header */
            {
                if (Section(fp) < 0)
                {
                    return (0);
                }
                c=eatWhitespace(fp);
                break;
            }

            case '\\':                  /* bogus backslash */
            {
                c=eatWhitespace(fp);
                break;
            }

            default:                    /* parameter line */
            {
        if (isSection){
                    if (!Parameter(fp,style,pfunc,c)) /* send parameter */
                    return (0);
        }else
            eatComment(fp); /* next parameter */
        
                c=eatWhitespace(fp);
            }
        }
    }

    return (1);
}

/*
**  Section()
**  scan a section name and regs them
**
**  Parameters:
**      fp      - open FILE pointer
**
**  Return Values:
**      0   if the section name was read
**      -1  if a lexical error was encountered.
**
**  Limitations and Comments:
**      from samba source code
**
**
*/

static int Section(fp)
FILE
    *fp;
{
    int
        c,
        i,
        end;

    char
        *func= LIBNAME "Section() -";

    i=0;
    end=0;

    c=eatWhitespace(fp);    /* 
                            ** we've already got the '['. scan past initial
                            ** white space
                            */

    while ((c != EOF) && (c > 0))
    {
        if (i > (bsize-2))
        {
            bsize += BUFR_INC;
            bufr=loc_realloc(bufr,bsize);
            if (bufr == NULL)
            {
                (void) fprintf(stderr,"%s malloc failed\n",func);
                return(0);
            }
        }

        switch (c)
        {
            case ']':           /* found the closing bracked */
            {
                bufr[end]='\0';
                if (end == 0)
                {
                    (void) fprintf(stderr,"%s empty section name\n",func);
                    return (0);
                }
                
        regSection(bufr); /* save current section */
        checkSection(); /* check current section vs user section */

                (void) eatComment(fp);
                return (1);
                break;
            }

            case '\n':
            {
                i=Continuation(bufr,i);
                if (i < 0)
                {
                    bufr[end]='\0';
                    (void) fprintf(stderr,"%s badly formed line in cfg file\n",
                                   func);

                    return (0);
                }
                end=((i > 0) && (bufr[i-1] == ' ')) ? (i-1): (i);
                c=getc(fp);
                break;
            }
            default:
            {
                if (isspace(c))
                {
                    bufr[end]=' ';
                    i=end + 1;
                    c=eatWhitespace(fp);
                }
                else
                {
                    bufr[i++]=c;
                    end=i;
                    c=getc(fp);
                }
                break;
            }
        }
    }
                            
    return (1);
}

/*
** expand a pointer to be a particular size
*/
static char *loc_realloc(p,size)
char
    *p;
int
    size;
{
    char
        *ret=NULL;

    if (size == 0)
    {
        if (p)
        {
            (void) free(p);
            (void) fprintf(stderr,"loc_realloc() asked for 0 bytes\n");
            return (NULL);
        }
    }

    if (!p)
        ret=(char *) malloc(size);
    else
        ret=(char *) realloc(p,size);

    if (!ret)
        (void) fprintf(stderr,"malloc problem, failed to expand to %d bytes\n",
                       size);
    return (ret);
}

 

/*
** New Section control functions
*/

static void regSection(char * sec_name)
{

    if (cursec != (char *) NULL)
    {
    #ifdef DEBUG
            fprintf(stderr,"regSection - Freeing usersec\n");
    #endif
        (void) free(cursec);
        cursec=(char *) NULL;
    }
    if (sec_name)
        cursec=strdup(sec_name);
}


static void checkSection()
{
    if(strcmp(getCurrentSection(),getUserSection()) == 0)
        isSection = 1;
    else
        isSection = 0;

}

void setUserSection(const char * sec_name){

    if (usersec != (char *) NULL)
    {
    #ifdef DEBUG
            fprintf(stderr,"setUserSection - Freeing usersec\n");
    #endif
        (void) free(usersec);
        usersec=(char *) NULL;
    }
    if (sec_name)
        usersec=strdup(sec_name);
}

static char *getCurrentSection()
{
    return(cursec);
}

char *getUserSection()
{
    return(usersec);
}


/*
TODO: ¿No Rewind?
 - Using a internal index sections...
fseek(fp, 100, SEEK_SET); // seek to the 100th byte of the file
fseek(fp, -30, SEEK_CUR); // seek backward 30 bytes from the current pos
fseek(fp, -10, SEEK_END); // seek to the 10th byte before the end of file
*/

void rewindConfFile()
{
    #ifdef DEBUG
        (void) fprintf(stderr, LIBNAME "Rewinded CFG\n");
    #endif
    fseek(cfgFILE, 0, SEEK_SET);   /* seek to the beginning of the file */
}
