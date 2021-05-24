/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2021 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/*    Authors: Julian Hall, Ivet Galabova, Qi Huangfu, Leona Gottwald    */
/*    and Michael Feldmeier                                              */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#ifndef MIP_HIGHS_MIP_SOLVER_H_
#define MIP_HIGHS_MIP_SOLVER_H_

#include "Highs.h"
#include "lp_data/HighsOptions.h"

struct HighsMipSolverData;
class HighsCutPool;
class HighsPseudocostInitialization;
class HighsCliqueTable;
class HighsImplications;

class HighsMipSolver {
 public:
  const HighsOptions* options_mip_;
  const HighsLp* model_;
  const HighsLp* orig_model_;
  HighsModelStatus modelstatus_;
  std::vector<double> solution_;
  double solution_objective_;
  double bound_violation_;
  double integrality_violation_;
  double row_violation_;
  double dual_bound_;
  double primal_bound_;
  int64_t node_count_;

  bool submip;
  const HighsBasis* rootbasis;
  const HighsPseudocostInitialization* pscostinit;
  const HighsCliqueTable* clqtableinit;
  const HighsImplications* implicinit;

  std::unique_ptr<HighsMipSolverData> mipdata_;

  void run();

  HighsInt numCol() const { return model_->numCol_; }

  HighsInt numRow() const { return model_->numRow_; }

  HighsInt numNonzero() const { return model_->Aindex_.size(); }

  const double* colCost() const { return model_->colCost_.data(); }

  double colCost(HighsInt col) const { return model_->colCost_[col]; }

  const double* rowLower() const { return model_->rowLower_.data(); }

  double rowLower(HighsInt col) const { return model_->rowLower_[col]; }

  const double* rowUpper() const { return model_->rowUpper_.data(); }

  double rowUpper(HighsInt col) const { return model_->rowUpper_[col]; }

  bool isSolutionFeasible(const std::vector<double>& solution) const;

  const HighsVarType* variableType() const {
    return model_->integrality_.data();
  }

  HighsVarType variableType(HighsInt col) const {
    return model_->integrality_[col];
  }

  HighsMipSolver(const HighsOptions& options, const HighsLp& lp,
                 const HighsSolution& solution, bool submip = false);

  ~HighsMipSolver();

  void setModel(const HighsLp& model) {
    model_ = &model;
    solution_objective_ = kHighsInf;
  }

  mutable HighsTimer timer_;
  void cleanupSolve();
};

#endif
