/*
 * STRUMPACK -- STRUctured Matrices PACKage, Copyright (c) 2014, The
 * Regents of the University of California, through Lawrence Berkeley
 * National Laboratory (subject to receipt of any required approvals
 * from the U.S. Dept. of Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE. This software is owned by the U.S. Department of Energy. As
 * such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * Developers: Pieter Ghysels, Francois-Henry Rouet, Xiaoye S. Li.
 *             (Lawrence Berkeley National Lab, Computational Research
 *             Division).
 *
 */
/**
 * \file HODLRMatrix.hpp
 * \brief Class wrapping around Yang Liu's HODLR code.
 */
#ifndef STRUMPACK_HODLR_MATRIX_HPP
#define STRUMPACK_HODLR_MATRIX_HPP

#include <cassert>
#include <algorithm>

#include "HSS/HSSPartitionTree.hpp"
#include "kernel/Kernel.hpp"
#include "clustering/Clustering.hpp"
#include "dense/DistributedMatrix.hpp"
#include "HODLROptions.hpp"
#include "HODLRWrapper.hpp"

namespace strumpack {

  /**
   * Code in this namespace is a wrapper aroung Yang Liu's Fortran
   * code:
   *    https://github.com/liuyangzhuan/ButterflyPACK
   */
  namespace HODLR {

