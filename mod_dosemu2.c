// dosemu2 specific code
#define DOSEMU2

static int parse_cmdline(void);

// FIXME: Should be changed to static as soon as you implement it locally below
extern int truename(char *dest, const char *src, int allowwildcards);
extern int build_posix_path(char *dest, const char *src, int allowwildcards);

#include "mod_dosemu.c"
#include <ctype.h>

/* FIXME:
 * Dosemu2 maintainers recommend to use "official" API instead of direct
 * function call.
 * As I don't know how to pass around a stack pointer from unix to
 * DOS world (so that it then can get converted back with MK_FP32) and
 * this generates unnecessary overhead, this isn't implemented. Feel free 
 * to do so, this is left as an exercise to the user, if user wants to
 * get the plugin approved as offical part of dosemu2:
static int truename(char *dest, const char *src, int allowwildcards)
{
    struct vm86_regs saved_regs = REGS;
    int ret;

    _DS = FP_SEG32((int)src);
    _SI = FP_OFF32((int)src);
    _ES = FP_SEG32((int)dest);
    _DI = FP_OFF32((int)dest);
    _CL = 0;
    _AX = 0x60;
    do_int_call_back(0x21);
    ret = _AX;
    REGS = saved_regs;
    return ret;
}

// build_posix_path is public, still, going via API is recommended
static int build_posix_path(char *dest, const char *src, int allowwildcards)
{
    // Would need to implement a new "API" in dosemu2 for this
    return 0;
}
*/

static void parse_pathmap(char *file)
{
    char lnxfile[PATH_MAX]={0}, line[PATH_MAX*2+2], *src, *dst, *p;
    FILE *fp;

    build_posix_path(lnxfile, file, 0);
    if (!(fp = fopen(lnxfile, "r")))
    {
        warn("BTRVDD: open %s -> %s failed\n", file, lnxfile);
        return;
    }
    while (fgets(line, sizeof(line), fp))
    {
        if (*line==';' || *line=='[') continue;
        if ((src=strtok(line, "=")) && (dst=strtok(NULL, "=")))
        {
            /* Trim */
            while (isspace(*src)) src++; while(isspace(*dst)) dst++;
            for (p=src+strlen(src)-1; isspace(*p)&&p>src; p--) *p=0;
            for (p=dst+strlen(dst)-1; isspace(*p)&&p>dst; p--) *p=0;
            warn("btrv_mappath %s => %s\n", src,dst);
            btrv_mappath(strdup(src), strdup(dst));
        }
    }
    fclose(fp);
}

// Parse .SYS file commandline 
static int parse_cmdline(void)
{
    char *cmdl, *p, *p1, *tok;

    /* ED:DI points to device request header */
    p = FAR2PTR(READ_DWORD(SEGOFF2LINEAR(_ES, _DI) + 18));
    p1 = strpbrk(p, "\r\n");
    if (p1)
        cmdl = strndup(p, p1 - p);	// who knows if DOS puts \0 at end
    else
        cmdl = strdup(p);
    for (tok=strtok(cmdl," "); tok; tok=strtok(NULL, " "))
    {
        if ((p=strchr(tok, '=')))
        {
            *p=0; p++;
            if (strcasecmp(tok, "maplocalpath") == 0) btrv_maplocalpath = strcasecmp(p, "1") || strcasecmp(p, "true") || strcasecmp(p, "on"); else
            if (strcasecmp(tok, "mappathfile") == 0) parse_pathmap(p);
        }
    }
    free(cmdl);
    return 0;
}
