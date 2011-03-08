/*
 * LibReadINI by D_Skywalk
 *
 * Copyright (c) 2006 David Colmenero Aka D_Skywalk
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
 *   Version 0.8e (Stable)
 * 
 * 0.8 - Initial Public version
 *
**/

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdlib.h>

#include "LibReadINI-global.h"
#include "linux_ansi-fix.h"

#define LIBNAME "LibReadINI - LibReadINI.c : "

/* User interface functions */
static void   initValues();

/* Internal Structs */
typedef struct cValue
{
    char * name;
    char * value;
    struct cValue * next;
}cValue;

typedef struct cSection
{
    struct cValue * value;
}cSection;


/* Global section */
cSection glSection = {NULL};


/*
** Open config file and allocate base memory
*/
int cfgOpen(filename,style)
const char
    *filename;
const int
    style;
{
    char *func= LIBNAME "cfgOpen() -";


    if(!openConfFile(filename,style)){
                (void) fprintf(stderr,"%s open failed\n",func);
            return (0);
    }
    return (1);

}

void cfgFree(){
        initValues(); /* free glSection */
}

/*
** Close config file and Free all
*/
void cfgClose(){
    closeConfFile();
}


/*
** Save current parameter in global section.
*/

static int saveValue(const char * param_name, const char * param_value)
{
    cValue *temp = NULL, **new = NULL;
    char *func= LIBNAME "saveValue() -";

    if(param_name == NULL || param_value == NULL)
        return (0);

    temp = (cValue*) memalign(128, sizeof(cValue)); //malloc(sizeof(cValue));
       memset((void *) temp, 0, sizeof(cValue));

    if(!temp){
        (void) fprintf(stderr,"%s malloc failed\n",func);
        return (0);
    }
    
    temp->name = strdup((const char *) param_name);
    temp->value = strdup((const char *) param_value); 
    temp->next = NULL;

    new = (cValue **) &glSection.value;

    for(;(*new) != NULL; new = &(*new)->next);

    *new = temp;

    return (1);

}

/*
** Init global variable
**
*** free glSection and init them
*/

static void initValues()
{
    cValue **temp = (cValue **) &glSection.value, *lib = NULL;
    while ( (*temp) != NULL ){
        lib=*temp;
        free(lib->name);
        free(lib->value);
        temp=&(*temp)->next;
        free(lib);
    }
    glSection.value = NULL;
        
}

/*
** Read any value and returns his pointer
** returns NULL if not is founded
*/

cValue * readValue(const char * nombre) {

    cValue * result = glSection.value;
    int noencontrado = -1;


    if(result != NULL){
        if(strcasecmp(result->name,nombre)==0)     /* check first parameter */
            return (result);
    }else
        return ((cValue*) NULL);  /* prevents seg fault... */

        

        do{
         result = result->next;

        if(result)
            noencontrado = strcasecmp(result->name,nombre);
            

    }while((result != NULL)&&(noencontrado));


    return (result);

}

/*
** Internal saving section function
**
*** parse all file & save current section in the global valiable
*/
static int saveCurrentSection()
{
        #ifdef DEBUG   
    char *func= LIBNAME "saveCurrentSection() -";
        #endif
    
    int result = -1;

    
    /* INIT VALUE SECTION :D */
    initValues();
    
        result=parseConfFile(saveValue);

        /* rewind cfg :? */
        rewindConfFile();

        #ifdef DEBUG   
            (void) fprintf(stderr,"%s parsed file successful with result: %i \n",func, result);
        #endif

    if((result) && (glSection.value == NULL)) /* not found */
        return 0;
    
    return (result);

}

/*
** User section selects 
**
*** Selects a section & save it in the global variable
*/

int cfgSelectSection(const char * sec_name){
    if(strcmp(getUserSection(),sec_name)!= 0){
    setUserSection(sec_name);
    return (saveCurrentSection());
    }else{
    return 1; /* user selects our current section */
    }
    
}


/*
** Read user functions
*/

int cfgReadInt(const char * VariableName,const int Default){
    cValue * entero = NULL;

    entero = readValue(VariableName);

    if(entero)
        return atoi(entero->value);
    else
        return Default;

}

char * cfgReadText(const char * VariableName,const char * Default){
    cValue * cadena = NULL;

    cadena = readValue(VariableName);

    if(cadena)
        return cadena->value;
    else
        return ((char*) Default);
}

bool cfgReadBool(const char * VariableName, const bool Default){
    cValue * temp = NULL;

    temp = readValue(VariableName);

    if(temp){
        if((strcasecmp("true", temp->value)==0)
         ||(strcasecmp("yes", temp->value)==0)
         ||(strcasecmp("1", temp->value)==0)
        )
            return TRUE;
        else
            return FALSE;
    } return Default;

}