    /**
     * \class HODLRMatrix
     *
     * \brief Hierarchically low-rank matrix representation.
     *
     * This requires MPI support.
     *
     * There are 3 different ways to create an HODLRMatrix
     *  - By specifying a matrix-(multiple)vector multiplication
     *    routine.
     *  - By specifying an element extraction routine.
     *  - By specifying a strumpack::kernel::Kernel matrix, defined by
     *    a collection of points and a kernel function.
     *
     * \tparam scalar_t Can be float, double, std:complex<float> or
     * std::complex<double>.
     *
     * \see HSS::HSSMatrix
     */
    template<typename scalar_t> class HODLRMatrix {
      using DenseM_t = DenseMatrix<scalar_t>;
      using DenseMW_t = DenseMatrixWrapper<scalar_t>;
      using DistM_t = DistributedMatrix<scalar_t>;
      using opts_t = HODLROptions<scalar_t>;
      using mult_t = typename std::function<
        void(Trans,const DenseM_t&,DenseM_t&)>;
      using elem_t = typename std::function<
        scalar_t(std::size_t i, std::size_t j)>;

    public:
      /**
       * Default constructor, makes an empty 0 x 0 matrix.
       */
      HODLRMatrix() {}

      /**
       * Construct an HODLR approximation for the kernel matrix K.
       *
       * \param c MPI communicator, this communicator is copied
       * internally.
       * \param K Kernel matrix object. The data associated with this
       * kernel will be permuted according to the clustering algorithm
       * selected by the HODLROptions objects.
       * \param perm Will be set to the permutation. This will be
       * resized.
       * \param opts object containing a number of HODLR options
       */
      HODLRMatrix
      (const MPIComm& c, kernel::Kernel<scalar_t>& K,
       std::vector<int>& perm, const opts_t& opts);

      /**
       * Construct an HODLR approximation using a routine to evaluate
       * individual matrix elements.
       *
       * \param c MPI communicator, this communicator is copied
       * internally.
       * \param t tree specifying the HODLR matrix partitioning
       * \param Aelem Routine, std::function, which can also be a
       * lambda function or a functor (class object implementing the
       * member "scalar_t operator()(int i, int j)"), that
       * evaluates/returns the matrix element A(i,j)
       * \param opts object containing a number of HODLR options
       */
      HODLRMatrix
      (const MPIComm& c, const HSS::HSSPartitionTree& tree,
       const std::function<scalar_t(int i, int j)>& Aelem,
       const opts_t& opts);

      /**
       * Construct an HODLR matrix using a specified HODLR tree and
       * matrix-vector multiplication routine.
       *
       * \param c MPI communicator, this communicator is copied
       * internally.
       * \param t tree specifying the HODLR matrix partitioning
       * \param Amult Routine for the matrix-vector product. Trans op
       * argument will be N, T or C for none, transpose or complex
       * conjugate. The const DenseM_t& argument is the the random
       * matrix R, and the final DenseM_t& argument S is what the user
       * routine should compute as A*R, A^t*R or A^c*R. S will already
       * be allocated.
       * \param opts object containing a number of options for HODLR
       * compression
       * \see compress, HODLROptions
       */
      HODLRMatrix
      (const MPIComm& c, const HSS::HSSPartitionTree& tree,
       const std::function<void(Trans op,const DenseM_t& R,DenseM_t& S)>& Amult,
       const opts_t& opts);

      /**
       * Construct an HODLR matrix using a specified HODLR tree. After
       * construction, the HODLR matrix will be empty, and can be filled
       * by calling one of the compress member routines.
       *
       * \param c MPI communicator, this communicator is copied
       * internally.
       * \param t tree specifying the HODLR matrix partitioning
       * \param opts object containing a number of options for HODLR
       * compression
       * \see compress, HODLROptions
       */
      HODLRMatrix
      (const MPIComm& c, const HSS::HSSPartitionTree& tree,
       const opts_t& opts);

      /**
       * Copy constructor is not supported.
       */
      HODLRMatrix(const HODLRMatrix<scalar_t>& h) = delete;

      /**
       * Move constructor.
       * \param h HODLRMatrix to move from, will be emptied.
       */
      HODLRMatrix(HODLRMatrix<scalar_t>&& h) { *this = h; }

      /**
       * Virtual destructor.
       */
      virtual ~HODLRMatrix();

      /**
       * Copy assignement operator is not supported.
       */
      HODLRMatrix<scalar_t>& operator=(const HODLRMatrix<scalar_t>& h) = delete;

      /**
       * Move assignment operator.
       * \param h HODLRMatrix to move from, will be emptied.
       */
      HODLRMatrix<scalar_t>& operator=(HODLRMatrix<scalar_t>&& h);

      /**
       * Return the number of rows in the matrix.
       * \return Global number of rows in the matrix.
       */
      std::size_t rows() const { return rows_; }
      /**
       * Return the number of columns in the matrix.
       * \return Global number of columns in the matrix.
       */
      std::size_t cols() const { return cols_; }
      /**
       * Return the number of local rows, owned by this process.
       * \return Number of local rows.
       */
      std::size_t lrows() const { return lrows_; }
      /**
       * Return the first row of the local rows owned by this process.
       * \return Return first local row
       */
      std::size_t begin_row() const { return dist_[c_.rank()]; }
      /**
       * Return last row (+1) of the local rows (begin_rows()+lrows())
       * \return Final local row (+1).
       */
      std::size_t end_row() const { return dist_[c_.rank()+1]; }
      /**
       * Return MPI communicator wrapper object.
       */
      const MPIComm& Comm() const { return c_; }

      double get_stat(const std::string& name) const {
        if (!stats_) return 0;
        return BPACK_get_stat<scalar_t>(stats_, name);
      }

      /**
       * Construct the compressed HODLR representation of the matrix,
       * using only a matrix-(multiple)vector multiplication routine.
       *
       * \param Amult Routine for the matrix-vector product. Trans op
       * argument will be N, T or C for none, transpose or complex
       * conjugate. The const DenseM_t& argument is the the random
       * matrix R, and the final DenseM_t& argument S is what the user
       * routine should compute as A*R, A^t*R or A^c*R. S will already
       * be allocated.
       */
      void compress
      (const std::function<void(Trans op,const DenseM_t& R,DenseM_t& S)>& Amult);

      /**
       * Construct the compressed HODLR representation of the matrix,
       * using only a matrix-(multiple)vector multiplication routine.
       *
       * \param Amult Routine for the matrix-vector product. Trans op
       * argument will be N, T or C for none, transpose or complex
       * conjugate. The const DenseM_t& argument is the the random
       * matrix R, and the final DenseM_t& argument S is what the user
       * routine should compute as A*R, A^t*R or A^c*R. S will already
       * be allocated.
       * \param rank_guess Initial guess for the rank
       */
      void compress
      (const std::function<void(Trans op,const DenseM_t& R,DenseM_t& S)>& Amult,
       int rank_guess);


      /**
       * Multiply this HODLR matrix with a dense matrix: Y = op(X),
       * where op can be none, transpose or complex conjugate. X and Y
       * are the local parts of block-row distributed matrices. The
       * number of rows in X and Y should correspond to the
       * distribution of this HODLR matrix.
       *
       * \param op Transpose, conjugate, or none.
       * \param X Right-hand side matrix. This is the local part of
       * the distributed matrix X. Should be X.rows() == this.lrows().
       * \param Y Result, should be Y.cols() == X.cols(), Y.rows() ==
       * this.lrows()
       * \see lrows, begin_row, end_row, mult
       */
      void mult(Trans op, const DenseM_t& X, DenseM_t& Y) const;

      /**
       * Multiply this HODLR matrix with a dense matrix: Y = op(X),
       * where op can be none, transpose or complex conjugate. X and Y
       * are in 2D block cyclic distribution.
       *
       * \param op Transpose, conjugate, or none.
       * \param X Right-hand side matrix. Should be X.rows() ==
       * this.rows().
       * \param Y Result, should be Y.cols() == X.cols(), Y.rows() ==
       * this.rows()
       * \see mult
       */
      void mult(Trans op, const DistM_t& X, DistM_t& Y) const;

      /**
       * Compute the factorization of this HODLR matrix. The matrix
       * can still be used for multiplication.
       *
       * \see solve, inv_mult
       */
      void factor();

      /**
       * Solve a system of linear equations A*X=B, with possibly
       * multiple right-hand sides.
       *
       * \param B Right hand side. This is the local part of
       * the distributed matrix B. Should be B.rows() == this.lrows().
       * \param X Result, should be X.cols() == B.cols(), X.rows() ==
       * this.lrows(). X should be allocated.
       * \see factor, lrows, begin_row, end_row, inv_mult
       */
      void solve(const DenseM_t& B, DenseM_t& X) const;

      /**
       * Solve a system of linear equations A*X=B, with possibly
       * multiple right-hand sides. X and B are in 2D block cyclic
       * distribution.
       *
       * \param B Right hand side. This is the local part of
       * the distributed matrix B. Should be B.rows() == this.rows().
       * \param X Result, should be X.cols() == B.cols(), X.rows() ==
       * this.rows(). X should be allocated.
       * \see factor, lrows, begin_row, end_row, inv_mult
       */
      void solve(const DistM_t& B, DistM_t& X) const;

      /**
       * Solve a system of linear equations op(A)*X=B, with possibly
       * multiple right-hand sides, where op can be none, transpose or
       * complex conjugate.
       *
       * \param B Right hand side. This is the local part of
       * the distributed matrix B. Should be B.rows() == this.lrows().
       * \param X Result, should be X.cols() == B.cols(), X.rows() ==
       * this.lrows(). X should be allocated.
       * \see factor, solve, lrows, begin_row, end_row
       */
      void inv_mult(Trans op, const DenseM_t& B, DenseM_t& X) const;

      DenseM_t redistribute_2D_to_1D(const DistM_t& R) const;
      void redistribute_2D_to_1D(const DistM_t& R2D, DenseM_t& R1D) const;
      void redistribute_1D_to_2D(const DenseM_t& S1D, DistM_t& S2D) const;

    private:
      F2Cptr ho_bf_ = nullptr;     // HODLR handle returned by Fortran code
      F2Cptr options_ = nullptr;   // options structure returned by Fortran code
      F2Cptr stats_ = nullptr;     // statistics structure returned by Fortran code
      F2Cptr msh_ = nullptr;       // mesh structure returned by Fortran code
      F2Cptr kerquant_ = nullptr;  // kernel quantities structure returned by Fortran code
      F2Cptr ptree_ = nullptr;     // process tree returned by Fortran code
      MPI_Fint Fcomm_;             // the fortran MPI communicator
      MPIComm c_;
      int rows_ = 0, cols_ = 0, lrows_ = 0;
      std::vector<int> perm_, iperm_; // permutation used by the HODLR code
      std::vector<int> dist_;         // begin rows of each rank

      template<typename S> friend class LRBFMatrix;
    };

