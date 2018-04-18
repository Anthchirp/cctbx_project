#ifndef SCITBX_EXAMPLES_BEVINGTON_STRUMPACK_H
#define SCITBX_EXAMPLES_BEVINGTON_STRUMPACK_H

#include <scitbx/array_family/shared.h>
#include <boost/python/tuple.hpp>
#include <scitbx/lstbx/normal_equations.h>
#include <scitbx/sparse/matrix.h>
#include <vector>
#include <Eigen/Sparse>
#include "StrumpackSparseSolver.hpp"

#include <iostream>
#include <fstream>

using std::size_t;

namespace scitbx{
namespace example{

/* Enforce a standard workflow from triplets to normal matrix to Cholesky factor:

Any time:
  n_parameters()
  reset()
  solved()
  formed_normal_matrix()

Upon instantiation, !solved(), !formed_normal_matrix():
  non_linear::add_equations()
  non_linear::add_equation_strumpack()
  non_linear::form_normal_matrix()--> set_from_triplets() + wipe_triplets()

After formation of eigen_normal_matrix, !solved():
  solve()
  normal_matrix()
  non_linear::add_constant_to_diagonal()
  non_linear::get_normal_matrix()
  non_linear::get_normal_matrix_diagonal()

After solution:
  solution()

Note: non_linear_ls_strumpack_wrapper *has a* linear_ls_strumpack_wrapper (not *is_a*)
*/

class linear_ls_strumpack_wrapper
  {

  public:
  ///Use Eigen sparse matrix format for the moment. STRUMPACK CSR matrix may be more suitable for future implementations
    typedef Eigen::SparseMatrix<double> sparse_matrix_t;

    /// Construct a least-squares problem with the given number of unknowns.
    linear_ls_strumpack_wrapper(int n_parameters)
      : solved_(false),
        formed_normal_matrix_(false),
        eigen_normal_matrix(n_parameters,n_parameters),
        scitbx_normal_matrix(0),
        right_hand_side_(n_parameters),
        solution_(n_parameters)
    {}

    /// Number of unknown parameters
    long n_parameters() const { return right_hand_side_.size(); }

    /// Reset the state to construction time, i.e. no equations accumulated
    void reset() {
      solved_ = false;
      formed_normal_matrix_ = false;
      eigen_normal_matrix = sparse_matrix_t(n_parameters(),n_parameters());
      std::fill(scitbx_normal_matrix.begin(), scitbx_normal_matrix.end(), double(0));
      std::fill(right_hand_side_.begin(), right_hand_side_.end(), double(0));
      std::fill(solution_.begin(), solution_.end(), double(0));
    }

    /// Only available if the equations have not been solved yet
    scitbx::af::shared<double>  right_hand_side() const {
      SCITBX_ASSERT(!solved());
      return right_hand_side_;
    }

