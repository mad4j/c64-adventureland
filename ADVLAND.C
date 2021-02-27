/* ADVLAND.C */

/* This file is a port of ADVEN1.BAS found on PC-SIG disk #203 */
/* This was originally written for DOS by Morten Lohre in 1993 */
/* It was further modified by Richard Goedeken in 2019.        */
/*                                                             */
/* Please see the NOTES.TXT and LICENSE.TXT files for more     */
/* information.                                                */
/*                                                             */
/* Copyright (C) 1993 by Morten Løhre, All Rights Reserved.    */
/* Copyright (C) 2019 by Richard Goedeken, All Rights Reselved.*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>
//#if defined(WIN32)
  #include <conio.h>
//#else
//  #include <unistd.h>
//  #include <termios.h>
//  #include <sys/select.h>
//#endif

signed int yes_no(void);
void empty_keyboardbuffer(void);
void welcome(void);
int get_input(void);
void look(void);
int get_item_string(int);
void turn(void);
bool action(int ac, const unsigned char **pAcVar);
int get_action_variable(const unsigned char **pAcVar);
void carry_drop(void);
int length(const char *s);
void copystring(char *dest, const char *source);
int comparestring(const char *s1, const char *s2, int maxlength);
bool check_logics(int cmd);

/* read static global data */
#include "ADVLAND.H"

/* dynamic global variables */
signed char     IA[IL];                 /* object locations */
signed int      NV[2];                  /* word numbers, NV[0] = first, NV[1] = second */
signed int      loadflag, endflag;      /* should we load or end? */
bool            is_dark;
signed int      room, lamp_oil, state_flags;
char            tps[80];                /* input string */

/* externals:
   IA[], I2[], loadflag, endflag, lamp_oil, is_dark, state_flags, room, NV[] */

/* console I/O replacement functions */

#if defined(WIN32)

void clrscr(void)
{
    system("cls");
}

#define getch _getch
#define kbhit _kbhit
#define getw  _getw
#define putw  _putw

#else

void clrscr(void)
{
    printf("\033[2J\033[H");
    return;
}
/*
int kbhit(void)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    // return non-zero value for pending character
    select(STDIN_FILENO+1, &readfds, NULL, NULL, &timeout);
    return FD_ISSET(STDIN_FILENO, &readfds);
}
*/
int getch(void)
{
    return fgetc(stdin);
}

#endif

char *gets(char *s)
{
    int i = 0;
    while (i < 79)
    {
        s[i] = getch();
        if (s[i] == '\b' || s[i] == 127)
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
            continue;
        }
        if ((s[i] < 'A' || s[i] > 'Z') &&
            (s[i] < 'a' || s[i] > 'z') &&
            s[i] != ' ' && s[i] != '\r' && s[i] != '\n')
        {
            continue;
        }
        if (s[i] == '\r' || s[i] == '\n')
        {
            printf("\n");
            break;
        }
        printf("%c", s[i]);
        i++;
    }
    s[i] = 0;
    return s;
}

/* Empty keyboard buffer */
void empty_keyboardbuffer(void)
{
    while (kbhit()!=0)
    {
        getch();
    }
}

/* Empty keyboard, get Y(es) or N(o), printf character with carriage return */
int yes_no(void)
{
    int ch;

    empty_keyboardbuffer();  /* empty keyboardbuffer */
    do
    {
        ch = getch();
        if (ch > 96) ch = ch - 32;
    } while (ch!=89 && ch!=78);
    printf("%c\n", ch);
    return(ch==89);  /* 1 if Y, 0 if N */
}