    /**
     * Routine used to pass to the fortran code to compute a selected
     * element of a kernel. The kernel argument needs to be a pointer
     * to a strumpack::kernel object.
     *
     * \param i row coordinate of element to be computed from the
     * kernel
     * \param i column coordinate of element to be computed from the
     * kernel
     * \param v output, kernel value
     * \param kernel pointer to Kernel object
     */
    template<typename scalar_t> void HODLR_kernel_evaluation
    (int* i, int* j, scalar_t* v, C2Fptr kernel) {
      *v = static_cast<kernel::Kernel<scalar_t>*>(kernel)->eval(*i, *j);
    }

    template<typename scalar_t> void HODLR_element_evaluation
    (int* i, int* j, scalar_t* v, C2Fptr elem) {
      *v = static_cast<std::function<scalar_t(int,int)>*>
        (elem)->operator()(*i, *j);
    }

    template<typename scalar_t> HODLRMatrix<scalar_t>::HODLRMatrix
    (const MPIComm& c, kernel::Kernel<scalar_t>& K,
     std::vector<int>& perm, const opts_t& opts) {
      int d = K.d();
      rows_ = cols_ = K.n();

      auto tree = binary_tree_clustering
        (opts.clustering_algorithm(), K.data(), perm, opts.leaf_size());
      int min_lvl = 2 + std::ceil(std::log2(c.size()));
      int lvls = std::max(min_lvl, tree.levels());
      tree.expand_complete_levels(lvls);
      auto leafs = tree.leaf_sizes();

      c_ = c;
      Fcomm_ = MPI_Comm_c2f(c_.comm());
      int P = c_.size();
      int rank = c_.rank();

      std::vector<int> groups(P);
      std::iota(groups.begin(), groups.end(), 0);

      // create hodlr data structures
      HODLR_createptree<scalar_t>(P, groups.data(), Fcomm_, ptree_);
      HODLR_createoptions<scalar_t>(options_);
      HODLR_createstats<scalar_t>(stats_);

      // set hodlr options
      int com_opt = 4;   // compression option 1:SVD 2:RRQR 3:ACA 4:BACA
      int sort_opt = 0;  // 0:natural order, 1:kd-tree, 2:cobble-like ordering
                         // 3:gram distance-based cobble-like ordering
      HODLR_set_D_option<scalar_t>(options_, "tol_comp", opts.rel_tol());
      HODLR_set_I_option<scalar_t>(options_, "verbosity", opts.verbose() ? 0 : -1);
      HODLR_set_I_option<scalar_t>(options_, "nogeo", 1);
      //HODLR_set_I_option<scalar_t>(options_, "Nmin_leaf", opts.leaf_size());
      HODLR_set_I_option<scalar_t>(options_, "Nmin_leaf", rows_);
      HODLR_set_I_option<scalar_t>(options_, "RecLR_leaf", com_opt);
      HODLR_set_I_option<scalar_t>(options_, "xyzsort", sort_opt);
      HODLR_set_I_option<scalar_t>(options_, "ErrFillFull", 0);
      HODLR_set_I_option<scalar_t>(options_, "BACA_Batch", 100);
      HODLR_set_I_option<scalar_t>(options_, "rank0", opts.rank_guess());
      HODLR_set_D_option<scalar_t>(options_, "rankrate", opts.rank_rate());
      if (opts.butterfly_levels() > 0)
        HODLR_set_I_option<scalar_t>(options_, "LRlevel", opts.butterfly_levels());
      HODLR_set_D_option<scalar_t>(options_, "tol_comp", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_rand", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_Rdetect", 0.1*opts.rel_tol());

      perm_.resize(rows_);
      // construct HODLR with geometrical points
      HODLR_construct_element
        (rows_, d, K.data().data(), lvls-1, leafs.data(),
         perm_.data(), lrows_, ho_bf_, options_, stats_, msh_, kerquant_,
         ptree_, &(HODLR_kernel_evaluation<scalar_t>), &K, Fcomm_);

      //---- NOT NECESSARY ?? --------------------------------------------
      iperm_.resize(rows_);
      for (auto& i : perm_) i--; // Fortran to C
      MPI_Bcast(perm_.data(), perm_.size(), MPI_INT, 0, c_.comm());
      for (int i=0; i<rows_; i++)
        iperm_[perm_[i]] = i;
      //------------------------------------------------------------------

      dist_.resize(P+1);
      dist_[rank+1] = lrows_;
      MPI_Allgather
        (MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
         dist_.data()+1, 1, MPI_INT, c_.comm());
      for (int p=0; p<P; p++)
        dist_[p+1] += dist_[p];
    }