    void solve() {
      SCITBX_ASSERT(formed_normal_matrix());
      int N = n_parameters();

      strumpack::StrumpackSparseSolver<double,int> spss(true,true); //verbose output on
      spss.options().set_reordering_method(strumpack::ReorderingStrategy::SCOTCH);
      //Previously used to build self adjoint sparse. A + A^T - diag(A)
/*      
      Eigen::SparseMatrix<double> eigen_normal_matrix_t (eigen_normal_matrix.transpose());
      double corner = eigen_normal_matrix_t.coeffRef(eigen_normal_matrix_t.rows()-1, 0 );
      double corner_t = eigen_normal_matrix_t.coeffRef(0, eigen_normal_matrix_t.cols()-1 );

      if(corner == corner_t){
        std::cout << "All is well: "<< corner << "   " << corner_t <<"\n";
      }
      else{
        std::cout << "All is not well: "<< corner << "   " << corner_t <<"\n";
      }
      
      Eigen::SparseMatrix<double> eigen_normal_matrix_full(eigen_normal_matrix);
      eigen_normal_matrix_full += eigen_normal_matrix_t;

      Eigen::VectorXd diag_vec(eigen_normal_matrix.diagonal());

      for(int d = 0; d < eigen_normal_matrix_t.rows(); ++d){
        eigen_normal_matrix_full.coeffRef(d,d) -= diag_vec(d);
      }

      corner = eigen_normal_matrix_full.coeffRef(eigen_normal_matrix_full.rows()-1, 0 );
      corner_t = eigen_normal_matrix_full.coeffRef(0, eigen_normal_matrix_full.cols()-1 );

      if(corner == corner_t){
        std::cout << "All is well: "<< corner << "   " << corner_t <<"\n";
      }
      else{
        std::cout << "All is not well: "<< corner << "   " << corner_t <<"\n";
      }
*/
      //Build full self adjoint sparse matrix from upper triangle data
      Eigen::SparseMatrix<double> eigen_normal_matrix_full = eigen_normal_matrix.selfadjointView<Eigen::Upper>();
/*    
      std::cout << "U_nnz=" << sam.nonZeros() << "\n";
      sam = eigen_normal_matrix.selfadjointView<Eigen::Lower>();
      std::cout << "L_nnz=" << sam.nonZeros() << "\n";
*/

      //Set the sparse matrix values by reading the EIGEN arrays directly
      spss.set_csr_matrix(eigen_normal_matrix_full.outerSize(),
                          eigen_normal_matrix_full.outerIndexPtr(),
                          eigen_normal_matrix_full.innerIndexPtr(),
                          eigen_normal_matrix_full.valuePtr(),true); //If matrix is symmetric false->true

//If the A matrix output is required -D_STRUMPACK_MATRIX_OUT_ compiler flag can be used
#ifdef _STRUMPACK_MATRIX_OUT_
      std::ofstream Amat, bvec, xvec;
      Amat.open ("A_strum.csv", std::ios::out | std::ios::app);
      bvec.open ("b_strum.csv", std::ios::out | std::ios::app);
      xvec.open ("x_strum.csv", std::ios::out | std::ios::app);
      //Output matrix as  row col value   format
      for (int k=0; k < eigen_normal_matrix_full.outerSize(); ++k){
        for (Eigen::SparseMatrix<double>::InnerIterator it(eigen_normal_matrix_full,k); it; ++it){
          Amat << "" << it.row() << "\t";
          Amat << it.col() << "\t";
          Amat << it.value() << std::endl;
        }
      }
      for (int kk=0; kk<eigen_normal_matrix_full.outerSize(); ++kk){
        bvec << *(right_hand_side_.begin() + kk) << "\n" ;
      }
      bvec << "\n";
      Amat.close();
      bvec.close();
      exit(-1); //Assuming only the first matrix is requested; exit afterwards
#endif
      //Create solution vector initialised initially to 0.
      scitbx::af::shared<double> x(eigen_normal_matrix_full.rows(),0.);

      spss.reorder();
      spss.factor();

      //Solve Ax=b, where b=right_hand_side_
      spss.solve(right_hand_side_.begin(), x.begin());

      //  XXX put the solution in the solution_ variable
      double* solnptr = solution_.begin();
      for (int i = 0; i < eigen_normal_matrix_full.rows(); ++i){
        *solnptr++ = x[i];
      }
      solved_ = true;
#ifdef _STRUMPACK_MATRIX_OUT_X_
      for (int kk=0; kk<eigen_normal_matrix_full.outerSize(); ++kk){
        xvec << x[kk] << "\n";
      }
      xvec << "\n\n";
      xvec.close();
      exit(-1); //Assuming only the first matrix is requested; exit afterwards
#endif
    }

