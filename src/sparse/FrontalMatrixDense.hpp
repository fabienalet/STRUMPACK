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
#ifndef FRONTAL_MATRIX_DENSE_HPP
#define FRONTAL_MATRIX_DENSE_HPP

#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include <random>

#include "misc/TaskTimer.hpp"
#include "dense/BLASLAPACKWrapper.hpp"
#include "CompressedSparseMatrix.hpp"
#include "MatrixReordering.hpp"
#if defined(STRUMPACK_USE_MPI)
#include "ExtendAdd.hpp"
#endif

namespace strumpack {

  template<typename scalar_t,typename integer_t> class FrontalMatrixDense
    : public FrontalMatrix<scalar_t,integer_t> {
    using F_t = FrontalMatrix<scalar_t,integer_t>;
    using DenseM_t = DenseMatrix<scalar_t>;
    using DenseMW_t = DenseMatrixWrapper<scalar_t>;
    using SpMat_t = CompressedSparseMatrix<scalar_t,integer_t>;
#if defined(STRUMPACK_USE_MPI)
    using ExtAdd = ExtendAdd<scalar_t,integer_t>;
#endif

  public:
    FrontalMatrixDense
    (integer_t sep, integer_t sep_begin, integer_t sep_end,
     std::vector<integer_t>& upd);

    void release_work_memory() override { F22_.clear(); }
    void extend_add_to_dense
    (DenseM_t& paF11, DenseM_t& paF12, DenseM_t& paF21, DenseM_t& paF22,
     const F_t* p, int task_depth) override;

    void sample_CB
    (const SPOptions<scalar_t>& opts, const DenseM_t& R,
     DenseM_t& Sr, DenseM_t& Sc, F_t* pa, int task_depth) override;
    void sample_CB
    (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa,
     int task_depth=0) const override;

    void sample_CB_to_F11
    (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa,
     int task_depth=0) const override;
    void sample_CB_to_F12
    (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa,
     int task_depth=0) const override;
    void sample_CB_to_F21
    (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa,
     int task_depth=0) const override;
    void sample_CB_to_F22
    (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa,
     int task_depth=0) const override;

    void multifrontal_factorization
    (const SpMat_t& A, const SPOptions<scalar_t>& opts,
     int etree_level=0, int task_depth=0) override;

    void forward_multifrontal_solve
    (DenseM_t& b, DenseM_t* work, int etree_level=0,
     int task_depth=0) const override;
    void backward_multifrontal_solve
    (DenseM_t& y, DenseM_t* work, int etree_level=0,
     int task_depth=0) const override;

    void extract_CB_sub_matrix
    (const std::vector<std::size_t>& I, const std::vector<std::size_t>& J,
     DenseM_t& B, int task_depth) const override;

    std::string type() const override { return "FrontalMatrixDense"; }

#if defined(STRUMPACK_USE_MPI)
    void extend_add_copy_to_buffers
    (std::vector<std::vector<scalar_t>>& sbuf,
     const FrontalMatrixMPI<scalar_t,integer_t>* pa) const override {
      ExtAdd::extend_add_seq_copy_to_buffers(F22_, sbuf, pa, this);
    }
#endif

  private:
    DenseM_t F11_, F12_, F21_, F22_;
    std::vector<int> piv; // regular int because it is passed to BLAS

    FrontalMatrixDense(const FrontalMatrixDense&) = delete;
    FrontalMatrixDense& operator=(FrontalMatrixDense const&) = delete;

    void factor_phase1
    (const SpMat_t& A, const SPOptions<scalar_t>& opts,
     int etree_level, int task_depth);
    void factor_phase2
    (const SpMat_t& A, const SPOptions<scalar_t>& opts,
     int etree_level, int task_depth);

    void fwd_solve_phase2
    (DenseM_t& b, DenseM_t& bupd, int etree_level, int task_depth) const;
    void bwd_solve_phase1
    (DenseM_t& y, DenseM_t& yupd, int etree_level, int task_depth) const;

    using F_t::lchild_;
    using FrontalMatrix<scalar_t,integer_t>::rchild_;
    using FrontalMatrix<scalar_t,integer_t>::dim_sep;
    using FrontalMatrix<scalar_t,integer_t>::dim_upd;
  };