int main(void)
{
    int i;        /* counting variable */
    FILE *fd;     /* load file handle */

    for (i=0; i<IL; i++) IA[i]=I2[i];     /* reset object locations */
    loadflag = 1;
    endflag = 0;
    srand((unsigned)time(NULL));  /* randomize */
    
    /* On linux, we need to disable "canonical mode" of the terminal, so that it doesn't buffer lines */
/*
#if !defined(WIN32)
    struct termios oldT = {0};
    tcgetattr(STDIN_FILENO, &oldT);
    struct termios newT =oldT;
    newT.c_lflag &= ~ECHO;
    newT.c_lflag &= ~ICANON;
    newT.c_cc[VMIN] = 1;
    newT.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &newT);
#endif
*/
    /* print welcome message */
    welcome();

    while (!endflag)
    {
        while (loadflag)
        {
            loadflag = 0;
            room = AR;
            lamp_oil = LT;
            is_dark = false;
            state_flags = 0;
            printf("\nUse saved game (Y or N)? ");
            if (yes_no()) /* yes */
            {
                printf("Is previously saved game now on the current disk? ");
                if (yes_no()) /* yes */
                {
                    fd = fopen("ADVEN-1.DAT","rb");
                    state_flags = getw(fd);
                    lamp_oil = getw(fd);
                    is_dark = getw(fd) ? true : false;
                    room = getw(fd);
                    for (i=0; i<IL; i++) IA[i]=getw(fd);
                    fclose(fd);
                }
                else loadflag = 1;      /* HERE WE GO AGAIN... */
            }
            if (!loadflag)
            {
                clrscr();
                look();
                NV[0] = 0;
                turn();
            }
        }
        if (!get_input())
        {
            turn();
            if (!loadflag && !endflag)
            {
                if (IA[9] == -1)
                {
                    lamp_oil--;
                    if (lamp_oil < 0)
                    {
                        printf("Your lamp has run out of oil!\n");
                        IA[9] = 0;
                    }
                    else if (lamp_oil < 25) printf("Your lamp will run out of oil in %u turns!", lamp_oil);
                }
                NV[0] = 0;
                turn();
            }
        }
    }

#if !defined(WIN32)
    tcsetattr(STDIN_FILENO, TCSADRAIN, &oldT);
#endif
}

/* Welcome */
void welcome(void)
{
    clrscr();
    printf(" W E L C O M E   T O \n A D V E N T U R E - 1+ \n\n\n\n\n");
    printf("The object of your adventure is to find treasures and return them\n");
    printf("to the proper place for you to accumulate points.  I'm your clone.  Give me\n");
    printf("commands that consist of a verb & noun, i.e. GO EAST, TAKE KEY, CLIMB TREE,\n");
    printf("SAVE GAME, TAKE INVENTORY, FIND AXE, etc.\n\n");
    printf("You'll need some special items to do some things, but I'm sure that you'll be\n");
    printf("a good adventurer and figure these things out (which is most of the fun of\n");
    printf("this game).\n\n");
    printf("Note that going in the opposite direction won't always get you back to where\n");
    printf("you were.\n\n\n");
    printf("HAPPY ADVENTURING!!!\n\n\n\n\n");
    empty_keyboardbuffer();
    printf("************************** Press any key to continue **************************");
    getch();
    clrscr();
}

/* Evaluate user input */
/* Externals:
   tps, NV[], NVS[][] */

int get_input(void)
{
    int i,j;              /* counting variables */
    char *word[2];        /* first and second string */
    const char  *s;

    do
    {
        printf("\nTell me what to do? ");
        gets((char *) tps);
    } while (tps[0] == 0);
    for (i=0; i<length(tps); i++) tps[i]=toupper(tps[i]);
    i = 0;
    while (tps[i]==' ' && tps[i]!='\0') i++;  /* go to first word */
    word[0] = tps+i;
    while (tps[i]!=' ' && tps[i]!='\0') i++;  /* go to next space */
    if (tps[i] != '\0')
        tps[i++] = '\0';                      /* null-terminate first word */
    while (tps[i]==' ' && tps[i]!='\0') i++;  /* go to next word */
    word[1] = tps+i;
    for (i=0; i<2; i++)
    {
        NV[i] = 0;
        if (word[i][0]!='\0')
        {
            for (j=0; j<NL; j++)
            {
                s = NVS[i][j];
                if (s[0] == '*') s++;  /* skip special char */
                if (comparestring(word[i],s,LN)==0)
                {
                    NV[i] = j;
                    while (NVS[i][NV[i]][0] == '*') NV[i]--;
                    break;
                }
            }
        }
    }
    if (NV[0] < 1)
    {
        printf("I don't know how to %s!\n", word[0]);
        return 1;
    }
    if (word[1][0]!='\0' && NV[1] < 1)
    {
        printf("I don't know what a %s is!\n", word[1]);
        return 1;
    }
    return 0;
}

