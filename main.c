#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lualib.h"
#include "lauxlib.h"
#include "lualib.h"

extern int luaopen_lpeg (lua_State *L);

const char *const PRGNAME = "ILE";

int endcollect = 0;

int argpos = 1;

const char *scriptname = "main.lua";

static int compile = 0;
static const char *outname;
static int helpreq = 0;

static void help(void)
{
    printf(
"ILE, the Indepedent Lua Executable -- with embedded LPeg\n"
"");            
}

static void error_opt(const char *str, size_t len)
{
    printf("E: Unrecognized option '");
    if (len) 
        printf("%.*s", (int)len, str);
    else
        printf("%s", str);
    printf("'. Type 'ile --help' for help.\n");
}

static int short_opt(int c, char const *rest, int pos)
{
    switch (c)
    {
    case 'h':
        help();
        return 0;
    case 'f':
        scriptname = rest;
        argpos = pos < 0 ? ~pos : pos;
        return 1;
    default:
        {
            char str[2] = {c};
            error_opt(str, 0);
        }
        return -1;
    }
}

static int long_opt(char const *str, size_t len, char const *arg, int pos)
{
    if (!strncmp("help", str, len))
    {
        helpreq = 1;
        return 0;
    }
    if (!strncmp("file", str, len))
    {
        scriptname = arg;
        argpos = pos < 0 ? ~pos : pos;
        return 1;
    }
    error_opt(str, len);
    return -1;
}

static int not_opt(char const *str, int pos)
{
    if (!endcollect)
    {
        argpos = pos - 1;
        endcollect = 1;
    }
    return 0;
}

static int getargs(lua_State *L, const char **argv, int n) 
{
    int narg;
    int i;
    int argc = 0;
    while (argv[argc]) argc++;  /* count total number of arguments */
    narg = argc - (n + 1);  /* number of arguments to the script */
    luaL_checkstack(L, narg + 3, "too many arguments to script");
    for (i=n+1; i < argc; i++)
        lua_pushstring(L, argv[i]);
    lua_createtable(L, narg, n + 1);
    for (i=0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - n);
    }
    return narg;
}

static int lerror(lua_State *L, int status)
{
    if (status)
    {
        const char *p = lua_tostring(L, -1);
        printf("E: lua error: %s\n",p);
        lua_pop(L, 1);
    }
    return status;
}

static int handle_options(int argc, char const *argv[])
{
    int i;
    int code = 0;

    for (i=1; i < argc && code != -1; ++i)
    {
        char const *opt = argv[i];
        if (opt[0] == '-' && !endcollect)
        {
            if (opt[1] == '-')
            {
                // Collect potential options
                if (opt[2] == 0)
                {
                    // End of options
                    endcollect = 1;
                }
                else
                {
                    // Long opt
                    size_t loc = strcspn(opt, "=");
                    if (opt[loc])
                    {
                        // char '=' found.
                        code = long_opt(opt+2, loc-2, opt+loc+1, i);
                        if (code == 0)
                        {
                            // The option wasn't expecting an arg.
                            printf("E: ARG!\n");
                            code = -1; 
                        }
                    }
                    else
                    {
                        code = long_opt(opt+2, loc-2, argv[i+1], -i);
                        if (code == 1)
                        {
                            // long_opt has consumed the arg
                            ++i;
                        }
                    }
                }
            }
            else
            {
                // short opt
                int j;
                for (j=1; opt[j]; ++j)
                {
                    const char *arg = opt+j+1;
                    if (!*arg) 
                    {
                        code = short_opt(opt[j], argv[i+1], -i);
                        if (code==-1) break;
                        if (code) ++i;
                    }
                    else
                    {
                        code = short_opt(opt[j], arg, i);
                        // Break either if arg is consumed or there is an error.
                        if (code) break;
                    }
                }
            }
        }
        // Collect 'floating' (positional) arguments
        else // ==> (endcollect || opt[0] != '-')
        {
            code = not_opt(opt, i);
        }
    }
    return code;
}

int main(int argc, char const *argv[])
{
    int retcode;
    lua_State *L = luaL_newstate();
    if (!L)
    {
        puts("Could not create Lua state: abort");
        exit(EXIT_FAILURE);
    }
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP, 0);   /* stop collector during initialization */
    luaL_openlibs(L);           /* open libraries */
    luaopen_lpeg(L);
    lua_setglobal(L, "lpeg");
    lua_gc(L, LUA_GCRESTART, 0);
    {
        FILE *f = fopen(argv[0],"r");
        if (!f)
        {
            printf("fatal: could not open '%s'\n", argv[0]);
        }
        else
        {
            union {
                char buf[8];
                struct {
                    uint32_t _;
                    uint32_t offset;
                };
            } d = {0};
            fseek(f, -8, SEEK_END);
            fread(d.buf, sizeof(d.buf), 1, f);
            if (!strncmp(d.buf, "ILES", 4))
            {
                // This is a compiled executable
                fseek(f, -d.offset-8, SEEK_END);
                char *s = malloc(d.offset);
                fread(s, sizeof(d.offset), 1, f);
                int c = luaL_loadstring(L, s);
                free(s);
                if (!lerror(L, c))
                {
                    getargs(L, argv, 0);
                    lua_setglobal(L, "arg");
                    c = lua_pcall(L, argc, 1, 0);
                    if (lerror(L, c))
                    {
                        retcode = -1;
                    }
                    else
                    {
                        retcode = lua_tointeger(L, -1);
                    }
                }
            }
            else
            {
                // There is no embedded script
                retcode = handle_options(argc, argv);
                if (retcode >= 0)
                {
                    if (helpreq)
                    {
                        help();
                    }
                    else if (compile)
                    {
                        // Do not execute. Compile instead.
                    }
                    else
                    {
                        // Execute script
                        int c = luaL_loadfile(L, scriptname);

                        if (!lerror(L, c))
                        {
                            getargs(L, argv, argpos);
                            lua_setglobal(L, "arg");
                            c = lua_pcall(L, argc-argpos-1, 1, 0);
                            if (lerror(L, c))
                            {
                                retcode = -1;
                            }
                            else
                            {
                                retcode = lua_tointeger(L, -1);
                            }
                        }
                    }
                }
            }
        }
    }
    lua_close(L);
    return 0;
}
