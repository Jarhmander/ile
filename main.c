#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lualib.h"
#include "lauxlib.h"
#include "lualib.h"

extern int luaopen_lpeg (lua_State *L);

static int short_opt(int c, char const *rest)
{
}

static int long_opt(char const *str, size_t len, char const *arg)
{

}

static int not_opt(char const *str)
{

}

static int handle_options(int argc, char const *argv[])
{
    int i;
    int endcollect = 0;
    int code = 0;

    for (i=1; i < argc && code != -1; ++i)
    {
        char const *opt = argv[i];
        if (opt[0] == '-')
        {
            if (opt[1] == '-' && !endcollect)
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
                        code = long_opt(opt+2, loc-2, opt+loc+1);
                    }
                    else
                    {
                        code = long_opt(opt+2, loc-2, argv[i+1]);
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
                for (j=2; opt[j]; ++j)
                {
                    code = short_opt(opt[j], opt+j+1);
                    // Break either if arg is consumed or there is an error.
                    if (code) break;
                }
            }
        }
            // Collect 'floating' (positional) arguments
            else // ==> (endcollect || opt[1] != '-')
            {
                code = not_opt(opt);
            }

    }
}

int main(int argc, char const *argv[])
{
    lua_State *L = luaL_newstate();
    if (!L)
    {
        puts("Could not create Lua state: abort");
        exit(EXIT_FAILURE);
    }
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
    luaL_openlibs(L);  /* open libraries */
    luaopen_lpeg(L);
    lua_setglobal(L, "lpeg");
    lua_gc(L, LUA_GCRESTART, 0);
    int r = luaL_loadfilex(L, "script.lua", "t");
    switch(r) 
    {
        case LUA_OK:
            lua_call(L,0,0);
            break;
        case LUA_ERRSYNTAX:
        case LUA_ERRMEM:
        case LUA_ERRGCMM:
        case LUA_ERRFILE:
            puts(lua_tostring(L, -1));
            lua_pop(L, 1);
    }
    lua_close(L);
    return 0;
}
