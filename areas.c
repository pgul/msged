/*
 *  AREAS.C
 *
 *  Written by jim nutt, John Dennis and released into the public domain
 *  by John Dennis in 1994.
 *
 *  This file contains the routines to select one of the areas specified
 *  in the config files.  It pops up a list with all the areas and lets
 *  the user choose.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "addr.h"
#include "nedit.h"
#include "msged.h"
#include "winsys.h"
#include "menu.h"
#include "main.h"
#include "memextra.h"
#include "specch.h"
#include "keys.h"
#include "unused.h"
#include "help.h"
#include "version.h"
#include "strextra.h"
#include "areas.h"

char **alist = NULL;
char **alist2 = NULL;

static int AreaBox(char **Itms, int y1, int y2, int len, int def, WND * hPrev, WND * hWnd, int Sel, int Norm, int indent);

void BuildList(char ***lst)
{
    int i;
    AREA *a;
    char line[181];
    unsigned long unread, last;

    *lst = xcalloc(SW->areas + 2, sizeof(char *));

    for (i = 0; i < SW->areas; i++)
    {
        a = arealist + i;

        memset(line, ' ', sizeof line);
        if (a->scanned)
        {
            last = a->lastread;
            if (last > a->messages)
            {
                last = a->messages;
            }
            unread = a->messages - a->lastread;
            if (unread > a->messages)
            {
                unread = a->messages;
            }
            sprintf(line, "%c%-*.*s", unread ? SC14 : ' ',
              maxx - 25, maxx - 25, a->description);
            line[strlen(line)] = ' ';
            sprintf(line + maxx - 23, "%6lu%6lu%6lu",
              a->messages, unread, last);
        }
        else
        {
            sprintf(line, " %-*.*s", maxx - 25, maxx - 25, a->description);
            line[strlen(line)] = ' ';
            sprintf(line + maxx - 19, " -     -     -");
        }

        (*lst)[i] = xstrdup(line);
    }
    (*lst)[i] = NULL;
}

static void SelShowItem(char *text, int y, int len, int Attr, int indent)
{
    char line[256];

    memset(line, ' ', sizeof(line));

    if (strlen(text) + indent > sizeof(line) - 1)
    {
        strcpy(line,"<--- internal buffer overflow --->");
    }

    strcpy(line + indent, text);
    line[sizeof(line)-1] = 0;
    WndPutsn(1, y, len, Attr, line);
}

static void SelShowPage(char **text, int top, int bot, int len, int pos, int Attr, int indent)
{
    int i, y;

    y = top;
    for (i = pos; text[i] != NULL; i++)
    {
        if (y > bot)
        {
            break;
        }
        SelShowItem(text[i], y++, len, Attr, indent);
    }
    if (y <= bot)
    {
        while (y <= bot)
        {
            SelShowItem(" ", y++, len, Attr, indent);
        }
    }
}

static void CalcDef(int max, int cur, int *top, int miny, int maxy, int *y)
{
    int dif;
    unused(cur);
    dif = (maxy - 1) - miny;
    if ((max - 1) - *top < dif && max > dif)
    {
        *y = maxy;
        *top = (max - 1) - dif;
    }
}

static int AreaBox(char **Itms, int y1, int y2, int len, int def, WND * hPrev, WND * hWnd, int Sel, int Norm, int indent)
{
    EVT e;
    char find[30];
    int itemCnt, Stuff, done, curY, Msg, currItem, Top, page, i;

    itemCnt = 0;
    Stuff = 0;
    for (i = 0; Itms[i] != NULL; i++)
    {
        itemCnt++;
    }

    currItem = def;
    curY = y1;
    page = y2 - y1;
    Top = currItem;

    if (currItem + y1 < y1)
    {
        curY = y1 + currItem;
        Top = 0;
    }
    else
    {
        if ((itemCnt - currItem) <= (y2 - y1))
        {
            Top -= ((y2 - y1 + 1) - (itemCnt - Top));
            curY = y1 + (def - Top);
            if (Top < 0)
            {
                Top = 0;
                curY--;
            }
        }
    }
    done = 0;

    SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
    SelShowItem(Itms[currItem], curY, len, Sel, indent);

    TTClearQue();               /* clear input queue */

    memset(find, '\0', sizeof find);
    while (!done)
    {
        if (!*find)
        {
            WndCurr(hPrev);
            WndClear(16, 0, maxx - 36, 0, cm[MN_NTXT]);
            WndWriteStr(0, 0, cm[MN_NTXT], ">>Pick New Area:");
            WndCurr(hWnd);
        }

        if (!Stuff)
        {
            Msg = MnuGetMsg(&e, hWnd->wid);
        }
        else
        {
            e.msgtype = WND_WM_CHAR;
            Msg = Stuff;
            Stuff = 0;
        }

        switch (e.msgtype)
        {
        case WND_WM_MOUSE:
            switch (Msg)
            {
            case RMOU_CLCK:
            case MOU_RBTUP:
                return -1;

            case LMOU_RPT:
            case MOU_LBTDN:
            case LMOU_CLCK:
                {
                    int x, y;
                    WndGetRel(e.x, e.y, &x, &y);
                    if (y >= y1 && y <= y2)  /* in window */
                    {
                        Stuff = 0;
                        if (x >= 0 && x < len)
                        {
                            if (y == curY)
                            {
                                if (Msg == LMOU_CLCK || Msg == MOU_LBTUP)
                                {
                                    return currItem;
                                }
                                else
                                {
                                    continue;
                                }
                            }

                            SelShowItem(Itms[currItem], curY, len, Norm, indent);

                            if (y > curY)
                            {
                                currItem += y - curY;
                            }
                            else
                            {
                                currItem -= curY - y;
                            }

                            curY = y;
                            SelShowItem(Itms[currItem], curY, len, Sel, indent);

                            if (Msg == LMOU_CLCK || Msg == MOU_LBTUP)
                            {
                                return currItem;
                            }
                        }
                    }
                    else
                    {
                        if (Msg != LMOU_CLCK)
                        {
                            if (y < y1)
                            {
                                Stuff = Key_Up;
                            }
                            else
                            {
                                Stuff = Key_Dwn;
                            }
                        }
                    }
                }
                memset(find, '\0', sizeof find);
                break;

            default:
                break;
            }
            break;

        case WND_WM_CHAR:
            switch (Msg)
            {
            case Key_Home:
                if (!currItem)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                currItem = 0;
                Top = 0;
                curY = y1;
                SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_A_H:
            case Key_F1:
                if (ST->helpfile != NULL)
                {
                    DoHelp(3);
                }
                break;

            case Key_End:
                if (currItem == itemCnt - 1)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                currItem = itemCnt - 1;
                while (currItem && currItem >= (itemCnt - page))
                {
                    currItem--;
                }
                Top = currItem;
                currItem = itemCnt - 1;
                curY = currItem - Top + y1;
                CalcDef(itemCnt, currItem, &Top, y1, y2, &curY);
                SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_Dwn:
                if (currItem == itemCnt - 1)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                currItem++;
                if (curY == y2)
                {
                    WndScroll(1, y1, len - 1, y2, 1);
                    Top++;
                }
                else
                {
                    curY++;
                }
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_Up:
                if (!currItem)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                currItem--;
                if (curY == y1)
                {
                    WndScroll(1, y1, len - 1, y2, 0);
                    if (Top)
                    {
                        Top--;
                    }
                }
                else
                {
                    curY--;
                }
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_PgUp:
                if (!currItem)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                if ((currItem -= page) < 0)
                {
                    currItem = 0;
                }
                Top = currItem;
                curY = y1;
                SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_PgDn:
                if (currItem == itemCnt - 1)
                {
                    break;
                }
                SelShowItem(Itms[currItem], curY, len, Norm, indent);
                Top = currItem;

                if ((currItem += page) > itemCnt - 1)
                {
                    currItem = itemCnt - 1;
                    if (currItem > page)
                    {
                        Top = currItem - page;
                    }
                    else
                    {
                        Top = 0;
                    }
                }

                curY = currItem - Top + y1;
                CalcDef(itemCnt, currItem, &Top, y1, y2, &curY);
                SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            case Key_Ent:
            case Key_Rgt:
                {
                    size_t i;
                    i = (size_t) (strchr(Itms[currItem] + 1, ' ') - 1 - Itms[currItem]);
                    if (i > 28)
                    {
                        i = 28;
                    }
                    strncpy(find, Itms[currItem] + 1, i);
                    *(find + i) = '\0';
                    strupr(find);
                    WndCurr(hPrev);
                    WndWriteStr(17, 0, cm[MN_NTXT], find);
                    WndCurr(hWnd);
                    return currItem;
                }

            case Key_Esc:
                if (SW->confirmations)
                {
                    if (confirm("Exit " PROG "?"))
                    {
                        return -1;
                    }
                }
                else
                {
                    return -1;
                }
                memset(find, '\0', sizeof find);
                break;

            case Key_A_X:
                return -1;

            case '*':
            case '#':
            case Key_A_S:
            case Key_A_T:
                arealist_area_scan(Msg == '*' || Msg == Key_A_T);

                for (i = 0; i < SW->areas; i++)
                {
                    xfree(alist[i]);
                }
                xfree(alist);
                BuildList(&alist);
                Itms = alist;
                SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                SelShowItem(Itms[currItem], curY, len, Sel, indent);
                memset(find, '\0', sizeof find);
                break;

            default:
                if (Msg > 32 && Msg < 127)
                {
                    if (strlen(find) >= 28)
                    {
                        break;
                    }
                    *(find + strlen(find)) = (char)toupper((char)Msg);
                    WndCurr(hPrev);
                    WndWriteStr(17, 0, cm[MN_NTXT], find);
                    WndCurr(hWnd);
                    i = currItem;

                    while (i < itemCnt)
                    {
                        if (SW->arealistexactmatch)
                        {
                            if (stristr(Itms[i] + 1, find) == Itms[i] + 1)
                            {
                                break;
                            }
                        }
                        else
                        {
                            if (stristr(Itms[i] + 1, find) != NULL)
                            {
                                break;
                            }
                        }
                        i++;
                    }
                    if (i == itemCnt)
                    {
                        for (i = 0; i < currItem; i++)
                        {
                            if (SW->arealistexactmatch)
                            {
                                if (stristr(Itms[i] + 1, find) == Itms[i] + 1)
                                {
                                    break;
                                }
                            }
                            else
                            {
                                if (stristr(Itms[i] + 1, find) != NULL)
                                {
                                    break;
                                }
                            }
                        }
                        if (i == currItem)
                        {
                            i = itemCnt;
                        }
                    }
                    if ((i != currItem) && (i != itemCnt))
                    {
                        SelShowItem(Itms[currItem], curY, len, Norm, indent);
                        currItem = i;
                        curY = y1;
                        Top = currItem;

                        if ((itemCnt - 1) - currItem < y2 - y1)
                        {
                            if (currItem > y2 - y1)
                            {
                                curY = y2;
                                Top = currItem - (y2 - y1);
                            }
                            else
                            {
                                curY = currItem + y1;
                                Top = 0;
                            }
                        }
                        SelShowPage(Itms, y1, y2, len, Top, Norm, indent);
                        SelShowItem(Itms[currItem], curY, len, Sel, indent);
                    }
                }
                else if (Msg == '\b' && *find)
                {
                    *(find + strlen(find) - 1) = '\0';
                    WndCurr(hPrev);
                    WndClear(17 + strlen(find), 0, maxx - 36, 0, cm[CM_NINF]);
                    WndCurr(hWnd);
                }
                else
                {
                    memset(find, '\0', sizeof find);
                }
                break;

            }
            break;
        }
    }
    return -1;
}

