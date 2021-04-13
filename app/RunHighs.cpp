/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2019 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**@file ../app/RunHighs.cpp
 * @brief HiGHS main
 * @author Julian Hall, Ivet Galabova, Qi Huangfu and Michael Feldmeier
 */
#include "HConfig.h"
#include "Highs.h"
#include "HighsIO.h"
#include "HighsMipSolver.h"
#include "HighsOptions.h"
#include "HighsRuntimeOptions.h"
#include "HighsTimer.h"
#include "presolve/HAggregator.h"

void printHighsVersionCopyright(const HighsLogOptions& log_options,
                                const char* message = nullptr);
void reportLpStatsOrError(const HighsLogOptions& log_options,
                          const HighsStatus read_status, const HighsLp& lp);
void reportSolvedLpStats(const HighsLogOptions& log_options,
                         const HighsStatus run_status, const Highs& highs);
HighsStatus callLpSolver(HighsOptions& options, const HighsLp& lp);
HighsStatus callMipSolver(HighsOptions& options, const HighsLp& lp);

int main(int argc, char** argv) {
  // Load user options.
  HighsOptions options;
  printHighsVersionCopyright(options.log_options);

  bool options_ok = loadOptions(argc, argv, options);
  if (!options_ok) return 0;
  Highs highs;
  //  highs.setOptionValue("log_dev_level", 1);
  HighsStatus read_status = highs.readModel(options.model_file);
  reportLpStatsOrError(options.log_options, read_status, highs.getLp());
  if (read_status == HighsStatus::Error)
    return 1;  // todo: change to read error

  // Run LP or MIP solver.
  const HighsLp& lp = highs.getLp();
  HighsStatus run_status = HighsStatus::Error;

  bool is_mip = false;
  for (HighsInt i = 0; i < (HighsInt)lp.integrality_.size(); i++)
    if (lp.integrality_[i] == HighsVarType::INTEGER) {
      is_mip = true;
      break;
    }
  is_mip = false;

  if (options.solver == "simplex" || options.solver == "ipm" ||
      (!is_mip && options.presolve != "mip")) {
    run_status = callLpSolver(options, lp);
  } else {
    run_status = callMipSolver(options, lp);
  }

  return (int)run_status;
}

void printHighsVersionCopyright(const HighsLogOptions& log_options,
                                const char* message) {
  highsLogUser(log_options, HighsLogType::INFO,
               "Running HiGHS %" HIGHSINT_FORMAT ".%" HIGHSINT_FORMAT
               ".%" HIGHSINT_FORMAT " [date: %s, git hash: %s]\n",
               HIGHS_VERSION_MAJOR, HIGHS_VERSION_MINOR, HIGHS_VERSION_PATCH,
               HIGHS_COMPILATION_DATE, HIGHS_GITHASH);
  highsLogUser(log_options, HighsLogType::INFO,
               "Copyright (c) 2021 ERGO-Code under MIT licence terms\n\n");
#ifdef HiGHSDEV
  // Report on preprocessing macros
  if (message != nullptr) {
    highsLogUser(log_options, HighsLogType::INFO, "In %s\n", message);
  }
#ifdef OPENMP
  highsLogUser(log_options, HighsLogType::INFO,
               "OPENMP           is     defined\n");
#else
  highsLogUser(log_options, HighsLogType::INFO,
               "OPENMP           is not defined\n");
#endif

#ifdef SCIP_DEV
  highsLogUser(log_options, HighsLogType::INFO,
               "SCIP_DEV         is     defined\n");
#else
  highsLogUser(log_options, HighsLogType::INFO,
               "SCIP_DEV         is not defined\n");
#endif

#ifdef HiGHSDEV
  highsLogUser(log_options, HighsLogType::INFO,
               "HiGHSDEV         is     defined\n");
#else
  highsLogUser(log_options, HighsLogType::INFO,
               "HiGHSDEV         is not defined\n");
#endif
  highsLogUser(log_options, HighsLogType::INFO,
               "Built with CMAKE_BUILD_TYPE=%s\n", CMAKE_BUILD_TYPE);
#endif
}

void reportLpStatsOrError(const HighsLogOptions& log_options,
                          const HighsStatus read_status, const HighsLp& lp) {
  if (read_status == HighsStatus::Error) {
    highsLogUser(log_options, HighsLogType::INFO, "Error loading file\n");
  } else {
    highsLogUser(log_options, HighsLogType::INFO, "LP       : %s\n",
                 lp.model_name_.c_str());
    highsLogUser(log_options, HighsLogType::INFO,
                 "Rows     : %" HIGHSINT_FORMAT "\n", lp.numRow_);
    highsLogUser(log_options, HighsLogType::INFO,
                 "Cols     : %" HIGHSINT_FORMAT "\n", lp.numCol_);
    highsLogUser(log_options, HighsLogType::INFO,
                 "Nonzeros : %" HIGHSINT_FORMAT "\n", lp.Avalue_.size());
    HighsInt num_int = 0;
    for (HighsUInt i = 0; i < lp.integrality_.size(); i++)
      if (lp.integrality_[i] != HighsVarType::CONTINUOUS) num_int++;
    if (num_int)
      highsLogUser(log_options, HighsLogType::INFO,
                   "Integer  : %" HIGHSINT_FORMAT "\n", num_int);
  }
}