    // Only available if the equations have not been solved yet
    scitbx::af::versa<double, scitbx::af::packed_u_accessor> normal_matrix() const {
      SCITBX_ASSERT(!solved());
      SCITBX_ASSERT(formed_normal_matrix());
      int N = n_parameters();

      scitbx::af::versa<double, scitbx::af::packed_u_accessor> result(n_parameters());
      double* ptr = result.begin();
      //SCITBX_EXAMINE(result.size());
      //SCITBX_EXAMINE(n_parameters());
      //SCITBX_EXAMINE(eigen_normal_matrix.nonZeros());
      // loop only thru non-zero elements to populate the result array.

      std::size_t offset_slow, offset_fast;
      for (int k=0; k < eigen_normal_matrix.outerSize(); ++k) { // column major, so outer (slow) means loop over column
        for (sparse_matrix_t::InnerIterator it(eigen_normal_matrix,k); it; ++it) {
          int irow = it.row();   // row index
          int icol = it.col();   // col index (here it is equal to k)
          offset_slow = N * irow - ( irow * (irow - 1) ) / 2;
          offset_fast = icol - irow;
          ptr[ offset_slow + offset_fast ] = it.value();
        }
      }
      return result;
    }

    void show_eigen_summary() const {
      SCITBX_ASSERT(formed_normal_matrix());
      long matsize = long(eigen_normal_matrix.cols()) * (eigen_normal_matrix.cols()+1)/2;
      printf("Number of parameters      %12ld\n",n_parameters());
      printf("Normal matrix square size %12ld\n",long(eigen_normal_matrix.cols()) * eigen_normal_matrix.cols());
      printf("Upper triangle size       %12ld\n",matsize);
      printf("Normal matrix non-zeros   %12ld, %6.2f%%\n",
              long(eigen_normal_matrix.nonZeros()),
              100. * long(eigen_normal_matrix.nonZeros())/double(matsize));
      Eigen::SimplicialLDLT<sparse_matrix_t> chol(eigen_normal_matrix.transpose());
      sparse_matrix_t lower = chol.matrixL();
      printf("Cholesky factor non-zeros %12ld, %6.2f%%\n",
              long(lower.nonZeros()),
              100. * long(lower.nonZeros())/double(matsize));
    }

    scitbx::af::shared<double> get_cholesky_diagonal() const {
      SCITBX_ASSERT (!solved_);
      SCITBX_ASSERT(formed_normal_matrix());
      int N = n_parameters();
      scitbx::af::shared<double> diagonal = scitbx::af::shared<double>(N);
      Eigen::SimplicialLDLT<sparse_matrix_t> chol(eigen_normal_matrix.transpose());
      Eigen::VectorXd diagonal_eigen = chol.vectorD();

      for (int k=0; k<N; ++k) { // column major, so outer (slow) means loop over column
        diagonal[k] = diagonal_eigen[k]; // copy over the diagonal into persistent array_family type
      }
      return diagonal;
    }

    scitbx::af::shared<double> get_cholesky_lower() const {
      SCITBX_ASSERT (!solved_);
      SCITBX_ASSERT(formed_normal_matrix());
      int N = n_parameters();
      scitbx::af::versa<double, scitbx::af::packed_l_accessor> triangular_result(n_parameters());
      Eigen::SimplicialLDLT<sparse_matrix_t> chol(eigen_normal_matrix.transpose());
      sparse_matrix_t lower = chol.matrixL();

      double* ptr = triangular_result.begin();
      for (int k=0; k<lower.outerSize(); ++k) { // column major, so outer (slow) means loop over column
        for (sparse_matrix_t::InnerIterator it(lower,k); it; ++it) {
          int irow = it.row();   // row index
          int icol = it.col();   // col index (here it is equal to k)
          std::size_t offset_slow = ( irow * irow +  irow )  / 2;
          std::size_t offset_fast = icol;
          ptr[ offset_slow + offset_fast ] = it.value();
        }
      }
      return triangular_result;
    }