int mainArea(void)
{
    WND *hCurr, *hWnd;
    int ret, wid, dep, i;

    msgederr = 0;
    wid = maxx - 1;
    dep = maxy - 2;

    WndClearLine(0, cm[MN_NTXT]);
    WndClearLine(maxy - 1, cm[MN_NTXT]);
    hCurr = WndTop();
    hWnd = WndOpen(0, 1, wid, dep, NBDR, 0, cm[MN_BTXT]);
    WndBox(0, 0, maxx - 1, maxy - 3, cm[MN_BTXT], SBDR);

    WndWriteStr(3, 0, cm[LS_TTXT], "EchoID");
    WndWriteStr(maxx - 19, 0, cm[LS_TTXT], "Msgs");
    WndWriteStr(maxx - 12, 0, cm[LS_TTXT], "New");
    WndWriteStr(maxx - 7, 0, cm[LS_TTXT], "Last");

    BuildList(&alist);

    ret = AreaBox(alist, 1, min(dep - 2, SW->areas), wid - 1, SW->area,
      hCurr, hWnd, cm[MN_STXT], cm[MN_NTXT], 1);

    for (i = 0; i < SW->areas; i++)
    {
        xfree(alist[i]);
    }

    xfree(alist);

    if (ret < 0)
    {
        msgederr = 1;
    }

    WndClose(hWnd);
    WndCurr(hCurr);
    return ret;
}

