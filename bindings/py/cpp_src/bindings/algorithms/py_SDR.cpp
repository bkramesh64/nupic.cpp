// TODO: LICENSE & COPYRIGHT

#include <bindings/suppress_register.hpp>  //include before pybind11.h
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <nupic/ntypes/Sdr.hpp>
#include <nupic/utils/StringUtils.hpp>  // trim

namespace py = pybind11;

using namespace nupic;

// TODO: docstrings everywhere!
namespace nupic_ext
{
    void init_SDR(py::module& m)
    {
        py::class_<SDR> py_SDR(m, "SDR");

        py_SDR.def(
            py::init<vector<UInt>>(),
            py::arg("inputDimensions")
        );

        py_SDR.def(
            py::init<SDR>(),
            py::arg("deepCopy")
        );

        // TODO: Deconstruct

        py_SDR.def_property_readonly("dimensions",
            [](const SDR &self) {
                return self.dimensions; });

        py_SDR.def_property_readonly("size",
            [](const SDR &self) {
                return self.size; });

        py_SDR.def("zero", &SDR::zero);


        // TODO: Reshape dense to correct dimensions!  This eliminates the need
        // for the "at" method since numpy can then do that just fine.
        py_SDR.def_property("dense",
            [](SDR &self) {
                auto capsule = py::capsule(&self, [](void *self) {});
                return py::array(self.size, self.getDense().data(), capsule);
            },
            [](SDR &self, SDR_dense_t data) {
                // TODO: CHECK DATA SIZE & DIMS VALID!
                self.setDense( data );
            }
        );

        py_SDR.def("setDenseInplace", [](SDR &self) {
            self.setDense( self.getDense() );
        });

        py_SDR.def_property("flatSparse",
            [](SDR &self) {
                auto capsule = py::capsule(&self, [](void *self) {});
                return py::array(self.getSum(), self.getFlatSparse().data(), capsule);
            },
            [](SDR &self, SDR_flatSparse_t data) {
                self.setFlatSparse( data );
            }
        );

        py_SDR.def_property("sparse",
            [](SDR &self) {
                auto capsule = py::capsule(&self, [](void *self) {});
                auto outer = py::list(capsule);
                for(auto dim = 0u; dim < self.dimensions.size(); dim++) {
                    auto vec = py::array(self.getSum(), self.getFlatSparse().data(), capsule);
                    outer.append(vec);
                }
                return outer;
            },
            [](SDR &self, SDR_sparse_t data) {
                self.setSparse( data );
            }
        );

        py_SDR.def("setSDR",      &SDR::setSDR);
        py_SDR.def("getSum",      &SDR::getSum);
        py_SDR.def("getSparsity", &SDR::getSparsity);
        py_SDR.def("overlap",     &SDR::overlap);

        // TODO: DEFAULT ARGUMENT FOR SEED = 0!
        py_SDR.def("randomize", [](SDR &self, Real sparsity, UInt seed = 0){
            Random rng( seed );
            self.randomize( sparsity, rng );
        });

        // TODO: DEFAULT ARGUMENT FOR SEED = 0!
        py_SDR.def("addNoise", [](SDR &self, Real fractionNoise, UInt seed = 0){
            Random rng( seed );
            self.addNoise( fractionNoise, rng );
        });

        py_SDR.def("__str__", [](SDR &self){
            stringstream buf;
            buf << self;
            return StringUtils::trim( buf.str() );
        });

        py_SDR.def("__eq__", [](SDR &self, SDR &other){ return self == other; });
        py_SDR.def("__ne__", [](SDR &self, SDR &other){ return self != other; });

        py_SDR.def(py::pickle(
            [](const SDR& self) {
                std::stringstream ss;
                self.save(ss);
                return ss.str();
        },
            [](std::string& s) {
                std::istringstream ss(s);
                SDR self;
                self.load(ss);
                return self;
        }));
    }
}