    scitbx::af::shared<int> get_eigen_permutation_ordering() const {
      SCITBX_ASSERT (!solved_);
      SCITBX_ASSERT(formed_normal_matrix());
      int N = n_parameters();
      scitbx::af::shared<int> one_D_result(n_parameters());
      Eigen::SimplicialLDLT<sparse_matrix_t> chol(eigen_normal_matrix.transpose());
      for (int k=0; k<N; ++k){
         one_D_result[k] = chol.permutationP().indices()[k];
      }
      return one_D_result;
    }

    bool solved() const {
      return solved_;
    }

    bool formed_normal_matrix() const {
      return formed_normal_matrix_;
    }

    /// Only available after the equations have been solved
    scitbx::af::shared<double> solution() const {
      SCITBX_ASSERT(solved());
      return solution_;
    }

  public:
    bool solved_;
    bool formed_normal_matrix_;
    long last_computed_matrixL_nonZeros_;
    sparse_matrix_t eigen_normal_matrix;
    scitbx::af::ref_owning_versa<double, scitbx::af::packed_u_accessor>
                                                                 scitbx_normal_matrix;
    scitbx::af::shared<double> right_hand_side_;
    scitbx::af::shared<double> solution_;
  };


class non_linear_ls_strumpack_wrapper:  public scitbx::lstbx::normal_equations::non_linear_ls<double> {
  public:
    typedef Eigen::Triplet<double> triplet_t;
    typedef std::vector<triplet_t> triplet_list_t;

    non_linear_ls_strumpack_wrapper(int const& n_parameters):
      scitbx::lstbx::normal_equations::non_linear_ls<double>(0),
      strumpack_wrapper(n_parameters)
      {}

    inline void
    reset(){
      scitbx::lstbx::normal_equations::non_linear_ls<double>::reset();
      strumpack_wrapper.reset();
      tripletList = triplet_list_t();
    }

    linear_ls_strumpack_wrapper& step_equations() {
      return strumpack_wrapper;
    }

    /// Respecting encapsulation, add constant to all diagonal elements.
    void add_constant_to_diagonal(double const& increment) {
      form_normal_matrix();
      // loop only thru non-zero elements to update the eigen array.
      // column major, so outer (slow) means loop over column
      for (int k=0; k < strumpack_wrapper.eigen_normal_matrix.outerSize(); ++k) { // column major, so outer (slow) means loop over column
        for (linear_ls_strumpack_wrapper::sparse_matrix_t::InnerIterator it(strumpack_wrapper.eigen_normal_matrix,k);it;++it) {
          int irow = it.row();   // row index
          int icol = it.col();   // col index (here it is equal to k)
          if (irow!=icol){continue;}
          it.valueRef() = it.value() + increment;
        }
      }
    }

    /// get normal matrix
    scitbx::af::versa<double, scitbx::af::packed_u_accessor> get_normal_matrix() {
      SCITBX_ASSERT(!strumpack_wrapper.solved());
      form_normal_matrix();
      return strumpack_wrapper.normal_matrix();
    }

    /// get diagonal elements of the normal matrix
    scitbx::af::shared<double> get_normal_matrix_diagonal() {
      SCITBX_ASSERT(!strumpack_wrapper.solved());
      form_normal_matrix();
      int N = strumpack_wrapper.n_parameters();

      scitbx::af::shared<double> result(N, scitbx::af::init_functor_null<double>());
      double* ptr = result.begin();
      //SCITBX_EXAMINE(result.size());
      //SCITBX_EXAMINE(N);
      //SCITBX_EXAMINE(strumpack_wrapper.eigen_normal_matrix.nonZeros());
      for (int k=0; k < strumpack_wrapper.eigen_normal_matrix.outerSize(); ++k) { // column major, so outer (slow) means loop over column
        for (linear_ls_strumpack_wrapper::sparse_matrix_t::InnerIterator it(strumpack_wrapper.eigen_normal_matrix,k);it;++it) {
          int irow = it.row();   // row index
          int icol = it.col();   // col index (here it is equal to k)
          if (irow!=icol){continue;}
          ptr[ k ] = it.value();
        }
      }
      return result;
    }

