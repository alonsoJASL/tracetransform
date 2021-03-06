//
// Configuration
//

// Include guard
#ifndef _TRACETRANSFORM_CIRCUS_
#define _TRACETRANSFORM_CIRCUS_

// Standard library
#include <cstddef>
#include <vector>

// Boost
#include <boost/optional.hpp>
#include <boost/program_options.hpp>

// Eigen
#include <Eigen/Dense>

// Local
#include "cudahelper/memory.hpp"


//
// Functionals
//

enum class PFunctional {
#ifdef WITH_CULA
    Hermite,
#endif
    P1,
    P2,
    P3
};

struct PFunctionalArguments {
    PFunctionalArguments(boost::optional<unsigned int> _order = boost::none,
                         boost::optional<size_t> _center = boost::none)
        : order(_order), center(_center) {}

    // Arguments for Hermite P-functional
    boost::optional<unsigned int> order;
    boost::optional<size_t> center;
};

struct PFunctionalWrapper {
    PFunctionalWrapper() {}

    PFunctionalWrapper(const std::string &_name, const PFunctional &_functional,
                       const PFunctionalArguments &_arguments =
                           PFunctionalArguments())
        : name(_name), functional(_functional), arguments(_arguments) {}

    std::string name;
    PFunctional functional;
    PFunctionalArguments arguments;
};

std::istream &operator>>(std::istream &in, PFunctionalWrapper &wrapper);


//
// Module definitions
//

std::vector<CUDAHelper::GlobalMemory<float> *>
getCircusFunctions(const CUDAHelper::GlobalMemory<float> *input,
                   const std::vector<PFunctionalWrapper> &pfunctionals);

#endif
