/*
 *  fconf.c
 *
 *  Written on 08-Jan-98 by Tobias Ernst.
 *  Released to the public domain.
 *
 *  Reads a husky project fidoconfig config file.
 */

#ifdef USE_FIDOCONFIG
#include <fidoconf/fidoconf.h>
#else
#include <stdlib.h>
#include <stdio.h>
#endif

#include <time.h>
#include <string.h>
#include "version.h"
#include "addr.h"
#include "areas.h"
#include "nedit.h"
#include "msged.h"
#include "strextra.h"
#include "memextra.h"
#include "config.h"
#include "fconf.h"
#include "version.h"
#include "group.h"
#include "environ.h"

#ifdef USE_FIDOCONFIG

/* USE_FIDOCONFIG means to use the fidoconfig libraries */

static void fc_copy_address(ADDRESS *a, s_addr *fc_a)
{  
    memset(a, 0, sizeof(ADDRESS));

    a->zone  = fc_a->zone;
    a->net   = fc_a->net;
    a->node  = fc_a->node;
    a->point = fc_a->point;

    a->fidonet = 1;

    if (fc_a->domain != NULL && *(fc_a->domain))
    {
        a->domain = xstrdup(fc_a->domain);
    }
    else
    {
        a->domain = NULL;
    }
}

static void fc_add_area(s_area *fc_area, int netmail, int local)
{
    static AREA a;

    memset(&a, 0, sizeof a);

    if (fc_area->msgbType != MSGTYPE_SDM
#ifdef USE_MSGAPI
        && fc_area->msgbType != MSGTYPE_SQUISH
        && fc_area->msgbType != MSGTYPE_JAM
#endif
        )
    {
        return;
    }

    fc_copy_address(&(a.addr), fc_area->useAka);

    a.tag = xstrdup(fc_area->areaName);
    a.description=makeareadesc(fc_area->areaName, fc_area->description);

    a.path = xstrdup(fc_area->fileName);
    
    if ((strcmp(fc_area->group, "\060") != 0) && (SW->areafilegroups))
    {
        a.group = group_gethandle(fc_area->group, 1);
    }
    else
    {
        a.group = 0;
    }

    if (netmail)
    {
        a.netmail = 1; a.priv = 1;
    }
    else if (local)
    {
        a.local = 1;
    }
    else
    {
        a.echomail = 1;
    }

    switch (fc_area->msgbType)
    {
    case MSGTYPE_SDM:
        a.msgtype = FIDO;
        break;
#ifdef USE_MSGAPI
    case MSGTYPE_SQUISH:
        a.msgtype = SQUISH;
        break;
    case MSGTYPE_JAM:
        a.msgtype = JAM;
        break;
#endif
    default:  /* should never get here */
        abort();
    }

    applyflags(&a, areafileflags);
    AddArea(&a);
}

