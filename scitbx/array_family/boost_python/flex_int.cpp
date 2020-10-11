#include <scitbx/array_family/boost_python/flex_wrapper.h>
#include <scitbx/array_family/boost_python/flex_pickle_single_buffered.h>
#include <scitbx/array_family/boost_python/byte_str.h>
#include <scitbx/array_family/boost_python/range_wrappers.h>
#include <scitbx/array_family/boost_python/numpy_bridge.hpp>
#include <scitbx/array_family/counts.h>
#include <scitbx/array_family/versa_matrix.h>
#include <scitbx/matrix/packed.h>
#include <scitbx/matrix/move.h>
#include <scitbx/stl/map_fwd.h>
#include <boost/python/args.hpp>
#include <boost/format.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/lexical_cast.hpp>
#include <map>

#if PY_MAJOR_VERSION >= 3
#define IS_PY3K
#endif

namespace scitbx { namespace af { namespace boost_python {

  template <typename intType>
  typename flex<intType>::type*
  from_std_string(const_ref<std::string> const& s)
  {
    shared<intType> result(reserve(s.size()));
    for(std::size_t i=0;i<s.size();i++) {
      if (s[i].size() == 0) {
        throw std::invalid_argument(
          "Empty string (integer value expected).");
      }
      intType value = 0;
      try {
        value = boost::lexical_cast<intType>(s[i]);
      }
      catch (boost::bad_lexical_cast const&) {
        throw std::invalid_argument(
          "Invalid integer value: \"" + s[i] + "\"");
      }
      result.push_back(value);
    }
    return new typename flex<intType>::type(result, result.size());
  }

  template <typename intType>
  shared<bool>
  as_bool(const_ref<intType> const& self, bool strict=true)
  {
    shared<bool> result((reserve(self.size())));
    for(std::size_t i=0;i<self.size();i++) {
      intType v = self[i];
      if (v == 0) {
        result.push_back(false);
      }
      else if (v == 1 || !strict) {
        result.push_back(true);
      }
      else {
        throw std::invalid_argument((
          boost::format(
            "scitbx.array_family.flex.int.as_bool(strict=True):"
            " all array elements must be 0 or 1,"
            " but value=%d at array index=%lu.") % v % i).str());
      }
    }
    return result;
  }

  template <typename intType>
  af::versa<long, af::flex_grid<> >
  as_long(
    af::const_ref<intType, af::flex_grid<> > const& O)
  {
    af::versa<long, af::flex_grid<> > result(
      O.accessor(), af::init_functor_null<long>());
    std::size_t n = O.accessor().size_1d();
    long* r = result.begin();
    for(std::size_t i=0;i<n;i++) {
      r[i] = static_cast<long>(O[i]);
    }
    return result;
  }

  /* For allowed syntax for the optional format_string argument see:
       http://www.boost.org/libs/format/doc/format.html#syntax
   */
  template <typename intType>
  af::shared<std::string>
  as_string(
    af::const_ref<intType, af::flex_grid<> > const& O,
    std::string format_string="%d")
  {
    std::size_t n = O.accessor().size_1d();
    af::shared<std::string> result((af::reserve(n)));
    for(std::size_t i=0;i<n;i++) {
      result.push_back((boost::format(format_string) % O[i]).str());
    }
    return result;
  }

template <typename intType>
#ifdef IS_PY3K
  PyObject*
#else
  std::string
#endif
  as_rgb_scale_string(
    af::const_ref<intType, af::flex_grid<> > const& O,
    af::tiny<double, 3> const& rgb_scales_low,
    af::tiny<double, 3> const& rgb_scales_high,
    intType saturation)
  {
    SCITBX_ASSERT(rgb_scales_low.const_ref().all_ge(0));
    SCITBX_ASSERT(rgb_scales_low.const_ref().all_le(1));
    SCITBX_ASSERT(rgb_scales_high.const_ref().all_ge(0));
    SCITBX_ASSERT(rgb_scales_high.const_ref().all_le(1));
    SCITBX_ASSERT(saturation != 0);
    double scale = 1. / saturation;
    std::size_t n = O.accessor().size_1d();
    std::string result(n*3, '\0');
    std::size_t j = 0;
    for(std::size_t i=0;i<n;i++) {
      double f = O[i] * scale;
      if      (f < 0) f = 0;
      else if (f > 1) f = 1;
      for(unsigned k=0;k<3;k++) {
        double fs = f * rgb_scales_high[k] + (1-f) * rgb_scales_low[k];
        intType c = static_cast<intType>(fs * 255 + 0.5);
        if (c > 255) c = 255;
        result[j++] = static_cast<char>(c);
      }
    }
#ifdef IS_PY3K
    return PyBytes_FromStringAndSize(result.c_str(), result.size());
#else
    return result;
#endif
  }

