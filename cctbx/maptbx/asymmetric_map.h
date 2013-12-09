#ifndef CCTBX_MAPTBX_ASYMMETRIC_MAP_H
#define CCTBX_MAPTBX_ASYMMETRIC_MAP_H

#include <boost/config.hpp>
#include <utility>
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
// #  include <boost/tr1/utility.hpp>
// #  include <boost/move/utility.hpp>
// namespace std { using boost::move; }
#endif

#include <scitbx/array_family/tiny.h>
#include <scitbx/array_family/tiny_algebra.h>
#include <scitbx/array_family/flex_types.h>
#include <scitbx/array_family/accessors/c_interval_grid.h>
#include <cctbx/sgtbx/space_group_type.h>
#include <scitbx/array_family/accessors/c_grid_padded.h>
#include <cctbx/sgtbx/direct_space_asu/proto/asymmetric_unit.h>
#include <scitbx/array_family/loops.h>
#include <scitbx/fftpack/real_to_complex_3d.h>

//! @todo move grid_symop to cctbx/sgtbx
#include <mmtbx/masks/grid_symop.h>

namespace cctbx { namespace maptbx
{

//! @todo code duplication: mmtbx/masks/atom_mask.cpp
inline void translate_into_cell(scitbx::int3 &num, const scitbx::int3 &den)
{
  for(register unsigned char i=0; i<3; ++i)
  {
    register int tn = num[i];
    register const int td = den[i];
    tn %= td;
    if( tn < 0 )
      tn += td;
    num[i] = tn;
  }
}

//! @todo: code duplication, see atom_mask.h
inline unsigned short site_symmetry_order(
  const std::vector<cctbx::sgtbx::grid_symop> &symops,
  const scitbx::int3 &num, const scitbx::int3 &den )
{
  unsigned short nops = 0;
  // num must be  inside cell
  for(size_t i=0; i<symops.size(); ++i)
  {
    scitbx::int3 sv = symops[i].apply_to( num );
    translate_into_cell(sv, den);
    if( scitbx::eq_all(sv , num) )
      ++nops;
  }
  CCTBX_ASSERT( nops>0U );
  return nops;
}

template<typename Grid> class mapped_grid_loop :
  public scitbx::af::nested_loop<Grid>
{
  typedef scitbx::af::nested_loop<Grid> base_t;

  using base_t::current_;
  using base_t::end_;
  using base_t::begin_;
  using base_t::over_;
  using base_t::incr;

public:
  mapped_grid_loop(const Grid& b, const Grid &e, const Grid &s) : base_t(b,e),
    grid_size_(s)
  {
    std::size_t sz = 1;
    for(short i=grid_size_.size()-1; i>=0; --i)
    {
      grid_step_[i] = sz;
      sz *= grid_size_[i];
      CCTBX_ASSERT( grid_step_[i] > 0 );
    }
    for(short i=grid_size_.size()-1; i>=0; --i)
    {
      if( i==0 )
        grid_delta_[i] = 0;
      else
      {
        grid_delta_[i] = -1+ grid_step_[i-1] -(end_[i]-begin_[i])*grid_step_[i];
      }
    }
    mapped_index_1d_ = this->mapped_index_1d(b);
  }

  bool advance()
  {
    for(short i = grid_step_.size()-1; i >= 0; --i)
    {
      current_[i]++;
      ++mapped_index_1d_;
      if (current_[i] < end_[i])
        return true;
      current_[i] = begin_[i];
      mapped_index_1d_ += grid_delta_[i];
    }
    over_ = true;
    return false;
  }

  std::size_t mapped_index_1d(const Grid& pos) const
  {
    std::size_t r=0;
    unsigned short i=0;
    for(; i<pos.size()-1; ++i)
      r += pos[i] * static_cast<std::size_t>( grid_step_[i] );
    return r + pos[i];
  }

  std::size_t mapped_index_1d() const
  {
    return this->mapped_index_1d_;
  }

private:
  Grid grid_size_, grid_step_, grid_delta_;
  std::size_t mapped_index_1d_;
};

namespace asu = cctbx::sgtbx::asu;

///// UNDER CONSTRUCTION
//  purporse: convert full unit_cell map into asymmetric unit sized map
//            convert processed aysmmetric map into form suitable for FFT
// motivation: performance improvment of various map algorithms
// code duplication: see atom_mask.h
class asymmetric_map
{
public:
  typedef scitbx::af::c_interval_grid<3> asu_grid_t;
  typedef scitbx::af::nested_loop<scitbx::int3> grid_iterator_t;
  typedef mapped_grid_loop<scitbx::int3> mapped_iterator_t;
  typedef scitbx::af::c_grid_padded<3> fft_grid_t;
  typedef scitbx::af::versa<double, fft_grid_t> fft_map_t;