void reportSolvedLpStats(const HighsLogOptions& log_options,
                         const HighsStatus run_status, Highs& highs) {
  if (run_status == HighsStatus::Error) {
    std::string statusname = HighsStatusToString(run_status);
    highsLogUser(log_options, HighsLogType::INFO, "HiGHS status: %s\n",
                 statusname.c_str());
  } else {
    highsLogUser(log_options, HighsLogType::INFO, "\n");
    HighsModelStatus model_status = highs.getModelStatus();
    HighsModelStatus scaled_model_status = highs.getModelStatus(true);
    HighsInfo highs_info = highs.getInfo();
    if (model_status != scaled_model_status) {
      if (scaled_model_status == HighsModelStatus::OPTIMAL) {
        // The scaled model has been solved to optimality, but not the
        // unscaled model, flag this up, but report the scaled model
        // status
        highsLogUser(log_options, HighsLogType::INFO,
                     "Primal infeasibility: %10.3e (%" HIGHSINT_FORMAT ")\n",
                     highs_info.max_primal_infeasibility,
                     highs_info.num_primal_infeasibilities);
        highsLogUser(log_options, HighsLogType::INFO,
                     "Dual   infeasibility: %10.3e (%" HIGHSINT_FORMAT ")\n",
                     highs_info.max_dual_infeasibility,
                     highs_info.num_dual_infeasibilities);
        model_status = scaled_model_status;
      }
    }
    highsLogUser(log_options, HighsLogType::INFO, "Model   status      : %s\n",
                 highs.modelStatusToString(model_status).c_str());
    highsLogUser(
        log_options, HighsLogType::INFO, "Primal  status      : %s\n",
        highs.primalDualStatusToString(highs_info.primal_status).c_str());
    highsLogUser(
        log_options, HighsLogType::INFO, "Dual    status      : %s\n",
        highs.primalDualStatusToString(highs_info.dual_status).c_str());
    highsLogUser(log_options, HighsLogType::INFO,
                 "Simplex   iterations: %" HIGHSINT_FORMAT "\n",
                 highs_info.simplex_iteration_count);
    if (highs_info.ipm_iteration_count)
      highsLogUser(log_options, HighsLogType::INFO,
                   "IPM       iterations: %" HIGHSINT_FORMAT "\n",
                   highs_info.ipm_iteration_count);
    if (highs_info.crossover_iteration_count)
      highsLogUser(log_options, HighsLogType::INFO,
                   "Crossover iterations: %" HIGHSINT_FORMAT "\n",
                   highs_info.crossover_iteration_count);
    if (model_status == HighsModelStatus::OPTIMAL) {
      double objective_function_value;
      highs.getInfoValue("objective_function_value", objective_function_value);
      highsLogUser(log_options, HighsLogType::INFO,
                   "Objective value     : %17.10e\n", objective_function_value);
    }
    double run_time = highs.getRunTime();
    highsLogUser(log_options, HighsLogType::INFO,
                 "HiGHS run time      : %13.2f\n", run_time);
    // Possibly write the solution to a file
    const HighsOptions& options = highs.getOptions();
    if (options.write_solution_to_file)
      highs.writeSolution(options.solution_file, options.write_solution_pretty);
  }
}

HighsStatus callLpSolver(HighsOptions& use_options, const HighsLp& lp) {
  // Solve LP case.
  Highs highs;
  highs.passOptions(use_options);
  //  const HighsOptions& options = highs.getOptions();

  // Load problem.
  highs.passModel(lp);
  // HighsStatus read_status = highs.readModel(options.model_file);
  // reportLpStatsOrError(options.log_options, read_status, highs.getLp());
  // if (read_status == HighsStatus::Error) return HighsStatus::Error;

  // Run HiGHS.
  highs.setBasis();
  HighsStatus run_status = highs.run();

  if (highs.getInfo().mip_node_count == -1)
    reportSolvedLpStats(use_options.log_options, run_status, highs);
  //  HighsRanging ranging; highs.getRanging(ranging);
  //  highs.writeSolution("", true);
  return run_status;
}

HighsStatus callMipSolver(HighsOptions& use_options, const HighsLp& lp) {
  use_options.log_dev_level = LOG_DEV_LEVEL_INFO;
  HighsMipSolver solver(use_options, lp);
  solver.run();

  return HighsStatus::OK;
}