int selectarea(char *topMsg, int def)
{
    WND *hCurr, *hWnd;
    int ret, wid, dep, i;

    msgederr = 0;
    wid = maxx - 1;
    dep = maxy - 2;

    WndClearLine(0, cm[MN_NTXT]);
    WndClearLine(maxy - 1, cm[MN_NTXT]);
    hCurr = WndTop();
    hWnd = WndOpen(0, 1, wid, dep, NBDR, 0, cm[MN_BTXT]);
    WndBox(0, 0, maxx - 1, maxy - 3, cm[MN_BTXT], SBDR);
    WndCurr(hCurr);
    WndClear(0, 0, maxx - 36, 0, cm[CM_NINF]);
    WndCurr(hWnd);
    WndWriteStr(3, 0, cm[LS_TTXT], "EchoID");
    WndWriteStr(maxx - 21, 0, cm[LS_TTXT], "Msgs");
    WndWriteStr(maxx - 13, 0, cm[LS_TTXT], "New");
    WndWriteStr(maxx - 7, 0, cm[LS_TTXT], "Last");

    BuildList(&alist2);

    ret = SelBox(alist2, 1, dep - 2, wid - 1, def, hCurr, hWnd, cm[MN_STXT],
      cm[MN_NTXT], SELBOX_REPLYOTH, topMsg);

    for (i = 0; i < SW->areas; i++)
    {
        xfree(alist2[i]);
    }

    xfree(alist2);

    if (ret < 0)
    {
        msgederr = 1;
    }

    WndClose(hWnd);
    WndCurr(hCurr);

    if (ret < 0)
    {
        return SW->area;
    }

    return ret;
}