    /// Add equations A x = b given a CCTBX-sparse matrix jacobian
    /** w[i] weights the i-th equation, i.e. the row \f$ A_{i.} \f$.
        The right_hand_side is negated, see lstbx/normal_equations.h
        Function intended for use within SparseLevMar algorithm of DIALS refinement.
     */
    inline
    void add_equations(af::const_ref<scalar_t> const &r,
                       sparse::matrix<scalar_t> const &jacobian,
                       af::const_ref<scalar_t> const &w)
    {
      typedef sparse::matrix<scalar_t>::row_iterator row_iterator;
      SCITBX_ASSERT(!strumpack_wrapper.formed_normal_matrix());
      SCITBX_ASSERT( r.size() == jacobian.n_rows()
                    && (!w.size() || r.size() == w.size()))
                   (r.size())(jacobian.n_rows())(w.size());
      SCITBX_ASSERT(jacobian.n_cols() == strumpack_wrapper.n_parameters())
                   (jacobian.n_cols())(strumpack_wrapper.n_parameters());
      add_residuals(r, w);
      sparse::matrix<scalar_t> a = jacobian.transpose();
      for (int ieqn = 0; ieqn < a.n_cols(); ++ieqn){
        int ndata = a.col(ieqn).non_zeroes();
        row_iterator iend = a.col(ieqn).end();
        for (row_iterator i = a.col(ieqn).begin(); i != iend; ++i)  {
          std::size_t idx_i = i.index();
          /* Need to skip over zero values, since the sparse::matrix Jacobian does
             not guarantee that it contains only non-zeroes.  For true sparse
             problems it creates a MemoryError to fill up the tripletList with
             > 10^9 entries.
          */
          if ( (*i)!=0. ){
            strumpack_wrapper.right_hand_side_[idx_i] -= w[ieqn] * (*i) * r[ieqn];

            for (row_iterator j=i; j != iend; ++j) {
              if ( (*j)!=0. ) {
                tripletList.push_back( triplet_t(idx_i, j.index(), w[ieqn] * (*i) * (*j)) );
                /*if (idx_i != j.index()){ //Build full sparse, not triangle. Symmtrically add i,j,v and j,i,v, using diagonal once
                  tripletList.push_back( triplet_t(j.index(), idx_i, w[ieqn] * (*i) * (*j)) );
                }*/
              }
            }
          }
        }
      }
    }

    /// Add the equation \f$ A_{i.} x = b_i \f$ with the given weight
    inline
    void add_equation_strumpack(double b_i,
                             scitbx::af::const_ref<std::size_t> const &row_idx,
                             scitbx::af::const_ref<double> const &row_data,
                             double w)
    {
      SCITBX_ASSERT(!strumpack_wrapper.formed_normal_matrix());
      int ndata = row_idx.size();
      for (int i=0; i<ndata; ++i)  {
        std::size_t idx_i = row_idx[i];
        strumpack_wrapper.right_hand_side_[idx_i] += w * row_data[i] * b_i;
        for (int j=0; j<ndata; ++j) {
          //push this term into the stack, later to be added to normal matrix
          tripletList.push_back( triplet_t(idx_i, row_idx[j], w * row_data[i] * row_data[j]) );
            /*if (idx_i != row_idx[j]){ //Build full sparse, not triangle. Symmtrically add i,j,v and j,i,v, using diagonal once
              tripletList.push_back( triplet_t(row_idx[j], idx_i, w * row_data[i] * row_data[j]) );
            }*/
        }
      }
    }

    inline void form_normal_matrix() {
      if (!strumpack_wrapper.formed_normal_matrix()){
        set_from_triplets();
        wipe_triplets();
      }
    }

