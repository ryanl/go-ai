#!/usr/bin/python

# Automagical C/C++ project builder.
# Copyright (C) 2009-2012 Ryan J. Lothian
#-----------------------------------------------------------------------
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
# -----------------------------------------------------------------------
"""
Just tell it the binaries you want, and the .cpp containing the
main functions, and it'll do the rest.

One more thing - for working out dependencies, this program needs to
know which calls are to libraries (so it won't find the source code to
those calls).

To get rid of build files, do rm .build/* in the same directory.
"""
# #################################################################### #
#                    You can edit this section.                        #
# #################################################################### #

import os

mypath = os.path.dirname(os.path.abspath( __file__ ))

from build_common import *

src_folder = mypath + "/src/"
include_folder = mypath + "/include/"

os.utime(src_folder + "interface_gtp/gtp_main.cpp", None) # touch this file so build # is updated

targets = {
    "test_ads"                  : src_folder + "tests/test_ads.cpp",
    "test_improved_bitset"      : src_folder + "tests/test_improved_bitset.cpp",
    "test_go_state"             : src_folder + "tests/test_go_state.cpp",
    "test_static_vector"        : src_folder + "tests/test_static_vector.cpp",
    "test_go_state_interactive" : src_folder + "tests/test_go_state_interactive.cpp",
    "test_random_permutation"   : src_folder + "tests/test_random_permutation.cpp",
    "test_pattern_matcher"      : src_folder + "tests/test_pattern_matcher.cpp",
    "test_tree"                 : src_folder + "tests/test_tree.cpp",
    "test_benchmark"            : src_folder + "tests/test_benchmark.cpp",
    "test_rng"                  : src_folder + "tests/test_rng.cpp",

    "bandit_algorithms"         : src_folder + "bandit_algorithms/evaluate_algorithm.cpp",

    "go_gtp"                    : src_folder + "interface_gtp/gtp_main.cpp",
    "test_gtp_parser"           : src_folder + "tests/test_gtp_parser.cpp",
    "play_two_gtp_engines"      : src_folder + "play_two_gtp_engines.cpp",
    #"make_opening_book"         : src_folder + "make_opening_book.cpp",

    "genetic_tictactoe"         : src_folder + "genetic_algorithm/example_tictactoe.cpp",
    "genetic_go"                : src_folder + "genetic_algorithm/example_go.cpp"
}

# example: if you would use -lgd -lm as arguments to g++
#          libraries = ["/usr/lib/libc.a", "/usr/lib/libstdc++.so.6",
#                       "/usr/lib/libgd.a", "/usr/lib/libm.a"]

libraries = {
#    "/usr/lib/libc.a"         : "",       # always include the C standard library
#    "/usr/lib/libstdc++.so.6" : "",       # always include the C++ standard library
#    "/usr/lib/libgd.a"        : "-lgd",
#    "/usr/lib/libm.a"         : "-lm",
}

#compiler  = ["C:/MinGW/bin/g++", "-g3", "-O0"]
#compiler  = ["g++", "-O3", "-fomit-frame-pointer", "-march=core2", "-msse2"]
#compiler  = ["g++", "-O2", "-pipe", "-g", "-pg", "-march=native", "-msse3", "-Wall", "-DBUILD_NUM=\"" + str(build_number) + "\"", "-DNDEBUG"]

ai_options = {
    "OPT_CHOOSE_MAX_TIMESPLAYED" : True,

    "OPT_BITARRAY_FOR_PATTERNS"  : True,
    "BOOST_THREAD"               : True,
    "OPT_USE_NEW_ADS"            : True,
    "OPT_USE_IMPROVED_BITSET"    : True,
    "USE_BOOST_THREAD"           : True,

    "HAS_BUILTIN_CLZ"            : True,
    "SEED_RNG"                   : True,

    "HAS_BOOST_MATH"             : False, # enables an extra stat in play_two_gtp_engines
    "NDEBUG"                     : True,  # True disables asserts
    "STATIC_VECTOR_NOINIT_HACK"  : True,
    "USE_BUILTIN_POPCOUNT"       : True  # you can try your compiler's implementation of POPCOUNT
                                         # note: valgrind doesn't like POPCOUNT instructions
}

#"-pg", "-g", -lboost_thread-mt", "-pthread"

gxx_debug_compiler = [
    "g++",
    "-std=c++0x",
    "-g",
    "-pipe",
    "-march=native",
    "-Wall", "-Wextra",
    "-I" + src_folder,
    "-I" + include_folder,
]

gxx_compiler = [
    "g++",
    "-fomit-frame-pointer",                           # frees up a register but hinders debugging
    "-O3",                                            # produces slightly faster code than -O3 on my PC
    "-pipe",
    "-funroll-loops",
    "-march=native",
    "-Wall",
    #"-ftracer",                                      # helps the optimizer, ~7% speed boost
    #"-mno-sse4a",
    #"-mno-popcnt", #popcnt confuses valgrind
    "-mpopcnt",
    "-ffast-math", "-fsingle-precision-constant",    # small speed boost (measured as 3-4%)

    #"-latomic_ops",
    "-I" + src_folder,
    "-I" + include_folder,
]

compiler = gxx_compiler
libs = ["-lpthread", "-lboost_thread"]

allopts = ""
for opt in ai_options.keys():
    x = ai_options[opt]
    if x is not False:
        s = "-D" + opt
        if x is not True:
            s += "=" + x

        compiler.append(s)
        allopts += opt + " "
allopts = allopts.strip()

import md5
m = md5.new()
m.update(" ".join(compiler))
compiler_hash = m.hexdigest()[:16] # used to group build files by compiler options

compiler.append("-DALLOPTS=\"" + allopts + "\"")

if len(sys.argv) == 1:
    chosen_targets = targets.keys()
elif len(sys.argv) == 2:
    chosen_targets = [ sys.argv[1] ]
else:
    print ("Usage: " + sys.argv[0] + " [target]")
    sys.exit(1)


print ("")
print ("Ryan's build tool!")
print ("================================================================")


run_build(targets, chosen_targets, compiler + ["-DOPT_BOARDSIZE=9"], "", compiler_hash + "-9", libs)