  template <typename intType>
  af::shared<intType> bitwise_not(
      af::const_ref<intType> const &self) {
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = ~self[i];
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_or_single(
      af::const_ref<intType> const &self,
      intType other) {
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] | other;
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_or_array(
      af::const_ref<intType> const &self,
      af::const_ref<intType> const &other) {
    SCITBX_ASSERT(self.size() == other.size());
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] | other[i];
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_and_single(
      af::const_ref<intType> const &self,
      intType other) {
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] & other;
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_and_array(
      af::const_ref<intType> const &self,
      af::const_ref<intType> const &other) {
    SCITBX_ASSERT(self.size() == other.size());
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] & other[i];
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_xor_single(
      af::const_ref<intType> const &self,
      intType other) {
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] ^ other;
    }
    return result;
  }

  template <typename intType>
  af::shared<intType> bitwise_xor_array(
      af::const_ref<intType> const &self,
      af::const_ref<intType> const &other) {
    SCITBX_ASSERT(self.size() == other.size());
    af::shared<intType> result(self.size());
    for (std::size_t i = 0; i < self.size(); ++i) {
      result[i] = self[i] ^ other[i];
    }
    return result;
  }

  // wrap each intType
  void wrap_flex_int()
  {
    using namespace boost::python;
    using boost::python::arg;
    flex_wrapper<int>::signed_integer("int", scope())
      .def_pickle(flex_pickle_single_buffered<int>())
      .def("__init__", make_constructor(
        from_std_string<int>, default_call_policies()))
      .def("__init__", make_constructor(
        flex_int_from_numpy_array, default_call_policies()))
      .def("copy_to_byte_str", copy_to_byte_str<versa<int, flex_grid<> > >)
      .def("slice_to_byte_str",
        slice_to_byte_str<versa<int, flex_grid<> > >)
      .def("as_bool", as_bool<int>, (arg("strict")=true))
      .def("as_long", as_long<int>)
      .def("as_string", as_string<int>, (arg("format_string")="%d"))
      .def("as_rgb_scale_string", as_rgb_scale_string<int>, (
        arg("rgb_scales_low"),
        arg("rgb_scales_high"),
        arg("saturation")))
      .def("counts", counts<int, std::map<long, long> >::unlimited)
      .def("counts", counts<int, std::map<long, long> >::limited, (
        arg("max_keys")))
      .def("matrix_is_symmetric",
        (bool(*)(
          const_ref<int, c_grid<2> > const&))
            matrix::is_symmetric)
      .def("matrix_copy_block",
        (versa<int, c_grid<2> >(*)(
          const_ref<int, c_grid<2> > const&,
          unsigned, unsigned, unsigned, unsigned))
            matrix::copy_block, (
              arg("i_row"),
              arg("i_column"),
              arg("n_rows"),
              arg("n_columns")))
      .def("matrix_transpose_in_place",
        (void(*)(versa<int, flex_grid<> >&)) matrix_transpose_in_place)
      .def("matrix_rot90",
        (versa<int, c_grid<2> >(*)(
           const_ref<int, c_grid<2> > const&, int)) matrix_rot90)
      .def("matrix_swap_rows_in_place",
        (void(*)(
          ref<int, c_grid<2> > const&, unsigned, unsigned))
            matrix::swap_rows_in_place, (
              arg("i"),
              arg("j")))
      .def("matrix_swap_columns_in_place",
        (void(*)(
          ref<int, c_grid<2> > const&, unsigned, unsigned))
            matrix::swap_columns_in_place, (
              arg("i"),
              arg("j")))
      .def("matrix_paste_block_in_place",
        (void(*)(
          ref<int, c_grid<2> > const&,
          const_ref<int, c_grid<2> > const&,
          unsigned, unsigned))
            matrix::paste_block_in_place, (
              arg("block"),
              arg("i_row"),
              arg("i_column")))
      .def("as_numpy_array", flex_int_as_numpy_array, (
        arg("optional")=false))
      .def("__invert__", &bitwise_not<int>)
      .def("__or__", &bitwise_or_single<int>)
      .def("__or__", &bitwise_or_array<int>)
      .def("__and__", &bitwise_and_single<int>)
      .def("__and__", &bitwise_and_array<int>)
      .def("__xor__", &bitwise_xor_single<int>)
      .def("__xor__", &bitwise_xor_array<int>)
    ;
    def(
      "int_from_byte_str",
      shared_from_byte_str<int>,
      (arg("byte_str")));
    range_wrappers<int, int>::wrap("int_range");
  }

}}} // namespace scitbx::af::boost_python