/* Print location description, exits and visible items */
/* Externals:
   is_dark, IA[], RSS[][], tps, room, RM[][], NVS[][] */

void look(void)
{
    int k;        /* Flag */
    int i,j;

    if (is_dark && (IA[9] != -1 && IA[9] != room))
    {
        printf("I can't see.  It's too dark!\n");
        return;
    }
        
    if (RSS[room][0] == '*') printf("%s", RSS[room]+1);
    else
    {
        printf("I'm in a %s",RSS[room]);
    }
    k = 1;
    for (i=0; i<IL; i++)
    {
        if (IA[i] == room)
        {
            if (k)
            {
                printf("\n\nVisible Items Here:\n");
                k = 0;
            }
            j = get_item_string(i);
            printf("   %.*s\n",j,IAS[i]);
        }
    }
    k = 1;
    for (i=0; i<6; i++)
    {
        if (RM[room][i]!=0)
        {
            if (k)
            {
                printf("\nObvious Exits:\n   ");
                k = 0;
            }
            printf("%s ",NVS[1][i + 1]);
        }
    }
    printf("\n\n");
}

void turn(void)
{
    int i,j,ac, go_direction = 0;

    if (NV[0] == 1)
    {
        if (NV[1] == 0)
        {
            printf("Where do you want me to go? Give me a direction too.\n");
            return;
        }
        else if (NV[1] < 7)
        {
            go_direction = NV[1];
        }
    }
    if (NV[0] >= 54 && NV[0] <= 59)
    {
        go_direction = NV[0] - 53;
    }

    if (go_direction > 0)
    {
        i = (is_dark) && (IA[9] != room) && (IA[9] != -1);
        if (i) printf("Warning: it's dangerous to move in the dark!\n");
        j = RM[room][go_direction - 1];
        if (j == 0 && !i)
        {
            printf("I can't go in that direction.\n");
        }
        else
        {
            if (j == 0 && i)
            {
                printf("I fell down and broke my neck.\n");
                j = RL-1;
                is_dark = false;
            }
            if (!i) clrscr();
            room = j;
            look();
        }
    }
    else
    {
        bool command_allowed = true;
        bool command_found = false;
        for (int cmd = 0; cmd < CL; cmd++)
        {
            i = C[cmd][0];
            if (NV[0] == 0 && i != 0)
                break;
            if (NV[0] == i)
            {
                i = C[cmd][1];
                if (i == NV[1] || i == 0 || (NV[0] == 0 && (rand()%100+1) <= i))
                {
                    command_found = true;
                    command_allowed = check_logics(cmd);
                    if (command_allowed)
                    {
                        const unsigned char *pAcVar = C[cmd];
                        for (int y = 12; y < 16; y++)
                        {
                            ac = C[cmd][y];
                            bool failed = action(ac, &pAcVar);
                            if (failed || loadflag == 1 || endflag == 1)
                                break;
                        }
                        if (NV[0] != 0 || loadflag == 1 || endflag == 1)
                            break;
                    }
                }
            }
        }
        if ((NV[0] == 10 || NV[0] == 18) && (!command_found || !command_allowed))
        {
            carry_drop();
            return;
        }
        if (NV[0] != 0)
        {
            if (!command_found)
                printf("I don't understand your command.\n");
            else if (!command_allowed)
                printf("I can't do that yet.\n");
        }
    }
}

/* externals:
   MSS[], IA[], NV[], room, is_dark, state_flags ... */
