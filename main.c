#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "lualib.h"
#include "lauxlib.h"
#include "lualib.h"

/* luaopen_* functions */
extern int luaopen_lpeg (lua_State *L);

/* Preloaders */

/*
 * LPeg
 */
static int preload_lpeg(lua_State *L) {
    /* Technically, there's two arguments passed, but we don't care about 'em */
    luaopen_lpeg(L);
    return 1;
}

static const struct preloaders {
    lua_CFunction fn;
    const char *iname;
} preloadtable[] =
{
    {preload_lpeg, "lpeg"},

    /* Sentinel value, do not remove or insert after */
    { NULL, NULL }
};

static void register_preloaders(lua_State *L) {
    const struct preloaders *pre = preloadtable;

    lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD"); /* package.preload["lpeg"] = preload_lpeg */

    while (pre->fn) {
        lua_pushcfunction(L, pre->fn);
        lua_setfield(L, -2, pre->iname);
        ++pre;
    }
    lua_pop(L, 1);
}

/* will be set to argv[0] */
const char *PRGNAME = "(null)";

int endcollect = 0;

int argpos = 0;

const char *scriptname = "main.lua";

static int compile = 0;
static const char *outname = "a.out";
static int helpreq = 0;

static void help(void) {
    printf(
"%s, the Independent Lua Executable\n"
"\n"
"It needs some text here.\n"
, PRGNAME);
}

/*
 *
 */
static void error_opt(const char *str, size_t len) {
    printf("Error: unrecognized option '");
    if (len)
        printf("%.*s", (int)len, str);
    else
        printf("%s", str);
}

/*
 * Create a global table 'arg' with all the arguments and the arguments to the
 * script on the stack (so it can be accessed with vararg '...')
 *
 * It does somewhat like the standard Lua interpreter:
 *
 * $ ile -f foo.lua bar fred
 *
 * Will give:
 *   arg[-2] = "ile", arg[-1] = "-f", arg[0] = "foo.lua", arg[1] = "bar",
 *   arg[2] = "fred"
 * and
 *   ... => "bar", "fred"
 *
 * n is the index into argv of the future arg[0]
 * Returns the number of arguments given to script.
 */
static int getargs(lua_State *L, const char **argv, int n) {
    int narg;
    int i;
    int argc = 0;

    while (argv[argc]) argc++;

    narg = argc - (n + 1);  /* number of arguments to the script */
    luaL_checkstack(L, narg + 3, "too many arguments to script");
    for (i=n+1; i < argc; i++)
        lua_pushstring(L, argv[i]);
    lua_createtable(L, narg, n + 1);
    for (i=0; i < argc; i++) {
        lua_pushstring(L, argv[i]);
        lua_rawseti(L, -2, i - n);
    }
    lua_setglobal(L, "arg");
    return narg;
}

static int lerror(lua_State *L, int status) {
    if (status) {
        const char *p = lua_tostring(L, -1);
        printf("E: lua error: %s\n",p);
        lua_pop(L, 1);
    }
    return status;
}

static int handle_options(int argc, char const *argv[]) {
    int i;
    int code = 0;

    PRGNAME = argv[0];

    for (i=1; i < argc && code != -1; ++i) {
        // I need to do something!
    }
    return code;
}

/*
 * Simple initialisation
 */
static void lua_init(lua_State *L) {
    luaL_checkversion(L);
    lua_gc(L, LUA_GCSTOP, 0);   /* stop collector during initialization */

    luaL_openlibs(L);           /* open libraries */
    register_preloaders(L);     /* register preloaders */

    lua_gc(L, LUA_GCRESTART, 0);
}

int main(int argc, char const *argv[]) {
    int retcode = EXIT_FAILURE;
    lua_State *L = luaL_newstate();
    if (!L) {
        puts("Could not create Lua state: abort");
        exit(EXIT_FAILURE);
    }

    lua_init(L);

    char *myself = 0;
    int myself_size = 0;

    FILE *f = fopen(argv[0],"rb");
    if (!f) {
        printf("fatal: could not open '%s'\n", argv[0]);
        goto quit;
    }
    fseek(f, 0, SEEK_END);
    myself_size = ftell(f);
    myself = malloc(myself_size);
    rewind(f);
    fread(myself, 1, myself_size, f);
    if (!strncmp(myself+myself_size-8, "ILES", 4)) {
        // This is a compiled executable
        myself[myself_size-8] = 0; // terminate it
        int codesize = *(int *)(myself+myself_size-4);
        int c = luaL_loadstring(L, myself+myself_size-codesize-8);
        free(myself);
        if (!lerror(L, c)) {
            getargs(L, argv, 0);
            c = lua_pcall(L, argc-1, 1, 0);
            if (lerror(L, c)) {
                retcode = -1;
            }
            else {
                retcode = lua_tointeger(L, -1);
            }
        }
    }
    else {
        // There is no embedded script
        retcode = handle_options(argc, argv);
        if (retcode >= 0) {
            if (helpreq) {
                help();
            } else if (compile) {
                // Do not execute. Compile instead.
                char buf[64];
                FILE *o = fopen(outname,"wb");
                if (!o) {
                    printf("fatal: could not open '%s'\n", outname);
                }
                else {
                    int n;
                    fwrite(myself, 1, myself_size, o);
                    free(myself);
                    myself = 0;
                    f = freopen(scriptname, "rb", f);
                    if (!f) {
                        printf("fatal: could not open '%s'\n", scriptname);
                    }
                    while ((n = fread(buf, 1, sizeof(buf), f)) != 0) {
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
            else {
                // Execute script
                int c = luaL_loadfile(L, scriptname);
                free(myself);
                myself = 0;

                if (!lerror(L, c)) {
                    getargs(L, argv, argpos);
                    c = lua_pcall(L, argc-argpos-1, 1, 0);
                    if (lerror(L, c)) {
                        retcode = -1;
                    } else {
                        retcode = lua_tointeger(L, -1);
                    }
                }
            }
            free(myself);
            myself = 0;
        }
    }
    fclose(f);
quit:
    lua_close(L);
    return retcode;
}
