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
 */
#ifndef HSS_MATRIX_COMPRESS_KERNEL_HPP
#define HSS_MATRIX_COMPRESS_KERNEL_HPP

#include "misc/RandomWrapper.hpp"
#include "misc/Tools.hpp"

namespace strumpack {
  namespace HSS {

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_ann
    (DenseMatrix<std::size_t>& ann, DenseM_t& scores,
     const elem_t& Aelem, const opts_t& opts) {
      std::cout << "---> USING COMPRESS_ANN <---" << std::endl;
      WorkCompressANN<scalar_t> w;
#pragma omp parallel if(!omp_in_parallel())
#pragma omp single nowait
      compress_recursive_ann
        (ann, scores, Aelem, opts, w, this->_openmp_task_depth);
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compress_recursive_ann
    (DenseMatrix<std::size_t>& ann, DenseM_t& scores, const elem_t& Aelem,
     const opts_t& opts, WorkCompressANN<scalar_t>& w, int depth) {
      if (this->leaf()) {
        if (this->is_untouched()) {
          std::vector<std::size_t> I, J;
          I.reserve(this->rows());
          J.reserve(this->cols());
          for (std::size_t i=0; i<this->rows(); i++)
            I.push_back(i+w.offset.first);
          for (std::size_t j=0; j<this->cols(); j++)
            J.push_back(j+w.offset.second);
          _D = DenseM_t(this->rows(), this->cols());
          Aelem(I, J, _D);
        }
      } else {
        w.split(this->_ch[0]->dims());
        bool tasked = depth < params::task_recursion_cutoff_level;
        if (tasked) {
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          this->_ch[0]->compress_recursive_ann
            (ann, scores, Aelem, opts, w.c[0], depth+1);
#pragma omp task default(shared)                                        \
  final(depth >= params::task_recursion_cutoff_level-1) mergeable
          this->_ch[1]->compress_recursive_ann
            (ann, scores, Aelem, opts, w.c[1], depth+1);
#pragma omp taskwait
        } else {
          this->_ch[0]->compress_recursive_ann
            (ann, scores, Aelem, opts, w.c[0], depth);
          this->_ch[1]->compress_recursive_ann
            (ann, scores, Aelem, opts, w.c[1], depth);
        }
        if (!this->_ch[0]->is_compressed() ||
            !this->_ch[1]->is_compressed())
          return;
        if (this->is_untouched()) {
          _B01 = DenseM_t(this->_ch[0]->U_rank(), this->_ch[1]->V_rank());
          Aelem(w.c[0].Ir, w.c[1].Ic, _B01);
          _B10 = DenseM_t(this->_ch[1]->U_rank(), this->_ch[0]->V_rank());
          Aelem(w.c[1].Ir, w.c[0].Ic, _B10);
        }
      }
      if (w.lvl == 0)
        this->_U_state = this->_V_state = State::COMPRESSED;
      else {
        compute_local_samples_ann(ann, scores, w, Aelem, opts);
        compute_U_V_bases_ann(w.S, opts, w, depth);
        this->_U_state = this->_V_state = State::COMPRESSED;
        w.c.clear();
      }
    }

    template<typename scalar_t> void
    HSSMatrix<scalar_t>::compute_local_samples_ann
    (DenseMatrix<std::size_t>& ann, DenseM_t& scores,
     WorkCompressANN<scalar_t>& w, const elem_t& Aelem, const opts_t& opts) {
      std::size_t ann_number = ann.rows();
      std::vector<std::size_t> I;
      if (this->leaf()) {
        I.reserve(this->rows());
        for (std::size_t i=0; i<this->rows(); i++)
          I.push_back(i+w.offset.first);

        w.ids_scores.reserve(this->rows()*ann_number);
        for (std::size_t i=w.offset.first;
             i<w.offset.first+this->rows(); i++)
          for (std::size_t j=0; j<ann_number; j++)
            if ((ann(j, i) < w.offset.first) ||
                (ann(j, i) >= w.offset.first + this->rows()))
              w.ids_scores.emplace_back(ann(j, i), scores(j, i));
      } else {
        I.reserve(w.c[0].Ir.size() + w.c[1].Ir.size());
        for (std::size_t i=0; i<w.c[0].Ir.size(); i++)
          I.push_back(w.c[0].Ir[i]);
        for (std::size_t i=0; i<w.c[1].Ir.size(); i++)
          I.push_back(w.c[1].Ir[i]);

        w.ids_scores.reserve(w.c[0].ids_scores.size()+
                             w.c[1].ids_scores.size());
        for (std::size_t i=0; i<w.c[0].ids_scores.size(); i++)
          if ((w.c[0].ids_scores[i].first < w.offset.first) ||
              (w.c[0].ids_scores[i].first >= w.offset.first + this->rows()))
            w.ids_scores.emplace_back(w.c[0].ids_scores[i]);
        for (std::size_t i=0; i<w.c[1].ids_scores.size(); i++)
          if ((w.c[1].ids_scores[i].first < w.offset.first) ||
              (w.c[1].ids_scores[i].first >= w.offset.first + this->rows()))
            w.ids_scores.emplace_back(w.c[1].ids_scores[i]);
      }

      // sort on column indices first, then on scores
      std::sort(w.ids_scores.begin(), w.ids_scores.end());

      // remove duplicate indices, keep only first entry of
      // duplicates, which is the one with the highest score, because
      // of the above sort
      w.ids_scores.erase
        (std::unique(w.ids_scores.begin(), w.ids_scores.end(),
                     [](const std::pair<std::size_t,double>& a,
                        const std::pair<std::size_t,double>& b) {
                       return a.first == b.first; }), w.ids_scores.end());

      // maximum number of samples, d <= number of w.ids_scores
      std::size_t d_max = this->leaf() ?
        I.size() + opts.dd() :   // leaf size + some oversampling
        w.c[0].Ir.size() + w.c[1].Ir.size() + opts.dd();
      auto d = std::min(w.ids_scores.size(), d_max);

      std::vector<std::size_t> Scolids;
      if (d < w.ids_scores.size()) {
        // sort based on scores, keep only d closest
        std::nth_element
          (w.ids_scores.begin(), w.ids_scores.begin()+d, w.ids_scores.end(),
           [](const std::pair<std::size_t,double>& a,
              const std::pair<std::size_t,double>& b) {
            return a.second < b.second; });
        w.ids_scores.resize(d);
      }

      // sort again on column indices first, since we want to search in it later
      std::sort(w.ids_scores.begin(), w.ids_scores.end());

      for (std::size_t z = 1; z < w.ids_scores.size(); z++)
        if (w.ids_scores[z].first <= w.ids_scores[z - 1].first)
          std::cout << "column indices are not sorted!" << std::endl;

      Scolids.reserve(d);
      for (std::size_t j = 0; j < d; j++)
        Scolids.push_back(w.ids_scores[j].first);
      w.S = DenseM_t(I.size(), Scolids.size());

      if (this->leaf())
      {
        Aelem(I, Scolids, w.S);
      }
      else
      {
        std::size_t cur_in_first = 0;
        std::size_t cur_in_second = 0;
        std::size_t cur_in_A = 0;

        // looks for column index cur_in_A is in w_child indices
        // if it is there copies values from w_child.S, otherwise write from Aelem
        // moves cursor in the first child (index_in_child)
        auto update_w_S = [&](const WorkCompressANN<scalar_t> &w_child,
                              size_t index_in_child, size_t row_shift) {
          while (index_in_child < w_child.ids_scores.size() && w_child.ids_scores[index_in_child].first < Scolids[cur_in_A])
            index_in_child++;
          if (index_in_child < w_child.ids_scores.size() && w_child.ids_scores[index_in_child].first == Scolids[cur_in_A])
          {
            for (std::size_t j = 0; j < w_child.Jr.size(); j++)
            {
              w.S(j + row_shift, cur_in_A) = w_child.S(w_child.Jr[j], index_in_child);
            }
          }
          else
          {
            DenseMatrix<float> cur_submatrix(w_child.Ir.size(), 1);
            std::vector<std::size_t> cur_column;
            cur_column.push_back(Scolids[cur_in_A]);
            Aelem(w_child.Ir, cur_column, cur_submatrix);
            for (std::size_t j = 0; j < w_child.Ir.size(); j++)
            {
              w.S(j + row_shift, cur_in_A) = cur_submatrix(j, 0);
            }
          }
          return index_in_child;
        };

        for (cur_in_A = 0; cur_in_A < Scolids.size(); cur_in_A++)
        {
          cur_in_first = update_w_S(w.c[0], cur_in_first, 0);
          cur_in_second = update_w_S(w.c[1], cur_in_second, w.c[0].Ir.size());
        }
      }
    }

    template<typename scalar_t> bool
    HSSMatrix<scalar_t>::compute_U_V_bases_ann
    (DenseM_t& S, const opts_t& opts,
     WorkCompressANN<scalar_t>& w, int depth) {
      auto rtol = opts.rel_tol() / w.lvl;
      auto atol = opts.abs_tol() / w.lvl;
      DenseM_t wSr(S);
      wSr.ID_row(_U.E(), _U.P(), w.Jr, rtol, atol, opts.max_rank(), depth);
      STRUMPACK_ID_FLOPS(ID_row_flops(wSr, _U.cols()));
      // exploit symmetry, set V = U
      _V.E() = _U.E();
      _V.P() = _U.P();
      w.Jc = w.Jr;
      _U.check();  assert(_U.cols() == w.Jr.size());
      _V.check();  assert(_V.cols() == w.Jc.size());
      bool accurate = true;
      int d = S.cols();
      if (!(d - opts.p() >= opts.max_rank() ||
            (int(_U.cols()) < d - opts.p() &&
             int(_V.cols()) < d - opts.p()))) {
        accurate = false;
        std::cout << "WARNING: ID did not reach required accuracy:"
                  << "\t increase k (number of ANN's), or Delta_d."
                  << std::endl;
      }
      this->_U_rank = _U.cols();  this->_U_rows = _U.rows();
      this->_V_rank = _V.cols();  this->_V_rows = _V.rows();
      w.Ir.reserve(_U.cols());
      w.Ic.reserve(_V.cols());
      if (this->leaf()) {
        for (auto i : w.Jr) w.Ir.push_back(w.offset.first + i);
        for (auto j : w.Jc) w.Ic.push_back(w.offset.second + j);
      } else {
        auto r0 = w.c[0].Ir.size();
        for (auto i : w.Jr)
          w.Ir.push_back((i < r0) ? w.c[0].Ir[i] : w.c[1].Ir[i-r0]);
        r0 = w.c[0].Ic.size();
        for (auto j : w.Jc)
          w.Ic.push_back((j < r0) ? w.c[0].Ic[j] : w.c[1].Ic[j-r0]);
      }
      return accurate;
    }

  } // end namespace HSS
} // end namespace strumpack

#endif // HSS_MATRIX_COMPRESS_KERNEL_HPP