  typedef scitbx::af::versa<double, asu_grid_t  > data_type;
  typedef scitbx::af::ref<double, asu_grid_t  >  data_ref_t;

  //! Creates asymmetric map from unit cell sized map
  /*! Unit cell sized map has to be symmetry expanded, for example
   * generated by FFT from hkl data
   */
  template<typename Grid>
  asymmetric_map(const sgtbx::space_group_type &group,
    scitbx::af::const_ref<double,Grid> cell_data) : asu_(group),
    optimized_asu_(asu_,cell_data.accessor().focus())
  {
    scitbx::int3 grid( cell_data.accessor().focus() ),
      padded_grid_size( cell_data.accessor().all() );
    this->copy_to_asu_box(grid, padded_grid_size, cell_data.begin());
  }

  //! Creates asymmetric map from unit cell sized map
  /*! Unit cell sized map has to be symmetry expanded, for example
   * generated by FFT from hkl data
   */
  asymmetric_map(const sgtbx::space_group_type &group,
    scitbx::af::const_ref<double, scitbx::af::flex_grid<> > cell_data)
    : asu_(group), optimized_asu_(asu_,adapt(cell_data.accessor().focus()))
  {
    scitbx::int3 grid( adapt(cell_data.accessor().focus()) ),
      padded_grid_size( adapt(cell_data.accessor().all()) );
    this->copy_to_asu_box(grid, padded_grid_size, cell_data.begin());
  }

#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
  //! Creates asymmetric map from asymmetric unit sized map
  /*! asu_data is invalid after this call */
  asymmetric_map(const sgtbx::space_group_type &group, data_type && asu_data,
    const scitbx::af::int3 &grid_size) : data_(std::move(asu_data)),
    asu_(group), optimized_asu_(asu_,grid_size)
  {
    //! @todo: test for grid_size and asu_data compatibility
  }
#endif

  //! Creates asymmetric map from asymmetric unit sized map
  /*! asu_data is invalid after this call */
  asymmetric_map(const sgtbx::space_group_type &group,
    scitbx::af::versa<double, scitbx::af::flex_grid<> > asu_data,
    const scitbx::af::int3 &grid_size) : asu_(group),
    optimized_asu_(asu_,grid_size)
  {
    //! @todo: test for grid_size and asu_data compatibility
    // const auto &acc = asu_data.accessor();
    const scitbx::af::flex_grid<> &acc =  asu_data.accessor();
    CCTBX_ASSERT( acc.nd() == 3U );
    asu_grid_t grid(af::adapt(acc.origin()), af::adapt(acc.last()));
    CCTBX_ASSERT( acc.size_1d() == grid.size_1d() );
#ifdef BOOST_NO_CXX11_RVALUE_REFERENCES
    data_ = data_type(asu_data.handle(), grid);
#else
    data_ = std::move(data_type(asu_data.handle(), grid));
#endif
  }

private:
  //! Hidden C++ style deep copy constructor
  asymmetric_map(const asymmetric_map &amap) : data_(amap.data_.deep_copy()),
    asu_(amap.asu_), optimized_asu_(amap.optimized_asu_)
  {}

public:
#ifndef BOOST_NO_CXX11_RVALUE_REFERENCES
  //! C++11 style shallow move constructor
  asymmetric_map(asymmetric_map &&amap) : data_(std::move(amap.data_)),
    asu_(std::move(amap.asu_)), optimized_asu_(std::move(amap.optimized_asu_))
  {}
#endif