    inline
    void set_from_triplets(){
      SCITBX_ASSERT(!strumpack_wrapper.formed_normal_matrix());
      strumpack_wrapper.eigen_normal_matrix.setFromTriplets(tripletList.begin(), tripletList.end());
      strumpack_wrapper.formed_normal_matrix_=true;
    }

    inline void wipe_triplets(){
      SCITBX_ASSERT(strumpack_wrapper.formed_normal_matrix());
      //critical to release this memory
      tripletList = triplet_list_t();
    }
    void show_eigen_summary(){
      form_normal_matrix();
      //strumpack_wrapper.show_eigen_summary();
    }
    scitbx::af::shared<double> get_cholesky_lower(){
      form_normal_matrix();
      return strumpack_wrapper.get_cholesky_lower();
    }
    scitbx::af::shared<double> get_cholesky_diagonal(){
      form_normal_matrix();
      return strumpack_wrapper.get_cholesky_diagonal();
    }
    scitbx::af::shared<int> get_eigen_permutation_ordering(){
      form_normal_matrix();
      return strumpack_wrapper.get_eigen_permutation_ordering();
    }
    bool solved() const{
      return strumpack_wrapper.solved();
    }
    long get_normal_matrix_ncols() const{
      return long(strumpack_wrapper.eigen_normal_matrix.cols());
    }
    long n_parameters() const{
      return long(strumpack_wrapper.n_parameters());
    }
    long get_normal_matrix_nnonZeros() const{
      return long(strumpack_wrapper.eigen_normal_matrix.nonZeros());
    }
    long get_lower_cholesky_nnonZeros() const{
      return long(strumpack_wrapper.last_computed_matrixL_nonZeros_);
    }

  public: /* data */
    linear_ls_strumpack_wrapper strumpack_wrapper;

  private:
    triplet_list_t tripletList;
};

class strumpack_base_class: public bevington_silver, public non_linear_ls_strumpack_wrapper {
  public:
    strumpack_base_class(int n_parameters):
      non_linear_ls_strumpack_wrapper(n_parameters)
      {
      }

    void access_cpp_build_up_directly_strumpack_eqn(bool objective_only, scitbx::af::shared<double> current_values) {

        vecd residuals = fvec_callable(current_values);
        if (objective_only){
          add_residuals(residuals.const_ref(), w_obs.const_ref());
          return;
        }

        // add one of the normal equations per each observation
        for (int ix = 0; ix < x_obs.size(); ++ix) {

          scitbx::af::shared<std::size_t> jacobian_one_row_indices;
          scitbx::af::shared<double> jacobian_one_row_data;

          jacobian_one_row_indices.push_back( 0 );
          jacobian_one_row_data.push_back( 1. );

          jacobian_one_row_indices.push_back( 1 );
          jacobian_one_row_data.push_back( std::exp( -x_obs[ix]/ current_values[3]) );

          jacobian_one_row_indices.push_back( 2 );
          jacobian_one_row_data.push_back( std::exp( -x_obs[ix]/ current_values[4]) );

          jacobian_one_row_indices.push_back( 3 );
          jacobian_one_row_data.push_back( current_values[1] * std::exp( -x_obs[ix]/ current_values[3]) *
                                           ( x_obs[ix] / (current_values[3]*current_values[3]) ));

          jacobian_one_row_indices.push_back( 4 );
          jacobian_one_row_data.push_back( current_values[2] * std::exp( -x_obs[ix]/ current_values[4]) *
                                           ( x_obs[ix] / (current_values[4]*current_values[4]) ));

          //add_equation(residuals[ix], jacobian_one_row.const_ref(), weights[ix]);
          add_residual(-residuals[ix], w_obs[ix]);
          add_equation_strumpack(residuals[ix], jacobian_one_row_indices.const_ref(), jacobian_one_row_data.const_ref(), w_obs[ix]);
        }
    }
};


}}

#endif // SCITBX_EXAMPLES_BEVINGTON_STRUMPACK_H