    template<typename scalar_t> HODLRMatrix<scalar_t>::HODLRMatrix
    (const MPIComm& c, const HSS::HSSPartitionTree& tree,
     const std::function<scalar_t(int i, int j)>& Aelem,
     const opts_t& opts) {
      rows_ = cols_ = tree.size;

      HSS::HSSPartitionTree full_tree(tree);
      int min_lvl = 2 + std::ceil(std::log2(c.size()));
      int lvls = std::max(min_lvl, full_tree.levels());
      full_tree.expand_complete_levels(lvls);
      auto leafs = full_tree.leaf_sizes();

      c_ = c;
      Fcomm_ = MPI_Comm_c2f(c_.comm());
      int P = c_.size();
      int rank = c_.rank();

      std::vector<int> groups(P);
      std::iota(groups.begin(), groups.end(), 0);

      // create hodlr data structures
      HODLR_createptree<scalar_t>(P, groups.data(), Fcomm_, ptree_);
      HODLR_createoptions<scalar_t>(options_);
      HODLR_createstats<scalar_t>(stats_);

      // set hodlr options
      int com_opt = 4;   // compression option 1:SVD 2:RRQR 3:ACA 4:BACA
      int sort_opt = 0;  // 0:natural order, 1:kd-tree, 2:cobble-like ordering
                         // 3:gram distance-based cobble-like ordering
      HODLR_set_D_option<scalar_t>(options_, "tol_comp", opts.rel_tol());
      HODLR_set_I_option<scalar_t>(options_, "verbosity", opts.verbose() ? 0 : -1);
      HODLR_set_I_option<scalar_t>(options_, "nogeo", 1);
      HODLR_set_I_option<scalar_t>(options_, "Nmin_leaf", rows_);
      HODLR_set_I_option<scalar_t>(options_, "RecLR_leaf", com_opt);
      HODLR_set_I_option<scalar_t>(options_, "xyzsort", sort_opt);
      HODLR_set_I_option<scalar_t>(options_, "ErrFillFull", 0);
      HODLR_set_I_option<scalar_t>(options_, "BACA_Batch", 100);
      HODLR_set_I_option<scalar_t>(options_, "rank0", opts.rank_guess());
      HODLR_set_D_option<scalar_t>(options_, "rankrate", opts.rank_rate());
      if (opts.butterfly_levels() > 0)
        HODLR_set_I_option<scalar_t>(options_, "LRlevel", opts.butterfly_levels());
      HODLR_set_D_option<scalar_t>(options_, "tol_comp", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_rand", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_Rdetect", 0.1*opts.rel_tol());

      perm_.resize(rows_);
      HODLR_construct_element
        (rows_, 0, NULL, lvls-1, leafs.data(),
         perm_.data(), lrows_, ho_bf_, options_, stats_, msh_, kerquant_,
         ptree_, &(HODLR_element_evaluation<scalar_t>), &Aelem, Fcomm_);

      //---- NOT NECESSARY ?? --------------------------------------------
      iperm_.resize(rows_);
      for (auto& i : perm_) i--; // Fortran to C
      MPI_Bcast(perm_.data(), perm_.size(), MPI_INT, 0, c_.comm());
      for (int i=0; i<rows_; i++)
        iperm_[perm_[i]] = i;
      //------------------------------------------------------------------

      dist_.resize(P+1);
      dist_[rank+1] = lrows_;
      MPI_Allgather
        (MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
         dist_.data()+1, 1, MPI_INT, c_.comm());
      for (int p=0; p<P; p++)
        dist_[p+1] += dist_[p];
    }