bool action(int ac, const unsigned char **pAcVar)
{
    FILE *fd;
    int i,j,p, linelen;

    if (ac > 101) printf("%s\n",MSS[ac - 50]);    /* Messages 52 and up */
    if (ac > 0 && ac < 52) printf("%s\n",MSS[ac]);  /* Messages 1 - 51 */
    if (ac == 52)
    {
        j = 0;
        for (i=1; i<IL; i++) if (IA[i] == -1) j++;
        if (j >= MX)
        {
            printf("I can't. I'm carrying too much!\n");
            return true;
        }
        else IA[get_action_variable(pAcVar)] = -1;
    }
    if (ac == 53) IA[get_action_variable(pAcVar)] = room;
    if (ac == 54) room = get_action_variable(pAcVar);
    if (ac == 55 || ac == 59) IA[get_action_variable(pAcVar)] = 0;
    if (ac == 56) is_dark = true;
    if (ac == 57) is_dark = false;
    if (ac == 58) state_flags |= 1 << get_action_variable(pAcVar);
    if (ac == 60) state_flags ^= 1 << get_action_variable(pAcVar);
    if (ac == 61)
    {
        printf("I'm dead...\n");
        room = RL-1;
        is_dark = false;
        look();
    }
    if (ac == 62)
    {
        i = get_action_variable(pAcVar);
        IA[i] = (get_action_variable(pAcVar));
    }
    if (ac == 63)
    {
        printf("The game is now over.\nAnother game? ");
        if (!yes_no())  /* No */ endflag = 1;
        else /* Yes */
        {
            for (i=0; i<IL; i++) IA[i] = I2[i];
            loadflag = 1;
        }
    }
    if (ac == 64) look();
    if (ac == 65)
    {
        j = 0;
        for (i=1; i<IL; i++) if (IA[i] == TR) if (IAS[i][0] == '*') j++;
        printf("I've stored %u treasures.  On a scale\nof 0 to 100, that rates a %u.\n",j,j*100/TT);
        if (j == TT)
        {
            printf("Well done.\nThe game is now over.\nAnother game? ");
            if (!yes_no())  /* No */ endflag = 1;
            else
            {
                for (i=0; i<IL; i++) IA[i] = I2[i];
                loadflag = 1;
            }
        }
    }
    if (ac == 66)
    {
        printf("I'm carrying:\n");
        j = -1;
        linelen = 0;
        for (i=0; i<IL; i++)
        {
            if (IA[i] == -1)
            {
                p = get_item_string(i);
                if ((p + linelen + 2) > MAXLINE) {
                    printf("\n");
                    linelen = 0;
                }
                printf("%.*s. ",p,IAS[i]);
                linelen += p;
                j = 0;
            }
        }
        if (j) printf("Nothing!\n");
    }
    if (ac == 67) state_flags |= 1;
    if (ac == 68) state_flags ^= 1;
    if (ac == 69)
    {
        lamp_oil = LT;
        IA[9] = -1;
    }
    if (ac == 70) clrscr();
    if (ac == 71)
    {
        printf("Is the current drive ready to receive the saved game? ");
        if (yes_no())
        {
            fd = fopen("ADVEN-1.DAT","wb");
            putw(state_flags,fd);
            putw(lamp_oil,fd);
            putw(is_dark,fd);
            putw(room,fd);
            for (i=0; i<IL; i++) putw(IA[i],fd);
            fclose(fd);
        }
        printf("\n");
    }
    if (ac == 72)
    {
        j = get_action_variable(pAcVar);
        p = get_action_variable(pAcVar);
        i = IA[j];
        IA[j] = IA[p];
        IA[p] = i;
    }
    return false;
}

/* Discard unwanted string at end of item description */
/* Externals:
   IAS[] */
/* Returns number of printable characters in item description */

int get_item_string(int i)
{
    int p;

    p = length(IAS[i]); /* points to back of string */
    if (IAS[i][p-1] == '/')
    {
        do
            p--;
        while (p>0 && IAS[i][p-1]!='/');
        if (IAS[i][p-1]!='/') p = length(IAS[i]);
        else p--;
    }
    return(p);
}