void check_fidoconfig(char *option_string)
{
#ifndef USE_FIDOCONFIG
    printf("\r\aError! This version of "PROG" has been compiled\n"
           "without support for the FIDOCONFIG standard.\n");
    exit(-1);
#else

    s_fidoconfig *fc_config = readConfig(NULL);
    s_area       *fc_area;
    int i;
    int check_type;

    if (option_string != NULL && !stricmp(option_string, "settings"))
    {
        check_type = 1;
    }
    else if (option_string != NULL && !stricmp(option_string, "both"))
    {
        check_type = 3;
    }
    else /* Default: Load areas only */
    {
       check_type = 2;
    }
    
    if (fc_config != NULL)                    
    {
        if (check_type & 1)     /* load settings */
        {
                                /* sysop name */
            for (i = 0; i < MAXUSERS; i++)
            {
                if (user_list[i].name == NULL)
                {
                    break;
                }
            }
            if (i < MAXUSERS)
            {
                user_list[i].name = xstrdup(fc_config->sysop);
                if (i == 0)
                {
                    release(ST->username);
                    ST->username = xstrdup(user_list[i].name);
                    SW->useroffset = user_list[i].offset;
                }
            }

                                /* addresses */
            if (fc_config->addrCount)
            {
                alias = xrealloc(alias,
                                 (SW->aliascount + fc_config->addrCount) *
                                 sizeof (ADDRESS));
                
                for (i = 0; i < fc_config->addrCount; i++)
                {
                    fc_copy_address(alias + SW->aliascount + i,
                                    fc_config->addr + i);
                }
                SW->aliascount += fc_config->addrCount;
            }

                                /* echotoss log */
            if (fc_config->echotosslog !=NULL)
            {
                release(ST->echotoss);
                ST->echotoss =
                    pathcvt(xstrdup(fc_config->echotosslog));
            }

                                /* area to place file requests in */
	    if (fc_config->netMailAreaCount > 0)
	    {
	        release(ST->freqarea);
		ST->freqarea = xstrdup(fc_config->netMailAreas[0].areaName);
	    }

                                /* fido user list */
            if (fc_config->nodelistDir != NULL &&
                fc_config->fidoUserList != NULL)
            {
                release(ST->userlist);
                ST->userlist = xmalloc(strlen(fc_config->nodelistDir)+
                                       strlen(fc_config->fidoUserList) + 1);
                strcpy(ST->userlist, fc_config->nodelistDir);
                strcat(ST->userlist, fc_config->fidoUserList);
            }
        }
        if (check_type & 2)     /* load areas */
        {
                                /* netmail, dupe, bad */

            fc_add_area(&(fc_config->dupeArea), 0, 1);
            fc_add_area(&(fc_config->badArea), 0, 1);
            
                                /* netmail areas */
            for (i=0; i<fc_config->netMailAreaCount; i++)
            {
                fc_area = &(fc_config->netMailAreas[i]);
                if (fc_area->msgbType != MSGTYPE_PASSTHROUGH)
                {
                    fc_add_area(fc_area, 1, 0);
                }
            }
      
                                /* local areas */
            for (i=0; i<fc_config->localAreaCount; i++)
            {
                fc_area = &(fc_config->localAreas[i]);
                if (fc_area->msgbType != MSGTYPE_PASSTHROUGH)
                {
                    fc_add_area(fc_area, 0, 1);
                }
            }
      
                                /* echomail areas */
            for (i=0; i<fc_config->echoAreaCount; i++)
            {
                fc_area = &(fc_config->echoAreas[i]);
                if (fc_area->msgbType != MSGTYPE_PASSTHROUGH)
                {
                    fc_add_area(fc_area, 0, 0);
                }
            }
        }

        disposeConfig(fc_config);
    }
    else
    {
        printf ("\r\aError! Cannot open fidoconfig!\n");
        exit(-1);
    }
#endif
}
#else

/* Our own version of parsing routines for fidoconfig - this one cannot get the
   full info out of fidoconfig, but only the area info. */

static void read_fidoconfig_file (char *filename);
static ADDRESS fc_default_address;
static int fc_default_address_set;

