/* Copyright (c) 2001-2002 The Regents of the University of California
   through E.O. Lawrence Berkeley National Laboratory, subject to
   approval by the U.S. Department of Energy.
   See files COPYRIGHT.txt and LICENSE.txt for further details.

   Revision history:
     2002 Aug: Created, based on cctbx/fftbx/fftbxmodule.cpp (rwgk)
     2001 Dec: Using iterator-based fftbx interface (rwgk)
     2001 Nov: Created (R.W. Grosse-Kunstleve)
 */

#include <scitbx/error.h>
#include <scitbx/fftpack/gridding.h>
#include <scitbx/fftpack/complex_to_complex_3d.h>
#include <scitbx/fftpack/real_to_complex_3d.h>
#include <scitbx/boost_python/utils.h>
#include <scitbx/array_family/flex_types.h>
#include <scitbx/array_family/boost_python/utils.h>
#include <scitbx/array_family/boost_python/tiny_conversions.h>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>

namespace scitbx { namespace fftpack { namespace {

  void raise_size_error()
  {
    PyErr_SetString(PyExc_RuntimeError, "Array is too small.");
    boost::python::throw_error_already_set();
  }

  void assert_0_based_1d_size(
    af::flex_grid<> const& grid,
    std::size_t sz)
  {
    af::boost_python::assert_0_based_1d(grid);
    if (grid.size1d() < sz) raise_size_error();
  }

  void assert_0_based_3d_size(
    af::flex_grid<> const& grid,
    af::int3 const& fft_n)
  {
    af::boost_python::assert_0_based_3d(grid);
    for(std::size_t i=0;i<3;i++) {
      if (grid.grid()[i] != fft_n[i]) raise_size_error();
    }
  }

  int adjust_gridding_2(int min_grid,
                        int max_prime)
  {
    return adjust_gridding(min_grid, max_prime);
  }

  int adjust_gridding_3(int min_grid,
                        int max_prime,
                        int mandatory_factor)
  {
    return adjust_gridding(min_grid, max_prime, mandatory_factor);
  }

  af::flex_grid_default_index_type
  adjust_gridding_triple_2(
    af::flex_grid_default_index_type const& min_grid,
    int max_prime)
  {
    return adjust_gridding_array_flex(min_grid, max_prime);
  }

  af::flex_grid_default_index_type
  adjust_gridding_triple_3(
    af::flex_grid_default_index_type const& min_grid,
    int max_prime,
    af::flex_grid_default_index_type const& mandatory_factors)
  {
    return adjust_gridding_array_flex(min_grid, max_prime,
                                             mandatory_factors);
  }

  struct factorization_wrappers
  {
    typedef factorization w_t;

    static void
    def_all(boost::python::module& this_module)
    {
      using namespace boost::python;
      this_module.add(
        class_<w_t>("factorization",
          args<>())
          .def_init(args<std::size_t, bool>())
          .def("n", &w_t::n)
          .def("factors", &w_t::factors)
      );
    }
  };

  typedef af::flex_double flex_real_array;
  typedef af::ref<double, af::grid<3> > ref_3d_real_array;
  typedef af::flex_complex_double flex_complex_array;
  typedef af::ref<std::complex<double>, af::grid<3> > ref_3d_complex_array;

  struct complex_to_complex_wrappers
  {
    typedef complex_to_complex<double> w_t;

    static flex_complex_array
    forward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.n());
      fft.forward(a.begin());
      return flex_complex_array(a, af::flex_grid<>(fft.n())
        .set_layout(fft.n()));
    }