  asymmetric_map explicit_copy() const
  {
    return asymmetric_map(*this);
  }

  //! Constant reference to asymmetric map data
  const data_type &data() const { return data_; }

  //! Modfiable reference to asymmetric map data, size can not be changed
  data_ref_t data_ref() { return data_.ref(); }

  //! Grid optimized asymmetric unit
  const asu::asymmetric_unit<asu::direct,asu::optimized> &optimized_asu() const
  {
    return optimized_asu_;
  }

  //! Creates space group, that has been used to create this map
  cctbx::sgtbx::space_group space_group() const
  {
    cctbx::sgtbx::space_group_symbols symbol("Hall: "+this->hall_symbol());
    cctbx::sgtbx::space_group group(symbol);
    CCTBX_ASSERT( group.type().hall_symbol() == this->hall_symbol() );
    return group;
  }

  //! Unit cell map size
  scitbx::int3 unit_cell_grid_size() const
  {
    return optimized_asu_.grid_size();
  }

  //! Beginning of the asymmetric box on the unit cell sized grid
  scitbx::int3 box_begin() const
  {
    return scitbx::int3(data_.accessor().origin());
  }

  //! End of the asymmetric box on the unit cell sized grid
  scitbx::int3 box_end() const
  {
    return scitbx::int3(data_.accessor().last());
  }

  grid_iterator_t grid_begin() const
  {
    return grid_iterator_t(this->box_begin(), this->box_end());
  }

  //! Unit cell sized gridding suitable for FFT
  fft_grid_t fft_grid() const
  {
    //! @todo: size compatible with space group for fft ?
    // is it guaranteed by asymmetric_unit ?
    scitbx::af::tiny<std::size_t,3> n_real = this->unit_cell_grid_size();
    return fft_grid_t(scitbx::fftpack::m_real_from_n_real(n_real), n_real);
  }

  //! Unit cell sized map suitable for fft
  fft_map_t map_for_fft() const;

  //! Structure factors from map
  scitbx::af::shared< std::complex<double> > structure_factors(
    scitbx::af::const_ref< cctbx::miller::index<> > indices) const;

  // not implemented, do I really need this one ?
  // unit_cell_map_t symmetry_expanded_unit_cell_map() const;

  enum format { xplor };

  //! Saves asymmetric map into xplor formatted file
  void save(const std::string &file_name, const uctbx::unit_cell &unit_cell,
    format f=xplor) const;

  //! Timings for profiling
  mutable std::string map_for_fft_times_, fill_density_times_, fill_fft_times_;

private:

  const std::string &hall_symbol() const
  {
    return asu_.hall_symbol;
  }

  std::vector<cctbx::sgtbx::grid_symop> grid_symops() const;

  void copy_to_asu_box(const scitbx::int3 &map_size,
    const scitbx::int3 &padded_map_size, const double *cell_data);

  mapped_iterator_t mapped_begin(const scitbx::int3 &grid_size) const
  {
    // open range : [begin, end)
    return mapped_iterator_t(this->box_begin(), this->box_end(), grid_size);
  }

  static scitbx::int3 adapt(const scitbx::af::flex_grid<>::index_type &f)
  {
    CCTBX_ASSERT( f.size()==3U );
    scitbx::int3 r(f[0],f[1],f[2]);
    return r;
  }

  data_type data_;
  asu::direct_space_asu asu_;
  asu::asymmetric_unit<asu::direct, asu::optimized> optimized_asu_;
};

}} // cctbx::maptbx
#endif