  template<typename scalar_t,typename integer_t>
  FrontalMatrixDense<scalar_t,integer_t>::FrontalMatrixDense
  (integer_t sep, integer_t sep_begin, integer_t sep_end,
   std::vector<integer_t>& upd)
    : F_t(nullptr, nullptr, sep, sep_begin, sep_end, upd) {}

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::extend_add_to_dense
  (DenseM_t& paF11, DenseM_t& paF12, DenseM_t& paF21, DenseM_t& paF22,
   const F_t* p, int task_depth) {
    const std::size_t pdsep = paF11.rows();
    const std::size_t dupd = dim_upd();
    std::size_t upd2sep;
    auto I = this->upd_to_parent(p, upd2sep);
#if defined(STRUMPACK_USE_OPENMP_TASKLOOP)
#pragma omp taskloop default(shared) grainsize(64)      \
  if(task_depth < params::task_recursion_cutoff_level)
#endif
    for (std::size_t c=0; c<dupd; c++) {
      auto pc = I[c];
      if (pc < pdsep) {
        for (std::size_t r=0; r<upd2sep; r++)
          paF11(I[r],pc) += F22_(r,c);
        for (std::size_t r=upd2sep; r<dupd; r++)
          paF21(I[r]-pdsep,pc) += F22_(r,c);
      } else {
        for (std::size_t r=0; r<upd2sep; r++)
          paF12(I[r],pc-pdsep) += F22_(r, c);
        for (std::size_t r=upd2sep; r<dupd; r++)
          paF22(I[r]-pdsep,pc-pdsep) += F22_(r,c);
      }
    }
    STRUMPACK_FLOPS((is_complex<scalar_t>()?2:1) * dupd * dupd);
    STRUMPACK_FULL_RANK_FLOPS((is_complex<scalar_t>()?2:1) * dupd * dupd);
    release_work_memory();
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::multifrontal_factorization
  (const SpMat_t& A, const SPOptions<scalar_t>& opts,
   int etree_level, int task_depth) {
    if (task_depth == 0) {
      // use tasking for children and for extend-add parallelism
#pragma omp parallel if(!omp_in_parallel()) default(shared)
#pragma omp single nowait
      factor_phase1(A, opts, etree_level, task_depth);
      // do not use tasking for blas/lapack parallelism (use system
      // blas threading!)
      factor_phase2(A, opts, etree_level, params::task_recursion_cutoff_level);
    } else {
      factor_phase1(A, opts, etree_level, task_depth);
      factor_phase2(A, opts, etree_level, task_depth);
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::factor_phase1
  (const SpMat_t& A, const SPOptions<scalar_t>& opts,
   int etree_level, int task_depth) {
    if (task_depth < params::task_recursion_cutoff_level) {
      if (lchild_)
#pragma omp task default(shared)                                        \
  final(task_depth >= params::task_recursion_cutoff_level-1) mergeable
        lchild_->multifrontal_factorization
          (A, opts, etree_level+1, task_depth+1);
      if (rchild_)
#pragma omp task default(shared)                                        \
  final(task_depth >= params::task_recursion_cutoff_level-1) mergeable
        rchild_->multifrontal_factorization
          (A, opts, etree_level+1, task_depth+1);
#pragma omp taskwait
    } else {
      if (lchild_)
        lchild_->multifrontal_factorization
          (A, opts, etree_level+1, task_depth);
      if (rchild_)
        rchild_->multifrontal_factorization
          (A, opts, etree_level+1, task_depth);
    }
    // TODO can we allocate the memory in one go??
    const auto dsep = dim_sep();
    const auto dupd = dim_upd();
    F11_ = DenseM_t(dsep, dsep); F11_.zero();
    F12_ = DenseM_t(dsep, dupd); F12_.zero();
    F21_ = DenseM_t(dupd, dsep); F21_.zero();
    A.extract_front
      (F11_, F12_, F21_, this->sep_begin_, this->sep_end_,
       this->upd_, task_depth);
    if (dupd) {
      F22_ = DenseM_t(dupd, dupd);
      F22_.zero();
    }
    if (lchild_)
      lchild_->extend_add_to_dense
        (F11_, F12_, F21_, F22_, this, task_depth);
    if (rchild_)
      rchild_->extend_add_to_dense
        (F11_, F12_, F21_, F22_, this, task_depth);
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::factor_phase2
  (const SpMat_t& A, const SPOptions<scalar_t>& opts,
   int etree_level, int task_depth) {
    if (dim_sep()) {
      piv = F11_.LU(task_depth);
      if (opts.replace_tiny_pivots()) {
        // TODO consider other values for thresh
        //  - sqrt(eps)*|A|_1 as in SuperLU ?
        auto thresh = blas::lamch<real_t>('E') * A.size();
        for (std::size_t i=0; i<F11_.rows(); i++)
          if (std::abs(F11_(i,i)) < thresh)
            F11_(i,i) = (std::real(F11_(i,i)) < 0) ? -thresh : thresh;
      }
      if (dim_upd()) {
        F12_.laswp(piv, true);
        trsm(Side::L, UpLo::L, Trans::N, Diag::U,
             scalar_t(1.), F11_, F12_, task_depth);
        trsm(Side::R, UpLo::U, Trans::N, Diag::N,
             scalar_t(1.), F11_, F21_, task_depth);
        gemm(Trans::N, Trans::N, scalar_t(-1.), F21_, F12_,
             scalar_t(1.), F22_, task_depth);
      }
    }
    STRUMPACK_FULL_RANK_FLOPS
      (LU_flops(F11_) +
       gemm_flops(Trans::N, Trans::N, scalar_t(-1.), F21_, F12_, scalar_t(1.)) +
       trsm_flops(Side::L, scalar_t(1.), F11_, F12_) +
       trsm_flops(Side::R, scalar_t(1.), F11_, F21_));
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::forward_multifrontal_solve
  (DenseM_t& b, DenseM_t* work, int etree_level, int task_depth) const {
    DenseMW_t bupd(dim_upd(), b.cols(), work[0], 0, 0);
    bupd.zero();
    if (task_depth == 0) {
      // tasking when calling the children
#pragma omp parallel if(!omp_in_parallel())
#pragma omp single nowait
      this->fwd_solve_phase1(b, bupd, work, etree_level, task_depth);
      // no tasking for the root node computations, use system blas threading!
      fwd_solve_phase2(b, bupd, etree_level, params::task_recursion_cutoff_level);
    } else {
      this->fwd_solve_phase1(b, bupd, work, etree_level, task_depth);
      fwd_solve_phase2(b, bupd, etree_level, task_depth);
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::fwd_solve_phase2
  (DenseM_t& b, DenseM_t& bupd, int etree_level, int task_depth) const {
    if (dim_sep()) {
      DenseMW_t bloc(dim_sep(), b.cols(), b, this->sep_begin_, 0);
      bloc.laswp(piv, true);
      if (b.cols() == 1) {
        trsv(UpLo::L, Trans::N, Diag::U, F11_, bloc, task_depth);
        if (dim_upd())
          gemv(Trans::N, scalar_t(-1.), F21_, bloc,
               scalar_t(1.), bupd, task_depth);
      } else {
        trsm(Side::L, UpLo::L, Trans::N, Diag::U,
             scalar_t(1.), F11_, bloc, task_depth);
        if (dim_upd())
          gemm(Trans::N, Trans::N, scalar_t(-1.), F21_, bloc,
               scalar_t(1.), bupd, task_depth);
      }
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::backward_multifrontal_solve
  (DenseM_t& y, DenseM_t* work, int etree_level, int task_depth) const {
    DenseMW_t yupd(dim_upd(), y.cols(), work[0], 0, 0);
    if (task_depth == 0) {
      // no tasking in blas routines, use system threaded blas instead
      bwd_solve_phase1
        (y, yupd, etree_level, params::task_recursion_cutoff_level);
#pragma omp parallel if(!omp_in_parallel())
#pragma omp single nowait
      // tasking when calling children
      this->bwd_solve_phase2(y, yupd, work, etree_level, task_depth);
    } else {
      bwd_solve_phase1(y, yupd, etree_level, task_depth);
      this->bwd_solve_phase2(y, yupd, work, etree_level, task_depth);
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::bwd_solve_phase1
  (DenseM_t& y, DenseM_t& yupd, int etree_level, int task_depth) const {
    if (dim_sep()) {
      DenseMW_t yloc(dim_sep(), y.cols(), y, this->sep_begin_, 0);
      if (y.cols() == 1) {
        if (dim_upd())
          gemv(Trans::N, scalar_t(-1.), F12_, yupd,
               scalar_t(1.), yloc, task_depth);
        trsv(UpLo::U, Trans::N, Diag::N, F11_, yloc, task_depth);
      } else {
        if (dim_upd())
          gemm(Trans::N, Trans::N, scalar_t(-1.), F12_, yupd,
               scalar_t(1.), yloc, task_depth);
        trsm(Side::L, UpLo::U, Trans::N, Diag::N, scalar_t(1.),
             F11_, yloc, task_depth);
      }
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::extract_CB_sub_matrix
  (const std::vector<std::size_t>& I, const std::vector<std::size_t>& J,
   DenseM_t& B, int task_depth) const {
    std::vector<std::size_t> lJ, oJ;
    this->find_upd_indices(J, lJ, oJ);
    if (lJ.empty()) return;
    std::vector<std::size_t> lI, oI;
    this->find_upd_indices(I, lI, oI);
    if (lI.empty()) return;
    for (std::size_t j=0; j<lJ.size(); j++)
      for (std::size_t i=0; i<lI.size(); i++)
        B(oI[i], oJ[j]) += F22_(lI[i], lJ[j]);
    STRUMPACK_FLOPS((is_complex<scalar_t>() ? 2 : 1) * lJ.size() * lI.size());
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB
  (const SPOptions<scalar_t>& opts, const DenseM_t& R, DenseM_t& Sr,
   DenseM_t& Sc, F_t* pa, int task_depth) {
    auto I = this->upd_to_parent(pa);
    auto cR = R.extract_rows(I);
    DenseM_t cS(dim_upd(), R.cols());
    TIMER_TIME(TaskType::F22_MULT, 1, t_f22mult);
    gemm(Trans::N, Trans::N, scalar_t(1.), F22_, cR,
         scalar_t(0.), cS, task_depth);
    TIMER_STOP(t_f22mult);
    Sr.scatter_rows_add(I, cS, task_depth);
    TIMER_TIME(TaskType::F22_MULT, 1, t_f22mult2);
    gemm(Trans::C, Trans::N, scalar_t(1.), F22_, cR,
         scalar_t(0.), cS, task_depth);
    TIMER_STOP(t_f22mult2);
    Sc.scatter_rows_add(I, cS, task_depth);
    STRUMPACK_CB_SAMPLE_FLOPS
      (gemm_flops(Trans::N, Trans::N, scalar_t(1.), F22_, cR, scalar_t(0.)) +
       gemm_flops(Trans::C, Trans::N, scalar_t(1.), F22_, cR, scalar_t(0.)) +
       cS.rows()*cS.cols()*2); // for the skinny-extend add
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB
  (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa, int task_depth) const {
    auto I = this->upd_to_parent(pa);
    auto cR = R.extract_rows(I);
    DenseM_t cS(dim_upd(), R.cols());
    TIMER_TIME(TaskType::F22_MULT, 1, t_f22mult);
    gemm(op, Trans::N, scalar_t(1.), F22_, cR,
         scalar_t(0.), cS, task_depth);
    TIMER_STOP(t_f22mult);
    S.scatter_rows_add(I, cS, task_depth);
    STRUMPACK_CB_SAMPLE_FLOPS
      (gemm_flops(op, Trans::N, scalar_t(1.), F22_, cR, scalar_t(0.)) +
       cS.rows()*cS.cols()); // for the skinny-extend add
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB_to_F11
  (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa, int task_depth) const {
    const std::size_t dupd = dim_upd();
    if (!dupd) return;
    std::size_t u2s;
    auto Ir = this->upd_to_parent(pa, u2s);
    auto Rcols = R.cols();
    DenseM_t cR(u2s, Rcols);
    for (std::size_t c=0; c<Rcols; c++)
      for (std::size_t r=0; r<u2s; r++)
        cR(r,c) = R(Ir[r],c);
    DenseM_t cS(u2s, Rcols);
    DenseMW_t CB11(u2s, u2s, const_cast<DenseM_t&>(F22_), 0, 0);
    gemm(op, Trans::N, scalar_t(1.), CB11, cR, scalar_t(0.), cS, task_depth);
    for (std::size_t c=0; c<Rcols; c++)
      for (std::size_t r=0; r<u2s; r++)
        S(Ir[r],c) += cS(r,c);
    STRUMPACK_CB_SAMPLE_FLOPS(u2s*Rcols);
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB_to_F12
  (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa, int task_depth) const {
    const std::size_t dupd = dim_upd();
    if (!dupd) return;
    std::size_t u2s;
    auto Ir = this->upd_to_parent(pa, u2s);
    auto pds = pa->dim_sep();
    auto Rcols = R.cols();
    DenseMW_t CB12(u2s, dupd-u2s, const_cast<DenseM_t&>(F22_), 0, u2s);
    if (op == Trans::N) {
      DenseM_t cR(dupd-u2s, Rcols);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=u2s; r<dupd; r++)
          cR(r-u2s,c) = R(Ir[r]-pds,c);
      DenseM_t cS(u2s, Rcols);
      gemm(op, Trans::N, scalar_t(1.), CB12, cR, scalar_t(0.), cS, task_depth);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=0; r<u2s; r++)
          S(Ir[r],c) += cS(r,c);
      STRUMPACK_CB_SAMPLE_FLOPS(u2s*Rcols);
    } else {
      DenseM_t cR(u2s, Rcols);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=0; r<u2s; r++)
          cR(r,c) = R(Ir[r],c);
      DenseM_t cS(dupd-u2s, Rcols);
      gemm(op, Trans::N, scalar_t(1.), CB12, cR, scalar_t(0.), cS, task_depth);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=u2s; r<dupd; r++)
          S(Ir[r]-pds,c) += cS(r-u2s,c);
      STRUMPACK_CB_SAMPLE_FLOPS((dupd-u2s)*Rcols);
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB_to_F21
  (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa, int task_depth) const {
    const std::size_t dupd = dim_upd();
    if (!dupd) return;
    std::size_t u2s;
    auto Ir = this->upd_to_parent(pa, u2s);
    auto Rcols = R.cols();
    auto pds = pa->dim_sep();
    DenseMW_t CB21(dupd-u2s, u2s, const_cast<DenseM_t&>(F22_), u2s, 0);
    if (op == Trans::N) {
      DenseM_t cR(u2s, Rcols);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=0; r<u2s; r++)
          cR(r,c) = R(Ir[r],c);
      DenseM_t cS(dupd-u2s, Rcols);
      gemm(op, Trans::N, scalar_t(1.), CB21, cR, scalar_t(0.), cS, task_depth);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=u2s; r<dupd; r++)
          S(Ir[r]-pds,c) += cS(r-u2s,c);
      STRUMPACK_CB_SAMPLE_FLOPS((dupd-u2s)*Rcols);
    } else {
      DenseM_t cR(dupd-u2s, Rcols);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=u2s; r<dupd; r++)
          cR(r-u2s,c) = R(Ir[r]-pds,c);
      DenseM_t cS(u2s, Rcols);
      gemm(op, Trans::N, scalar_t(1.), CB21, cR, scalar_t(0.), cS, task_depth);
      for (std::size_t c=0; c<Rcols; c++)
        for (std::size_t r=0; r<u2s; r++)
          S(Ir[r],c) += cS(r,c);
      STRUMPACK_CB_SAMPLE_FLOPS(u2s*Rcols);
    }
  }

  template<typename scalar_t,typename integer_t> void
  FrontalMatrixDense<scalar_t,integer_t>::sample_CB_to_F22
  (Trans op, const DenseM_t& R, DenseM_t& S, F_t* pa, int task_depth) const {
    const std::size_t dupd = dim_upd();
    if (!dupd) return;
    std::size_t u2s;
    auto Ir = this->upd_to_parent(pa, u2s);
    auto pds = pa->dim_sep();
    auto Rcols = R.cols();
    DenseM_t cR(dupd-u2s, Rcols);
    for (std::size_t c=0; c<Rcols; c++)
      for (std::size_t r=u2s; r<dupd; r++)
        cR(r-u2s,c) = R(Ir[r]-pds,c);
    DenseM_t cS(dupd-u2s, Rcols);
    DenseMW_t CB22(dupd-u2s, dupd-u2s, const_cast<DenseM_t&>(F22_), u2s, u2s);
    gemm(op, Trans::N, scalar_t(1.), CB22, cR, scalar_t(0.), cS, task_depth);
    for (std::size_t c=0; c<Rcols; c++)
      for (std::size_t r=u2s; r<dupd; r++)
        S(Ir[r]-pds,c) += cS(r-u2s,c);
    STRUMPACK_CB_SAMPLE_FLOPS((dupd-u2s)*Rcols);
  }

} // end namespace strumpack

#endif
