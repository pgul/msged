/*
 *  CONFIG.H
 *
 *  Written on 30-Jul-90 by jim nutt.  Changes on 10-Jul-94 by John Dennis.
 *  Released to the public domain.
 *
 *  Finds & reads configuration files, initializes everything.
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

struct colorverb
{
    char *name;
    int color;
};

char *strip_geese_feet(char *s);
char *striplwhite(char *str);
char *striptwhite(char *str);
char *skip_to_blank(char *str);
void kill_trail_slash(char *str);
void opening(char *cfgfile, char *areafile);
void parse_tokens(char *str, char *tokens[], int num);

#endif
