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

int argpos = 0;

const char *scriptname = "main.lua";

static int compile = 0;
static const char *outname = "a.out";
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
    case 'c':
        compile = 1;
        outname = rest;
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
    if (!strncmp("compile", str, len))
    {
        compile = 1;
        outname = arg;
        return 1;
    }
    error_opt(str, len);
    return -1;
}

static int not_opt(char const *str, int pos)
{
    if (compile)
    {
        printf("E: Unexpected argument '%s'\n", str);
        return -1;
    }
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
                            printf("E: Unexpected argument to '%.*s'.\n",
                                                (int)loc, opt);
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

static int preload_lpeg(lua_State *L)
{
    /* Technically, there's two arguments passed, but we don't care about 'em */
    luaopen_lpeg(L);
    return 1;
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

    lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD"); /* package.preload["lpeg"] = preload_lpeg */
    lua_pushcfunction(L, preload_lpeg);
    lua_setfield(L, -2, "lpeg");
    lua_pop(L, 1);

    lua_gc(L, LUA_GCRESTART, 0);
    char *myself = 0;
    int myself_size = 0;
    {
        FILE *f = fopen(argv[0],"rb");
        if (!f)
        {
            printf("fatal: could not open '%s'\n", argv[0]);
        }
        else
        {
            fseek(f, 0, SEEK_END);
            myself_size = ftell(f);
            myself = malloc(myself_size);
            rewind(f);
            fread(myself, 1, myself_size, f);
            if (!strncmp(myself+myself_size-8, "ILES", 4))
            {
                // This is a compiled executable
                myself[myself_size-8] = 0; // terminate it
                int codesize = *(int *)(myself+myself_size-4);
                int c = luaL_loadstring(L, myself+myself_size-codesize-8);
                free(myself);
                if (!lerror(L, c))
                {
                    getargs(L, argv, 0);
                    lua_setglobal(L, "arg");
                    c = lua_pcall(L, argc-1, 1, 0);
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
                        char buf[64];
                        FILE *o = fopen(outname,"wb");
                        if (!o)
                        {
                            printf("fatal: could not open '%s'\n", outname);
                        }
                        else
                        {
                            int n;
                            fwrite(myself, 1, myself_size, o);
                            free(myself);
                            myself = 0;
                            f = freopen(scriptname, "rb", f);
                            if (!f)
                            {
                                printf("fatal: could not open '%s'\n", scriptname);
                            }
                            while ((n = fread(buf, 1, sizeof(buf), f)) != 0)
                            {
                                fwrite(buf, 1, n, o);
                            }
                            fwrite("ILES", 1, 4, o);
                            fseek(f, 0, SEEK_END);
                            {
                                long i = ftell(f);
                                fwrite(&i, 1, 4, o);
                            }
                            fclose(o);
                            sprintf(buf, "chmod +x '%s'", outname);
                            system(buf);
                        }
                    }
                    else
                    {
                        // Execute script
                        int c = luaL_loadfile(L, scriptname);
                        free(myself);
                        myself = 0;

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
                    free(myself);
                    myself = 0;
                }
            }
            fclose(f);
        }
    }
    lua_close(L);
    return retcode;
}
