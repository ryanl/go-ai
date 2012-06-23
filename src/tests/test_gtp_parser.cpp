#include "../interface_gtp/generic/gtp_parser.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    GTPParser p("GTP test engine", "0.0");
    p.run(std::cin, std::cout);
}