    template<typename scalar_t> HODLRMatrix<scalar_t>::HODLRMatrix
    (const MPIComm& c, const HSS::HSSPartitionTree& tree,
     const mult_t& Amult, const opts_t& opts)
      : HODLRMatrix<scalar_t>(c, tree, opts) {
      compress(Amult);
    }

    template<typename scalar_t> HODLRMatrix<scalar_t>::HODLRMatrix
    (const MPIComm& c, const HSS::HSSPartitionTree& tree,
     const opts_t& opts) {
      rows_ = cols_ = tree.size;

      HSS::HSSPartitionTree full_tree(tree);
      int min_lvl = 2 + std::ceil(std::log2(c.size()));
      int lvls = std::max(min_lvl, full_tree.levels());
      full_tree.expand_complete_levels(lvls);
      auto leafs = full_tree.leaf_sizes();

      c_ = c;
      if (c_.is_null()) return;
      Fcomm_ = MPI_Comm_c2f(c_.comm());
      int P = c_.size();
      int rank = c_.rank();

      std::vector<int> groups(P);
      std::iota(groups.begin(), groups.end(), 0);

      // create hodlr data structures
      HODLR_createptree<scalar_t>(P, groups.data(), Fcomm_, ptree_);
      HODLR_createoptions<scalar_t>(options_);
      HODLR_createstats<scalar_t>(stats_);

      // set hodlr options
      int com_opt = 2;   // compression option 1:SVD 2:RRQR 3:ACA 4:BACA
      int sort_opt = 0;  // 0:natural order, 1:kd-tree, 2:cobble-like ordering
                         // 3:gram distance-based cobble-like ordering
      HODLR_set_I_option<scalar_t>(options_, "verbosity", opts.verbose() ? 2 : -1);
      HODLR_set_I_option<scalar_t>(options_, "nogeo", 1);
      HODLR_set_I_option<scalar_t>(options_, "Nmin_leaf", rows_);
      //HODLR_set_I_option<scalar_t>(options_, "RecLR_leaf", com_opt);
      HODLR_set_I_option<scalar_t>(options_, "xyzsort", sort_opt);
      HODLR_set_I_option<scalar_t>(options_, "ErrFillFull", 0);
      HODLR_set_I_option<scalar_t>(options_, "BACA_Batch", 100);
      HODLR_set_I_option<scalar_t>(options_, "rank0", opts.rank_guess());
      HODLR_set_D_option<scalar_t>(options_, "rankrate", opts.rank_rate());
      if (opts.butterfly_levels() > 0)
        HODLR_set_I_option<scalar_t>(options_, "LRlevel", opts.butterfly_levels());
      HODLR_set_D_option<scalar_t>(options_, "tol_comp", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_rand", opts.rel_tol());
      HODLR_set_D_option<scalar_t>(options_, "tol_Rdetect", 0.1*opts.rel_tol());

      perm_.resize(rows_);
      HODLR_construct_matvec_init<scalar_t>
        (rows_, lvls-1, leafs.data(), perm_.data(), lrows_, ho_bf_, options_,
         stats_, msh_, kerquant_, ptree_);

      //---- NOT NECESSARY ?? --------------------------------------------
      iperm_.resize(rows_);
      for (auto& i : perm_) i--; // Fortran to C
      MPI_Bcast(perm_.data(), perm_.size(), MPI_INT, 0, c_.comm());
      for (int i=0; i<rows_; i++)
        iperm_[perm_[i]] = i;
      //------------------------------------------------------------------

      dist_.resize(P+1);
      dist_[rank+1] = lrows_;
      MPI_Allgather
        (MPI_IN_PLACE, 0, MPI_DATATYPE_NULL,
         dist_.data()+1, 1, MPI_INT, c_.comm());
      for (int p=0; p<P; p++)
        dist_[p+1] += dist_[p];
    }

    template<typename scalar_t> HODLRMatrix<scalar_t>::~HODLRMatrix() {
      if (stats_) HODLR_deletestats<scalar_t>(stats_);
      if (ptree_) HODLR_deleteproctree<scalar_t>(ptree_);
      if (msh_) HODLR_deletemesh<scalar_t>(msh_);
      if (kerquant_) HODLR_deletekernelquant<scalar_t>(kerquant_);
      if (ho_bf_) HODLR_delete<scalar_t>(ho_bf_);
      if (options_) HODLR_deleteoptions<scalar_t>(options_);
    }

    template<typename scalar_t> HODLRMatrix<scalar_t>&
    HODLRMatrix<scalar_t>::operator=(HODLRMatrix<scalar_t>&& h) {
      ho_bf_ = h.ho_bf_;       h.ho_bf_ = nullptr;
      options_ = h.options_;   h.options_ = nullptr;
      stats_ = h.stats_;       h.stats_ = nullptr;
      msh_ = h.msh_;           h.msh_ = nullptr;
      kerquant_ = h.kerquant_; h.kerquant_ = nullptr;
      ptree_ = h.ptree_;       h.ptree_ = nullptr;
      Fcomm_ = h.Fcomm_;
      c_ = h.c_;
      rows_ = h.rows_;
      cols_ = h.cols_;
      lrows_ = h.lrows_;
      std::swap(perm_, h.perm_);
      std::swap(iperm_, h.iperm_);
      std::swap(dist_, h.dist_);
      return *this;
    }

    template<typename scalar_t> void HODLR_matvec_routine
    (const char* op, int* nin, int* nout, int* nvec,
     const scalar_t* X, scalar_t* Y, C2Fptr func) {
      auto A = static_cast<std::function<
        void(Trans,const DenseMatrix<scalar_t>&,
             DenseMatrix<scalar_t>&)>*>(func);
      DenseMatrixWrapper<scalar_t> Yw(*nout, *nvec, Y, *nout),
        Xw(*nin, *nvec, const_cast<scalar_t*>(X), *nin);
      (*A)(c2T(*op), Xw, Yw);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::compress(const mult_t& Amult) {
      if (c_.is_null()) return;
      C2Fptr f = static_cast<void*>(const_cast<mult_t*>(&Amult));
      HODLR_construct_matvec_compute
        (ho_bf_, options_, stats_, msh_, kerquant_, ptree_,
         &(HODLR_matvec_routine<scalar_t>), f);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::compress
    (const mult_t& Amult, int rank_guess) {
      HODLR_set_I_option<scalar_t>(options_, "rank0", rank_guess);
      compress(Amult);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::mult
    (Trans op, const DenseM_t& X, DenseM_t& Y) const {
      if (c_.is_null()) return;
      HODLR_mult(char(op), X.data(), Y.data(), lrows_, lrows_, X.cols(),
                 ho_bf_, options_, stats_, ptree_);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::mult
    (Trans op, const DistM_t& X, DistM_t& Y) const {
      if (c_.is_null()) return;
      DenseM_t Y1D(lrows_, X.cols());
      {
        auto X1D = redistribute_2D_to_1D(X);
        HODLR_mult(char(op), X1D.data(), Y1D.data(), lrows_, lrows_,
                   X.cols(), ho_bf_, options_, stats_, ptree_);
      }
      redistribute_1D_to_2D(Y1D, Y);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::inv_mult
    (Trans op, const DenseM_t& X, DenseM_t& Y) const {
      if (c_.is_null()) return;
      HODLR_inv_mult
        (char(op), X.data(), Y.data(), lrows_, lrows_, X.cols(),
         ho_bf_, options_, stats_, ptree_);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::factor() {
      if (c_.is_null()) return;
      HODLR_factor<scalar_t>(ho_bf_, options_, stats_, ptree_, msh_);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::solve(const DenseM_t& B, DenseM_t& X) const {
      if (c_.is_null()) return;
      HODLR_solve(X.data(), B.data(), lrows_, X.cols(),
                  ho_bf_, options_, stats_, ptree_);
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::solve(const DistM_t& B, DistM_t& X) const {
      if (c_.is_null()) return;
      DenseM_t X1D(lrows_, X.cols());
      {
        auto B1D = redistribute_2D_to_1D(B);
        HODLR_solve(X1D.data(), B1D.data(), lrows_, X.cols(),
                    ho_bf_, options_, stats_, ptree_);
      }
      redistribute_1D_to_2D(X1D, X);
    }


    template<typename scalar_t> DenseMatrix<scalar_t>
    HODLRMatrix<scalar_t>::redistribute_2D_to_1D(const DistM_t& R2D) const {
      DenseM_t R1D(lrows_, R2D.cols());
      redistribute_2D_to_1D(R2D, R1D);
      return R1D;
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::redistribute_2D_to_1D
    (const DistM_t& R2D, DenseM_t& R1D) const {
      TIMER_TIME(TaskType::REDIST_2D_TO_HSS, 0, t_redist);
      if (c_.is_null()) return;
      const auto P = c_.size();
      const auto rank = c_.rank();
      // for (int p=0; p<P; p++)
      //   copy(dist_[rank+1]-dist_[rank], R2D.cols(), R2D, dist_[rank], 0,
      //        R1D, p, R2D.grid()->ctxt_all());
      // return;
      const auto Rcols = R2D.cols();
      int R2Drlo, R2Drhi, R2Dclo, R2Dchi;
      R2D.lranges(R2Drlo, R2Drhi, R2Dclo, R2Dchi);
      const auto Rlcols = R2Dchi - R2Dclo;
      const auto Rlrows = R2Drhi - R2Drlo;
      const auto nprows = R2D.nprows();
      const auto B = DistM_t::default_MB;
      std::vector<std::vector<scalar_t>> sbuf(P);
      if (R2D.active()) {
        // global, local, proc
        std::vector<std::tuple<int,int,int>> glp(Rlrows);
        {
          std::vector<std::size_t> count(P);
          for (int r=R2Drlo; r<R2Drhi; r++) {
            auto gr = perm_[R2D.rowl2g(r)];
            auto p = -1 + std::distance
              (dist_.begin(), std::upper_bound
               (dist_.begin(), dist_.end(), gr));
            glp[r-R2Drlo] = std::tuple<int,int,int>{gr, r, p};
            count[p] += Rlcols;
          }
          std::sort(glp.begin(), glp.end());
          for (int p=0; p<P; p++)
            sbuf[p].reserve(count[p]);
        }
        for (int r=R2Drlo; r<R2Drhi; r++)
          for (int c=R2Dclo, lr=std::get<1>(glp[r-R2Drlo]),
                 p=std::get<2>(glp[r-R2Drlo]); c<R2Dchi; c++)
            sbuf[p].push_back(R2D(lr,c));
      }
      std::vector<scalar_t> rbuf;
      std::vector<scalar_t*> pbuf;
      c_.all_to_all_v(sbuf, rbuf, pbuf);
      assert(R1D.rows() == lrows_ && R1D.cols() == Rcols);
      if (lrows_) {
        std::vector<int> src_c(Rcols);
        for (int c=0; c<Rcols; c++)
          src_c[c] = R2D.colg2p_fixed(c)*nprows;
        for (int r=0; r<lrows_; r++) {
          auto gr = perm_[r + dist_[rank]];
          auto src_r = R2D.rowg2p_fixed(gr);
          for (int c=0; c<Rcols; c++)
            R1D(r, c) = *(pbuf[src_r + src_c[c]]++);
        }
      }
    }

    template<typename scalar_t> void
    HODLRMatrix<scalar_t>::redistribute_1D_to_2D
    (const DenseM_t& S1D, DistM_t& S2D) const {
      TIMER_TIME(TaskType::REDIST_2D_TO_HSS, 0, t_redist);
      if (c_.is_null()) return;
      const auto rank = c_.rank();
      const auto P = c_.size();
      const auto B = DistM_t::default_MB;
      const auto cols = S1D.cols();
      int S2Drlo, S2Drhi, S2Dclo, S2Dchi;
      S2D.lranges(S2Drlo, S2Drhi, S2Dclo, S2Dchi);
      const auto nprows = S2D.nprows();
      std::vector<std::vector<scalar_t>> sbuf(P);
      assert(S1D.rows() == lrows_);
      assert(S1D.rows() == dist_[rank+1] - dist_[rank]);
      if (lrows_) {
        std::vector<std::tuple<int,int,int>> glp(lrows_);
        for (int r=0; r<lrows_; r++) {
          auto gr = iperm_[r + dist_[rank]];
          // assert(gr == r + dist_[rank]);
          assert(gr >= 0 && gr < S2D.rows());
          glp[r] = std::tuple<int,int,int>{gr,r,S2D.rowg2p_fixed(gr)};
        }
        std::sort(glp.begin(), glp.end());
        std::vector<int> pc(cols);
        for (int c=0; c<cols; c++)
          pc[c] = S2D.colg2p_fixed(c)*nprows;
        {
          std::vector<std::size_t> count(P);
          for (int r=0; r<lrows_; r++)
            for (int c=0, pr=std::get<2>(glp[r]); c<cols; c++)
              count[pr+pc[c]]++;
          for (int p=0; p<P; p++)
            sbuf[p].reserve(count[p]);
        }
        for (int r=0; r<lrows_; r++)
          for (int c=0, lr=std::get<1>(glp[r]),
                 pr=std::get<2>(glp[r]); c<cols; c++)
            sbuf[pr+pc[c]].push_back(S1D(lr,c));
      }
      std::vector<scalar_t> rbuf;
      std::vector<scalar_t*> pbuf;
      c_.all_to_all_v(sbuf, rbuf, pbuf);
      if (S2D.active()) {
        for (int r=S2Drlo; r<S2Drhi; r++) {
          auto gr = perm_[S2D.rowl2g(r)];
          assert(gr == S2D.rowl2g(r));
          auto p = -1 + std::distance
            (dist_.begin(), std::upper_bound(dist_.begin(), dist_.end(), gr));
          assert(p < P && p >= 0);
          for (int c=S2Dclo; c<S2Dchi; c++) {
            auto tmp = *(pbuf[p]++);
            S2D(r,c) = tmp;
          }
        }
      }
    }

  } // end namespace HODLR
} // end namespace strumpack

#endif // STRUMPACK_HODLR_MATRIX_HPP