/* Externals: C[][] */
int get_action_variable(const unsigned char **pAcVar)
{
    do
    {
        *pAcVar += 2;
    } while ((*pAcVar)[1] != 0);
    return (*pAcVar)[0];
}

/* Can I carry or drop it? If so, do it. */
void carry_drop(void)
{
    if (NV[1] == 0)
    {
        printf("What?\n");
        return;
    }
    if (NV[0] == 10)
    {
        int items = 0;
        for (int i=0; i<IL; i++)
            if (IA[i] == -1)
                items++;
        if (items >= MX)
        {
            printf("I can't. I'm carrying too much!\n");
            return;
        }
    }
    
    bool found_object = false;
    for (int j=0; j<IL; j++)
    {
        int ll = length(IAS[j]) - 1;
        if (IAS[j][ll] != '/')
            continue;
        copystring(tps,IAS[j]);
        tps[ll] = '\0';     /* remove ending slash */
        do
        {
            ll--;
        } while (ll > 1 && tps[ll] != '/');
        ll++;   /* skip starting slash, point to first char in item name */
        if (comparestring(tps+ll, NVS[1][NV[1]], LN)==0)
        {
            found_object = true;
            if (NV[0] == 10)
            {
                if (IA[j] == room)
                {
                    IA[j] = -1;
                    printf("OK, taken.\n");
                    return;
                }
            }
            else
            {
                if (IA[j] == -1)
                {
                    IA[j] = room;
                    printf("OK, dropped.\n");
                    return;
                }
            }
        }
    }
    if (!found_object)
    {
        printf("It's beyond my power to do that.\n");
    }
    else
    {
        if (NV[0] == 10)
            printf("I don't see it here.\n");
        else
            printf("I'm not carrying it!\n");
    }
}

int length(const char *s)
{
    int i;

    i = 0;
    while (s[i] != '\0') i++;
    return(i);
}

void copystring(char *dest, const char *source)
{
    int i;

    i = 0;
    while (source[i]!='\0')
    {
        dest[i] = source[i];
        i++;
    }
    dest[i] = '\0';
}

int comparestring(const char *s1, const char *s2, int maxlength)
{
    int i = 0;
    while (s1[i]==s2[i] && s1[i] != 0 && i < maxlength) i++;
    if ((s1[i] == 0 && s2[i] == 0) || i == maxlength)
        return 0; // strings match
        
    return 1; // strings don't match
}

/* externals:
   C[][], room, IA[], state_flags */
bool check_logics(int cmd)
{
    int y,ll,k,i;

    bool allowed = true;
    y = 2;
    do
    {
        ll = C[cmd][y++];
        k = C[cmd][y++];
        if (k == 1) allowed = (IA[ll] == -1);
        if (k == 2) allowed = (IA[ll] == room);
        if (k == 3) allowed = (IA[ll] == room || IA[ll] == -1);
        if (k == 4) allowed = (room == ll);
        if (k == 5) allowed = (IA[ll] != room);
        if (k == 6) allowed = (IA[ll] != -1);
        if (k == 7) allowed = (room != ll);
        if (k == 8)
        {
            allowed = (state_flags & (1 << ll)) != 0;
        }
        if (k == 9)
        {
            allowed = (state_flags & (1 << ll)) == 0;
        }
        if (k == 10)
        {
            allowed = false;
            for (i=0; i<IL; i++)
            {
                if (IA[i] == -1)
                {
                    allowed = true;
                    i = IL;
                }
            }
        }
        if (k == 11)
        {
            allowed = true;
            for (i=0; i<IL; i++)
            {
                if (IA[i] == -1)
                {
                    allowed = false;
                    i = IL;
                }
            }
        }
        if (k == 12) allowed = (IA[ll] != -1 && IA[ll] != room);
        if (k == 13) allowed = (IA[ll] != 0);
        if (k == 14) allowed = (IA[ll] == 0);
    } while (y <= 10 && allowed);
    return allowed;
}