void check_fidoconfig(char *option_string)
{
    char *filename;
    
    if (option_string != NULL)
    {
        printf ("\r\nOnly loading area info from fidoconfig, because this
binary is not linked\nagainst fidoconfig libaries.\n");
    }

    filename = getenv("FIDOCONFIG");

    if (filename == NULL)
    {
        printf ("\r\nError: You must set the FIDOCONFIG environment variable!\n");
        return;
    }

    fc_default_address_set = 0;
    memset(&fc_default_address, 0, sizeof(ADDRESS));
    release (fc_default_address.domain);

    read_fidoconfig_file(filename);
}

static void parse_fc_address(void)
{
    char *token = strtok(NULL, " \t");

    if (token == NULL)
    {
        printf ("\r\nFidoconfig address statement missing argument.\n");
        return;
    }

    if (!fc_default_address_set)
    {
        ADDRESS tmp;
        
        fc_default_address_set = 1;
        parsenode(token);
        memcpy(&fc_default_address, &tmp, sizeof(ADDRESS));
    }
}

static void parse_fc_include(void)
{
    char *token = strtok(NULL, " \t");
    char *fn;

    if (token != NULL)
    {
        fn = pathcvt(token);
        read_fidoconfig_file(fn);
        xfree(fn);
    }
    else
    {
        printf ("\r\nFidoconfig include statement missing argument.\n");
    }
}

static char *fc_get_description(char *firsttoken)
{
    
    static char desc[257];
    char *token = firsttoken;
    int len = 0;

    *desc = '\0';
    while(token != NULL)
    {
        if (len + 1 >= sizeof(desc))
        {
            return NULL;
        }
        if (*desc)
        {
            desc[len++] = ' '; desc[len] = '\0';
        }
        else
        {
            if (*token=='"')
            {
                token++;
            }
        }
        if (len + strlen(token) >= sizeof(desc))
        {
            return NULL;
        }
        strcpy(desc + len, token);
        len += strlen(token);

        if (!len || desc[len - 1]!='"')
        {
            token=strtok(NULL, " \t");
        }
        else
        {
            token = NULL;
        }
    }
    
    return desc;
}


    
static void parse_fc_area(int type)
{
    static AREA a;
    char *area_description = NULL;
    char *token, *next;
    int option;

    memset(&a, 0, sizeof(AREA));

    token = strtok(NULL, " \t");
    if (token == NULL)
    {
        printf ("\r\nFidoconfig *area statement missing argument.\n");
        return;
    }

    a.tag = xstrdup(token);

    token = strtok(NULL, " \t");
    if (token == NULL)
    {
        xfree(a.tag);
        printf ("\r\nFidoconfig *area statement missing argument.\n");
        return;
    }
    else if (stricmp(token, "passthrough"))
    {
        xfree(a.tag);
        return;
    }
    
    a.path = pathcvt(xstrdup(token));

    copy_addr(&(a.addr), &(fc_default_address));

    switch(type)
    {
    case 1:
        a.netmail = 1;
        break;
    case 2:
        a.local = 1;
        break;
    case 3:
        a.echomail = 1;
        break;
    }

    a.msgtype = FIDO;

    token = strtok(NULL, " \t");
    while (token != NULL)
    {
        if (token[0] = '-')
        {
            option = 0;
            if (!stricmp(token + 1, "b"))
            {
                option = 1;
            }
            else if (!stricmp(token + 1, "a"))
            {
                option = 2;
            }
            else if (!stricmp(token + 1, "g"))
            {
                option = 3;
            }
            else if (!stricmp(token + 1, "d"))
            {
                option = 4;
            }

            if (option)
            {
                token = strtok(NULL, " \t");
                if (token != NULL)
                {
                    switch(option)
                    {
                    case 1:
                        if (!stricmp(token, "msg"))
                        {
                            a.msgtype = FIDO;
                        }
#ifdef USE_MSGAPI
                        else if (!stricmp(token, "squish"))
                        {
                            a.msgtype = SQUISH;
                        }
                        else if (!stricmp(token, "jam"))
                        {
                            a.msgtype = JAM;
                        }
#endif
                        else
                        {
                            release(a.tag);
                            release(a.path);
                            release(a.addr.domain);
                            return;
                        }
                        break;

                    case 2:
                        release(a.addr.domain);
                        a.addr = parsenode(token);
                        break;

                    case 3:
                        a.group = group_gethandle(token, 1);
                        break;

                    case 4:
                        area_description = fc_get_description(token);
                        break;
                    }
                }
            }
        }
        token = strtok(NULL, " \t");
    }

    a.description=makeareadesc(a.tag, area_description);
    applyflags(&a, areafileflags);
    AddArea(&a);
}

static void parse_fc_line(char *line)
{
    char *token;

    if (!(*line))
    {
        return;
    }
    
    token = strtok(line, " \t");

    if (token == NULL)
    {
        return;
    }

    if ((!stricmp(token, "netmailarea")) ||
        (!stricmp(token, "netarea")))
    {
        parse_fc_area(1); /* netmail folders */
    }
    else if ((!stricmp(token, "dupearea")) ||
             (!stricmp(token, "badarea")) ||
             (!stricmp(token, "localarea")))
    {
        parse_fc_area(2); /* local folders */
    }
    else if ((!stricmp(token, "echoarea")))
    {
        parse_fc_area(3); /* echomail folders */
    }
    else if ((!stricmp(token, "include")))
    {
        parse_fc_include();
    }
    else if ((!stricmp(token, "address")))
    {
        parse_fc_address();
    }
    else
    {
        /* unknown token */
        ;
    }

    return;
}
    
static void read_fidoconfig_file (char *filename)
{
    FILE *f = fopen(filename, "r");
    static char line[2048]; /* uh, oh, care for reentrance! */
    char *start;
    char *expanded_line;
    size_t l;
    int c;

    if (f == NULL)
    {
        printf ("\r\nError: Can't open %s while parsing fidoconfig file.\n",
                filename);
        return;
    }

    while(fgets(line, sizeof(line), f) != NULL)
    {
        l = strlen(line);
        
        /* handle trailing \n */
        if (l)
        {
            if (line[l-1] != '\n')
            {
                /* eat up superfluous characters in extra long line */
                do
                {
                    c = fgetc(f);
                } while (c != '\n' && c != EOF);
            }
            else
            {
                line[l-1] = '\0';
            }
        }

        /* trim spaces at beginning */
        for (start = line;
             ((*start == ' ') || (*start == '\t') || (*start == (char)0xFE));
             start++, l--);
        memmove(line, start, l+1);

        /* trim trailing spaces */
        while (l && line[l - 1] == ' ' || line[l - 1] == '\t')
        {
            line[l - 1] = '\0';
            l--;
        }

        /* strip comments */
        if (*line='#')
        {
            *line='\0';
        }
        else
        {
            start = strchr(line,'#');
            if ((start != NULL) && (*(start - 1)==' ' || *(start - 1) == '\t'))
            {
                *(start - 1) = '\0';
            }
        }
        
        expanded_line = env_expand(line); /* expand %ENVIRONMENT% variables */
        parse_fc_line(expanded_line);
        xfree(expanded_line);
    }

    fclose(f);
}
#endif
