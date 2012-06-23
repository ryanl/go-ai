
import warnings
warnings.filterwarnings('ignore',".*The popen2 module is deprecated*")

import os
import sys
import subprocess

def object_file_for_cpp(cpp_path, compiler_hash):
    fn = os.path.relpath(cpp_path).replace("/", "_")
    fn = fn.replace("\\", "_")

    return ".build/" + compiler_hash + "-" + fn + ".o"

def source_file_for_header(header_path):
    if len(header_path) > 4 and header_path[len(header_path) - 4:] == ".hpp":
        return header_path[0:len(header_path) - 4] + ".cpp"
    elif len(header_path) > 2 and header_path[len(header_path) - 2:] == ".h":
        return header_path[0:len(header_path) - 2] + ".cpp"
    else:
        # print header_path  + " is not an .hpp file"
        return ""

# local dependencies are non-system header files
# specifically those in src_folder
def read_dependencies_file_for(cpp_path):
    fn = os.path.relpath(cpp_path).replace("/", "_")
    fn = fn.replace("\\", "_")
    dep_path = ".build/" + fn + ".d"

    dependencies = []

    try:
        f = open(dep_path, "r")
    except IOError:  # dependency file doesn't exist
        return None

    deps = f.readlines()
    f.close()

    ret = []
    for d in deps:
        ret.append(d.strip())

    return ret

def rewrite_dependencies_file_for(cpp_path, compiler):
    fn = os.path.relpath(cpp_path).replace("/", "_")
    fn = fn.replace("\\", "_")
    dep_path = ".build/" + fn + ".d"

    cmd = compiler + [cpp_path,  "-MM", "-MG"]
    output = run_cmd_and_get_output(cmd)
    find_colon = output.find(":")

    if find_colon == -1:
        print ("ERROR: no colon in", output)
        sys.exit(1)
    else:
        output = output[find_colon + 1:]

    output = output.replace("\\\n", " ");
    dependencies = output.split(" ")

    f = open(dep_path, "w")
    for d in dependencies:
        if d.strip() != "":
            f.write(d.strip() + "\n")
    f.close()


def run_cmd_and_get_output(cmd):
    print ("$ " +  " ".join(cmd))
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, \
                         stderr=subprocess.PIPE)


    # r, w, e  = popen2.popen3(cmd)
    (output, err_output) = p.communicate()
    if err_output != "":
        print(err_output)

    return "".join(output)

def object_file_for_source_is_out_of_date(cpp_path, compiler_hash):
    dependencies = read_dependencies_file_for(cpp_path)

    if dependencies == None:
        # the source code has never been built!
        return True
    else:
        obj_path = object_file_for_cpp(cpp_path, compiler_hash)
        if not os.path.exists(obj_path):
            return True
        else:
            obj_mod_time = os.path.getmtime(obj_path)
            for dep in dependencies:
                if (not os.path.exists(dep)) or os.path.getmtime(dep) > obj_mod_time:
                    print ("Dependency " + dep + " is newer than the object file")
                    return True
            return False



def build_object_file_for_if_out_of_date(cpp_path, compiler, compiler_hash):
    obj_path = object_file_for_cpp(cpp_path, compiler_hash)
    if object_file_for_source_is_out_of_date(cpp_path, compiler_hash):
        print ("__OUT-OF-DATE: " + cpp_path)

        obj_path = object_file_for_cpp(cpp_path, compiler_hash)
        rewrite_dependencies_file_for(cpp_path, compiler)

        try:
            os.remove(obj_path)
        except OSError:
            pass # file does not exist

        cmd = compiler + [cpp_path, "-c", "-o", obj_path] #, "-ipo_c"
        compiler_output = run_cmd_and_get_output(cmd)

        if not os.path.exists(obj_path):
            print ("ERROR: object file not created by compile!")
            print (compiler_output)
            print ("")
            return (True, False) # not successful
        else:
            return (True, True) # successful
    else:
        print ("__UP-TO-DATE: " + cpp_path)
        return (False, True) # successful


#def get_symbols_in_file(object_file_path, defined_symbols):
#
#    if defined_symbols:
#        defined_option = "--defined-only"
#    else:
#        defined_option = "--undefined-only"
#
#    # for .a and .o files
#    cmd = ["nm", defined_option, object_file_path]
#    output = run_cmd_and_get_output(cmd)
#
#    # for .so files
#    cmd = ["nm", defined_option, "-D", object_file_path]
#    output += run_cmd_and_get_output(cmd)
#
#    symbols = []
#
#    for line in output.split("\n"):
#        split_line = line.split(" ")
#        if len(split_line) == 3:
#            symbol = split_line[2]
#            symbols.append(symbol)
#
#    return symbols


def add_symbols_provided_to_dict(cpp_path, symbol_dict):
    obj_path = object_file_for_cpp(cpp_path)
    symbols = get_symbols_in_file(obj_path, True)
    for s in symbols:
        symbol_dict[s] = cpp_path

def recursive_ls(root_dir, ret=[]):
    filenames = os.listdir(root_dir)
    for fn in filenames:
        fn_full = root_dir + "/" + fn

        if os.path.isdir(fn_full):
            recursive_ls(fn_full, ret) # pass ret byref
        else:
            ret.append(fn_full)
    return ret

# This is the clever bit, and the bit that may cause problems (it may
# try to be too clever and if part of your code doesn't compile it likes
# to look anywhere it can to find the functions)
def build_dependencies_and_link(target, cpp_path, compiler, bin_suffix, compiler_hash):
    bin_path = "bin/" + target + bin_suffix

    print ("")
    print ("BUILDING: " + bin_path)

    q = [os.path.abspath(cpp_path)]
    source_files_seen = { q[0] : True }

    for s in q:
        r = build_object_file_for_if_out_of_date(s, compiler, compiler_hash)
        if not r[1]:
            sys.exit(1) # error building one of the files


        for d in read_dependencies_file_for(s):
            t = source_file_for_header(d)
            if os.path.exists(t) and not (os.path.abspath(t) in source_files_seen):
                q.append(t)
                source_files_seen[os.path.abspath(t)] = True

    cmd = compiler[:] # create a copy

    for source_cpp in q:
        cmd.append(os.path.relpath(object_file_for_cpp(source_cpp, compiler_hash)))

    cmd += ["-o", bin_path]
    #for l in libraries.values():
    #    if l != "": cmd.append(l)

    print ("__LINKING: "  + bin_path)

    compiler_output = run_cmd_and_get_output(cmd)

    if not os.path.exists(bin_path):
        print ("ERROR: bin file " + bin_path + " not created by compile!")
        print ("COMPILER OUTPUT: " + compiler_output)
        return False # not successful
    else:
        print ("")
        return True # successful


def run_build(targets, chosen_targets, compiler, bin_suffix, compiler_hash):
    try:
        os.mkdir(".build");
    except OSError:
        pass # already exists

    try:
        os.mkdir("bin");
    except OSError:
        pass # already exists


    #os.chdir(src_folder)
    #os.chdir("..")

    #library_symbols = {}

    # First thing to do is find out what symbols the libraries provide
    # for lib in libraries.keys():
    #    symbols = get_symbols_in_file(lib, True)
    #    for s in symbols:
    #        library_symbols[s] = lib

    for target in chosen_targets:
        if not (target in targets):
            print ("Target " + target + " not known")
            sys.exit(1)
        else:
            cpp_path = targets[target]
            build_dependencies_and_link(target, cpp_path, compiler, bin_suffix, compiler_hash)

