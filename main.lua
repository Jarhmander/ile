print ("arg=", arg, "...", ...);

print "Hello world from Lua 5.3";

print('lpeg = ', lpeg);
local lpeg = require 'lpeg';
print('lpeg = ', lpeg);

local P, S, C = lpeg.P, lpeg.S, lpeg.C;
local R, V    = lpeg.R, lpeg.V;
local Cmt     = lpeg.Cmt;
local Cf, Cc  = lpeg.Cf, lpeg.Cc;
local tonumber=tonumber;
local lilg = {};

do local _ENV = lilg;

    digit   = R"09";
    nzdigit = R"19";
    integer = C(nzdigit * digit^0) / tonumber;

    sp      = S" \t\n";
    sps     = sp^0;
    sep     = P',';

    list    = P { integer * sps * (sep * sps * V(1))^-1 };

    local plus = function (a,b) return a+b; end
    local bor  = function (a,b) return a|b; end
    local band = function (a,b) return a&b; end
    local bxor = function (a,b) return a~b; end
    local function foldwith(p, fun)
        return Cf(p, fun) * -1;
    end
    sumlist  = foldwith(list, plus);
    borlist  = foldwith(list, bor);
    bandlist = foldwith(list, band);
    bxorlist = foldwith(list, bxor);
end

local list = '1, 2, 3, 4, 5';
print("Sum of the list "..list.." is:         ", lilg.sumlist:match(list));
print("Bitwise-or of the list "..list.." is:  ", lilg.borlist:match(list));
print("Bitwise-and of the list "..list.." is: ", lilg.bandlist:match(list));
print("Bitwise-xor of the list "..list.." is: ", lilg.bxorlist:match(list));




