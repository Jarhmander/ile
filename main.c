#include <stdio.h>
#include <stdlib.h>
#include "lualib.h"
#include "lauxlib.h"
#include "lualib.h"

extern int luaopen_lpeg (lua_State *L);

int main()
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
