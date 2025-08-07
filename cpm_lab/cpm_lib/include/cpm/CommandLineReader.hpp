#pragma once

#include <string>
#include <sstream>
#include <vector>

/**
 * \file CommandLineReader.hpp
 * \brief Used to read a single command line argument from argv
 */

namespace cpm {
    /**
     * \brief Read a boolean command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    bool cmd_parameter_bool(std::string name, bool default_value, int argc, char *argv[]);

    /**
     * \brief Read an integer command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    int cmd_parameter_int(std::string name, int default_value, int argc, char *argv[]);

    /**
     * \brief Read an integer command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    uint64_t cmd_parameter_uint64_t(std::string name, uint64_t default_value, int argc, char *argv[]);

    /**
     * \brief Read a double command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    double cmd_parameter_double(std::string name, double default_value, int argc, char *argv[]);

    /**
     * \brief Read a std::string command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    std::string cmd_parameter_string(std::string name, std::string default_value, int argc, char *argv[]);

    /**
     * \brief Read an integer command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    std::vector<int> cmd_parameter_ints(std::string name, std::vector<int> default_value, int argc, char *argv[]);

    /**
     * \brief Read a double command line argument from argv (form: --name=value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    std::vector<double> cmd_parameter_doubles(std::string name, std::vector<double> default_value, int argc, char *argv[]);
    /**
     * \brief Read an float array command line argument from argv (form: --name=value,value), use a default value if it does not exist
     * \ingroup cpmlib
     */
    std::vector<float> cmd_parameter_floats(std::string name, std::vector<float> default_value, int argc, char *argv[]);

}