    static flex_complex_array
    forward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), 2 * fft.n());
      fft.forward(a.begin());
      return flex_complex_array(a.handle(), af::flex_grid<>(fft.n())
        .set_layout(fft.n()));
    }

    static flex_complex_array
    backward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.n());
      fft.backward(a.begin());
      return flex_complex_array(a, af::flex_grid<>(fft.n())
        .set_layout(fft.n()));
    }

    static flex_complex_array
    backward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), 2 * fft.n());
      fft.backward(a.begin());
      return flex_complex_array(a.handle(), af::flex_grid<>(fft.n())
        .set_layout(fft.n()));
    }

    static void
    def_all(boost::python::module& this_module)
    {
      using namespace boost::python;
      this_module.add(
        class_<w_t, bases<factorization> >("complex_to_complex",
          args<>())
          .def_init(args<std::size_t>())
          .def("wa", &w_t::wa)
          .def("forward", forward_complex)
          .def("forward", forward_real)
          .def("backward", backward_complex)
          .def("backward", backward_real)
      );
    }
  };

  struct real_to_complex_wrappers
  {
    typedef real_to_complex<double> w_t;

    static flex_complex_array
    forward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.n_complex());
      fft.forward(a.begin());
      return flex_complex_array(a, af::flex_grid<>((fft.n_complex()))
        .set_layout(fft.n_complex()));
    }

    static flex_complex_array
    forward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.m_real());
      fft.forward(a.begin());
      return flex_complex_array(a.handle(), af::flex_grid<>((fft.n_complex()))
        .set_layout(fft.n_complex()));
    }

    static flex_real_array
    backward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.n_complex());
      fft.backward(a.begin());
      return flex_real_array(a.handle(), af::flex_grid<>((fft.m_real()))
        .set_layout(fft.n_real()));
    }

    static flex_real_array
    backward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_1d_size(a.accessor(), fft.m_real());
      fft.backward(a.begin());
      return flex_real_array(a, af::flex_grid<>((fft.m_real()))
        .set_layout(fft.n_real()));
    }

    static void
    def_all(boost::python::module& this_module)
    {
      using namespace boost::python;
      this_module.add(
        class_<w_t, bases<factorization> >("real_to_complex",
          args<>())
          .def_init(args<std::size_t>())
          .def("n_real", &w_t::n_real)
          .def("m_real", &w_t::m_real)
          .def("n_complex", &w_t::n_complex)
          .def("wa", &w_t::wa)
          .def("forward", forward_complex)
          .def("forward", forward_real)
          .def("backward", backward_complex)
          .def("backward", backward_real)
      );
    }
  };

  struct complex_to_complex_3d_wrappers
  {
    typedef complex_to_complex_3d<double> w_t;

    static flex_complex_array
    forward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.n());
      ref_3d_complex_array map(a.begin(), af::grid<3>(fft.n()));
      fft.forward(map);
      return flex_complex_array(a, af::flex_grid<>(af::adapt(fft.n()))
        .set_layout(af::adapt(fft.n())));
    }

    static flex_complex_array
    forward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), n_real_from_n_complex(fft.n()));
      ref_3d_real_array map(
        a.begin(), af::grid<3>(n_real_from_n_complex(fft.n())));
      fft.forward(map);
      return flex_complex_array(a.handle(), af::flex_grid<>(af::adapt(fft.n()))
        .set_layout(af::adapt(fft.n())));
    }

    static flex_complex_array
    backward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.n());
      ref_3d_complex_array map(a.begin(), af::grid<3>(fft.n()));
      fft.backward(map);
      return flex_complex_array(a, af::flex_grid<>(af::adapt(fft.n()))
        .set_layout(af::adapt(fft.n())));
    }

    static flex_complex_array
    backward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), n_real_from_n_complex(fft.n()));
      ref_3d_real_array map(
        a.begin(), af::grid<3>(n_real_from_n_complex(fft.n())));
      fft.backward(map);
      return flex_complex_array(a.handle(), af::flex_grid<>(af::adapt(fft.n()))
        .set_layout(af::adapt(fft.n())));
    }

    static void
    def_all(boost::python::module& this_module)
    {
      using namespace boost::python;
      this_module.add(
        class_<w_t>("complex_to_complex_3d",
          args<>())
          .def_init(args<std::size_t, std::size_t, std::size_t>())
          .def_init(args<af::int3>())
          .def("n", &w_t::n)
          .def("forward", forward_complex)
          .def("forward", forward_real)
          .def("backward", backward_complex)
          .def("backward", backward_real)
      );
    }
  };

  struct real_to_complex_3d_wrappers
  {
    typedef real_to_complex_3d<double> w_t;

    static flex_complex_array
    forward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.n_complex());
      ref_3d_complex_array map(a.begin(), af::grid<3>(fft.n_complex()));
      fft.forward(map);
      return flex_complex_array(a,
        af::flex_grid<>(af::adapt((fft.n_complex())))
        .set_layout(af::adapt(fft.n_complex())));
    }

    static flex_complex_array
    forward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.m_real());
      ref_3d_real_array map(a.begin(), af::grid<3>(fft.m_real()));
      fft.forward(map);
      return flex_complex_array(a.handle(),
        af::flex_grid<>(af::adapt((fft.n_complex())))
        .set_layout(af::adapt(fft.n_complex())));
    }

    static flex_real_array
    backward_complex(w_t& fft, flex_complex_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.n_complex());
      ref_3d_complex_array map(a.begin(), af::grid<3>(fft.n_complex()));
      fft.backward(map);
      return flex_real_array(a.handle(),
        af::flex_grid<>(af::adapt((fft.m_real())))
        .set_layout(af::adapt(fft.n_real())));
    }

    static flex_real_array
    backward_real(w_t& fft, flex_real_array a)
    {
      a.size(); // assertion
      assert_0_based_3d_size(a.accessor(), fft.m_real());
      ref_3d_real_array map(a.begin(), af::grid<3>(fft.m_real()));
      fft.backward(map);
      return flex_real_array(a, af::flex_grid<>(af::adapt((fft.m_real())))
        .set_layout(af::adapt(fft.n_real())));
    }

    static void
    def_all(boost::python::module& this_module)
    {
      using namespace boost::python;
      this_module.add(
        class_<w_t>("real_to_complex_3d",
          args<>())
          .def_init(args<std::size_t, std::size_t, std::size_t>())
          .def_init(args<af::int3>())
          .def("n_real", &w_t::n_real)
          .def("m_real", &w_t::m_real)
          .def("n_complex", &w_t::n_complex)
          .def("forward", forward_complex)
          .def("forward", forward_real)
          .def("backward", backward_complex)
          .def("backward", backward_real)
      );
    }
  };

  void init_module(boost::python::module& this_module)
  {
    this_module
      .setattr("__version__",
        scitbx::boost_python::cvs_revision("$Revision$"))
    ;

    scitbx::af::boost_python::register_tiny_types_conversions();
    scitbx::boost_python::import_module("scitbx_boost.array_family.flex");

    this_module
      .def("adjust_gridding", adjust_gridding_2)
      .def("adjust_gridding", adjust_gridding_3)
      .def("adjust_gridding_triple", adjust_gridding_triple_2)
      .def("adjust_gridding_triple", adjust_gridding_triple_3)
    ;

    factorization_wrappers::def_all(this_module);
    complex_to_complex_wrappers::def_all(this_module);
    real_to_complex_wrappers::def_all(this_module);
    complex_to_complex_3d_wrappers::def_all(this_module);
    real_to_complex_3d_wrappers::def_all(this_module);
  }

}}} // namespace scitbx::fftpack::<anonymous>

BOOST_PYTHON_MODULE_INIT(fftpack)
{
  boost::python::module this_module("fftpack");
  scitbx::fftpack::init_module(this_module);
}
