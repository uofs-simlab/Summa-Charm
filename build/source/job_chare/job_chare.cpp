#include "FileAccessChare.decl.h" // Include this first to resolve CBase_FileAccessChare
#include "GruWorker.decl.h"       // Include this first to resolve CBase_GruWorker
#include "job_chare.hpp"
#include "SummaChare.decl.h"
#include "gru_struc.hpp"

#include "summa_init_struc.hpp" // Re-enable the include
#include <algorithm>
#include <fstream>
#include <limits>
#include <numeric>
#include <limits.h>
#include <unistd.h>

JobChare::JobChare(Batch batch, bool enable_logging,
                   JobChareSettings job_chare_settings,
                   FileAccessChareSettings fa_chare_settings,
                   HRUChareSettings hru_chare_settings,
                   CkChareID summa_chare_proxy)
    : CBase_JobChare(), batch_(batch),
      summa_chare_proxy_(summa_chare_proxy), enable_logging_(enable_logging),
      job_chare_settings_(job_chare_settings),
      fa_chare_settings_(fa_chare_settings),
      hru_chare_settings_(hru_chare_settings)
{
  std::string err_msg;
  // Get hostname for logging
  gethostname(hostname_, HOST_NAME_MAX);

  // Timing Information
  timing_info_ = TimingInfo();
  timing_info_.addTimePoint("total_duration");
  timing_info_.updateStartPoint("total_duration");
  timing_info_.addTimePoint("init_duration");
  timing_info_.updateStartPoint("init_duration");

  // Create Loggers
  if (enable_logging_)
  {
    logger_ = std::make_unique<Logger>(batch_.getLogDir() + "batch_" +
                                       std::to_string(batch_.getBatchID()));
    err_logger_ = std::make_unique<ErrorLogger>(batch_.getLogDir());
    success_logger_ = std::make_unique<SuccessLogger>(batch_.getLogDir());
  }
  else
  {
    logger_ = std::make_unique<Logger>("");
    err_logger_ = std::make_unique<ErrorLogger>("");
    success_logger_ = std::make_unique<SuccessLogger>("");
  }

  if (enable_logging_ && !batch_.getLogDir().empty())
  {
    pe_distribution_csv_path_ =
        batch_.getLogDir() + "pe_gru_distribution_batch_" +
        std::to_string(batch_.getBatchID()) + ".csv";
    std::ofstream csv_file(pe_distribution_csv_path_, std::ios::out);
    if (csv_file.is_open())
    {
      csv_file << "batch_id,label,pe,count,total,min,max,equal,start_gru,num_gru\n";
      pe_distribution_csv_ready_ = true;
    }
  }

  // GruStruc Initialization
  gru_struc_ =
      std::make_unique<GruStruc>(batch_.getStartHRU(), batch_.getNumHRU(),
                                 job_chare_settings_.max_run_attempts_);
  if (gru_struc_->readDimension())
  {
    err_msg = "ERROR: Job_Chare - ReadDimension\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (gru_struc_->readIcondNlayers())
  {
    err_msg = "ERROR: Job_Chare - ReadIcondNlayers\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  gru_struc_->getNumHrusPerGru();
  gru_start_times_.assign(
      static_cast<size_t>(gru_struc_->getNumGru()) + 1,
      std::chrono::time_point<std::chrono::steady_clock>{});

  if (fa_chare_settings_.num_partitions_in_output_buffer_ < 1)
  {
    CkPrintf("JobChare: Invalid output partitions=%d, forcing to 1\n",
             fa_chare_settings_.num_partitions_in_output_buffer_);
    fa_chare_settings_.num_partitions_in_output_buffer_ = 1;
  }

  // SummaInitStruc Initialization
  summa_init_struc_ = std::make_unique<SummaInitStruc>();
  if (summa_init_struc_->allocate(batch_.getNumHRU()) != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc allocation failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_paramSetup() != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc paramSetup failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  if (summa_init_struc_->summa_readRestart() != 0)
  {
    err_msg = "ERROR -- Job_Chare: SummaInitStruc readRestart failed\n";
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }
  summa_init_struc_->getInitTolerance(tolerance_settings_.rel_tol_temp_cas_, 
      tolerance_settings_.rel_tol_temp_veg_, tolerance_settings_.rel_tol_wat_veg_, 
      tolerance_settings_.rel_tol_temp_soil_snow_, tolerance_settings_.rel_tol_wat_snow_, 
      tolerance_settings_.rel_tol_matric_, tolerance_settings_.rel_tol_aquifr_, 
      tolerance_settings_.abs_tol_temp_cas_, tolerance_settings_.abs_tol_temp_veg_, 
      tolerance_settings_.abs_tol_wat_veg_, tolerance_settings_.abs_tol_temp_soil_snow_, 
      tolerance_settings_.abs_tol_wat_snow_, tolerance_settings_.abs_tol_matric_, 
      tolerance_settings_.abs_tol_aquifr_, default_tol_, tolerance_settings_.be_steps_);

  num_gru_info_ = NumGRUInfo(batch_.getStartHRU(), batch_.getStartHRU(), batch_.getNumHRU(),
                             batch_.getNumHRU(), gru_struc_->getFileGru(), false);



  // Start File Access Chare and Become User Selected Mode
  file_access_chare_ = CProxy_FileAccessChare::ckNew(num_gru_info_, fa_chare_settings_, thisProxy.ckGetChareID());

  int num_timesteps = file_access_chare_.initFileAccessChare(gru_struc_->getFileGru(), gru_struc_->getNumHru());
  if (num_timesteps < 0)
  {
    std::string err_msg =
        "ERROR: JobChare: FileAccessChare initialization failed\n";
    CkPrintf("JobChare: %s", err_msg.c_str());
    // this->handleError(-2, err_msg);
    CProxy_SummaChare(summa_chare_proxy_).reportError(-2, err_msg);
    return;
  }

  timing_info_.updateEndPoint("init_duration");

  // Start JobChare in User Selected Mode
  logger_->log("JobChare Initialized");
  logger_->log("Async Mode: File Access Chare Ready");

  // TODO: Implement the data assimilation mode logic if needed
  num_steps_ = num_timesteps;
  spawnGruChares();
}

// ------------------------ Member Functions ------------------------
void JobChare::logPeGruDistribution(const char *label,
                                    const std::vector<int> &counts)
{
  if (counts.empty())
  {
    return;
  }

  const auto minmax = std::minmax_element(counts.begin(), counts.end());
  const int min_count = *minmax.first;
  const int max_count = *minmax.second;
  const int total_count = std::accumulate(counts.begin(), counts.end(), 0);
  const bool is_equal = (min_count == max_count);

  CkPrintf("\n________________PE GRU DISTRIBUTION (%s)________________\n",
           label);
  CkPrintf("PEs=%zu TotalGRUs=%d Min=%d Max=%d Equal=%s\n", counts.size(),
           total_count, min_count, max_count, is_equal ? "YES" : "NO");
  for (size_t pe = 0; pe < counts.size(); ++pe)
  {
    CkPrintf("PE %zu -> %d GRUs\n", pe, counts[pe]);
  }
  CkPrintf("________________________________________________________\n");

  appendPeGruDistributionCsv(label, counts);
}

void JobChare::appendPeGruDistributionCsv(const char *label,
                                          const std::vector<int> &counts)
{
  if (!pe_distribution_csv_ready_ || counts.empty())
  {
    return;
  }

  const auto minmax = std::minmax_element(counts.begin(), counts.end());
  const int min_count = *minmax.first;
  const int max_count = *minmax.second;
  const int total_count = std::accumulate(counts.begin(), counts.end(), 0);
  const int equal_flag = (min_count == max_count) ? 1 : 0;

  std::ofstream csv_file(pe_distribution_csv_path_, std::ios::out | std::ios::app);
  if (!csv_file.is_open())
  {
    return;
  }
  for (size_t pe = 0; pe < counts.size(); ++pe)
  {
    csv_file << batch_.getBatchID() << "," << label << "," << pe << ","
             << counts[pe] << "," << total_count << "," << min_count << ","
             << max_count << "," << equal_flag << "," << batch_.getStartHRU()
             << "," << batch_.getNumHRU() << "\n";
  }
}

void JobChare::enqueueJob(int job_index)
{
  if (job_index <= 0)
  {
    return;
  }
  pending_jobs_global_.push_back(job_index);
}

bool JobChare::dequeueJobForWorker(int worker_id, int &job_index)
{
  if (worker_id < 0 || worker_id >= num_workers_ ||
      pending_jobs_global_.empty())
  {
    return false;
  }
  job_index = pending_jobs_global_.front();
  pending_jobs_global_.pop_front();
  return true;
}

void JobChare::spawnGruChares()
{
  int num_grus = gru_struc_->getNumGru();
  total_gru_to_construct_ = num_grus;
  num_gru_constructed_ = 0;

  if (num_grus <= 0)
  {
    finalizeJob();
    return;
  }

  const int pe_count = std::max(1, CkNumPes());
  const bool reserve_pe0 =
      job_chare_settings_.reserve_pe0_for_control_ && pe_count > 1;
  const int usable_worker_pes = reserve_pe0 ? (pe_count - 1) : pe_count;
  if (job_chare_settings_.reserve_pe0_for_control_ && pe_count <= 1)
  {
    CkPrintf("JobChare: reserve_pe0_for_control requested but only %d PE is available; ignoring.\n",
             pe_count);
  }
  int requested_workers =
      (job_chare_settings_.worker_pool_size_ > 0)
          ? job_chare_settings_.worker_pool_size_
          : usable_worker_pes;
  if (!job_chare_settings_.allow_worker_oversubscription_ &&
      requested_workers > usable_worker_pes)
  {
    CkPrintf(
        "JobChare: Capping worker_pool_size from %d to %d (usable worker PEs). "
        "Set allow_worker_oversubscription=true to override.\n",
        requested_workers, usable_worker_pes);
    requested_workers = usable_worker_pes;
  }
  num_workers_ = std::min(num_grus, std::max(1, requested_workers));
  worker_prefetch_depth_effective_ =
      std::max(1, job_chare_settings_.worker_prefetch_depth_);
  if (num_workers_ < usable_worker_pes)
  {
    CkPrintf("JobChare: WARNING only %d workers for %d usable worker PEs; %d PE(s) will be idle.\n",
             num_workers_, usable_worker_pes, usable_worker_pes - num_workers_);
  }
  if (!reserve_pe0 && pe_count > 1 && requested_workers == pe_count - 1)
  {
    CkPrintf("JobChare: NOTE worker_pool_size=%d with reserve_pe0_for_control=false leaves one PE idle. "
             "Use worker_pool_size=%d (all PEs) or set reserve_pe0_for_control=true.\n",
             requested_workers, pe_count);
  }

  pending_jobs_global_.clear();
  job_to_worker_.assign(static_cast<size_t>(num_grus) + 1, -1);
  worker_to_pe_.assign(static_cast<size_t>(num_workers_), -1);
  inflight_tasks_per_worker_.assign(static_cast<size_t>(num_workers_), 0);
  assigned_grus_per_pe_.assign(static_cast<size_t>(pe_count), 0);
  completed_grus_per_pe_.assign(static_cast<size_t>(pe_count), 0);

  // Build GRU tracking metadata and populate global work queue.
  for (int i = 0; i < num_grus; ++i)
  {
    const int netcdf_index = gru_struc_->getStartGru() + i;
    const int job_index = i + 1;
    enqueueJob(job_index);

    CkChareID dummy_chare_id;
    memset(&dummy_chare_id, 0, sizeof(CkChareID));
    std::unique_ptr<GRU> gru_obj = std::make_unique<GRU>(
        netcdf_index, job_index, dummy_chare_id, dt_init_factor_,
        tolerance_settings_, default_tol_, job_chare_settings_.max_run_attempts_);
    gru_struc_->addGRU(std::move(gru_obj));
  }

  CkPrintf(
      "JobChare: Global work queue: workers=%d requested=%d PEs=%d usable_worker_pes=%d "
      "pending_jobs=%zu\n",
      num_workers_, requested_workers, pe_count, usable_worker_pes,
      pending_jobs_global_.size());
  CkPrintf("JobChare: Worker prefetch depth=%d\n",
           worker_prefetch_depth_effective_);
  if (num_workers_ > 0)
  {
    const int avg_jobs_per_worker =
        (num_grus + num_workers_ - 1) / num_workers_;
    if (worker_prefetch_depth_effective_ >= avg_jobs_per_worker)
    {
      CkPrintf("JobChare: WARNING worker_prefetch_depth=%d >= avg_jobs_per_worker=%d; "
               "this effectively turns dynamic scheduling into static chunking.\n",
               worker_prefetch_depth_effective_, avg_jobs_per_worker);
    }
  }
  if (job_chare_settings_.allow_worker_oversubscription_ &&
      requested_workers > usable_worker_pes)
  {
    CkPrintf("JobChare: WARNING worker oversubscription is enabled (%d workers / %d usable worker PEs)\n",
             requested_workers, usable_worker_pes);
  }

  // Create a fixed worker pool and pin one worker per PE.
  gru_worker_array_ = CProxy_GruWorker::ckNew();
  CkChareID file_access_id = file_access_chare_.ckGetChareID();
  const int worker_pe_count = usable_worker_pes;
  const int worker_pe_offset = reserve_pe0 ? 1 : 0;
  CkPrintf("JobChare: Worker PE mapping=%s\n",
           reserve_pe0 ? "reserve PE0 for control actors"
                       : "use all available PEs");
  for (int worker = 0; worker < num_workers_; ++worker)
  {
    const int on_pe = (worker % worker_pe_count) + worker_pe_offset;
    worker_to_pe_[static_cast<size_t>(worker)] = on_pe;
    gru_worker_array_[worker].insert(worker, num_steps_, hru_chare_settings_,
                                     fa_chare_settings_.num_timesteps_in_output_buffer_,
                                     file_access_id, thisProxy.ckGetChareID(),
                                     on_pe);
  }
  gru_worker_array_.doneInserting();

  // Give FileAccessChare direct access to GruWorkers so write-resume and
  // forcing-file messages bypass the JobChare bottleneck.
  file_access_chare_.setGruWorkerProxy(
      gru_worker_array_.ckGetArrayID(), num_grus + 1, num_workers_);

  for (int worker = 0; worker < num_workers_; ++worker)
  {
    while (static_cast<size_t>(worker) < inflight_tasks_per_worker_.size() &&
           inflight_tasks_per_worker_[static_cast<size_t>(worker)] <
               worker_prefetch_depth_effective_)
    {
      if (!assignNextTask(worker))
      {
        break;
      }
    }
  }

  logPeGruDistribution("ASSIGNED_INITIAL", assigned_grus_per_pe_);

  gru_struc_->decrementRetryAttempts();
}

void JobChare::notifyGruConstructed(int job_index)
{
  // Legacy callback kept for compatibility with older GruChare runs.
  (void)job_index;
}

// Implementation method for finalization
void JobChare::finalizeJob()
{
  logPeGruDistribution("ASSIGNED_FINAL", assigned_grus_per_pe_);
  logPeGruDistribution("COMPLETED_SUCCESS", completed_grus_per_pe_);

  std::tuple<double, double> read_write_duration = file_access_chare_.finalize();
  CkPrintf("read_write_duration = (%f, %f)\n",
           std::get<0>(read_write_duration), std::get<1>(read_write_duration));
  int err = 0;
  int num_failed_grus = gru_struc_->getNumGruFailed();
  timing_info_.updateEndPoint("total_duration");

  // Deallocate GRU_Struc
  summa_init_struc_.reset();
  gru_struc_.reset(); 
  // Tell Parent we are done
  double total_duration = timing_info_.getDuration("total_duration").value_or(-1.0);
  double read_duration = std::get<0>(read_write_duration);
  double write_duration = std::get<1>(read_write_duration);
  CProxy_SummaChare(summa_chare_proxy_).doneJob(num_failed_grus, total_duration, read_duration, write_duration);
}

void JobChare::doneHRUJob(int job_index, int worker_id)
{
  handleFinishedGRU(job_index, worker_id);
}

bool JobChare::assignNextTask(int worker_id, bool ignore_prefetch_cap)
{
  if (worker_id < 0 || worker_id >= num_workers_ || !gru_struc_ ||
      assigned_grus_per_pe_.empty() || static_cast<size_t>(worker_id) >= worker_to_pe_.size() ||
      static_cast<size_t>(worker_id) >= inflight_tasks_per_worker_.size())
  {
    return false;
  }
  if (!ignore_prefetch_cap &&
      inflight_tasks_per_worker_[static_cast<size_t>(worker_id)] >=
      worker_prefetch_depth_effective_)
  {
    return false;
  }

  int job_index = -1;
  if (!dequeueJobForWorker(worker_id, job_index))
  {
    return false;
  }

  if (job_index <= 0 ||
      static_cast<size_t>(job_index) >= job_to_worker_.size())
  {
    return false;
  }

  const int netcdf_index = gru_struc_->getStartGru() + job_index - 1;
  const int pe = worker_to_pe_[static_cast<size_t>(worker_id)];
  if (pe < 0 || static_cast<size_t>(pe) >= assigned_grus_per_pe_.size())
  {
    return false;
  }

  job_to_worker_[job_index] = worker_id;
  assigned_grus_per_pe_[static_cast<size_t>(pe)]++;
  if (static_cast<size_t>(job_index) < gru_start_times_.size())
  {
    gru_start_times_[job_index] = std::chrono::steady_clock::now();
  }
  inflight_tasks_per_worker_[static_cast<size_t>(worker_id)]++;

  // Update FAC mapping before sending assignTask so forcing/write replies
  // can be routed directly to this worker without bouncing via JobChare.
  file_access_chare_.updateJobWorkerMapping(job_index, worker_id);
  gru_worker_array_[worker_id].assignTask(
      netcdf_index, job_index, tolerance_settings_, dt_init_factor_, default_tol_);
  return true;
}

void JobChare::requestMoreWork(int worker_id)
{
  if (worker_id < 0 || worker_id >= num_workers_ ||
      static_cast<size_t>(worker_id) >= inflight_tasks_per_worker_.size())
  {
    return;
  }
  // Workers can be stalled behind output-buffer barriers. In that state they
  // still have inflight tasks, but we must allow one extra assignment to avoid
  // deadlock when a partition has not yet received all required GRUs.
  assignNextTask(worker_id, true);
}

void JobChare::handleFinishedGRU(int job_index, int worker_id)
{
  if (!gru_struc_) {
    return;
  }
  const int resolved_worker_id =
      (job_index >= 0 && static_cast<size_t>(job_index) < job_to_worker_.size())
          ? job_to_worker_[job_index]
          : worker_id;

  auto *gru = gru_struc_->getGRU(job_index);
  if (!gru) {
    return;
  }
  if (gru->getStatus() == gru_state::failed ||
      gru->getStatus() == gru_state::succeeded) {
    return;
  }
  if (job_index >= 0 &&
      static_cast<size_t>(job_index) < gru_start_times_.size()) {
    const auto start_time = gru_start_times_[job_index];
    if (start_time != std::chrono::time_point<std::chrono::steady_clock>{}) {
      const auto run_time = std::chrono::duration<double>(
          std::chrono::steady_clock::now() - start_time).count();
      gru->setRunTime(run_time);
    }
  }
  const int resolved_pe_id =
      (resolved_worker_id >= 0 &&
       static_cast<size_t>(resolved_worker_id) < worker_to_pe_.size())
          ? worker_to_pe_[static_cast<size_t>(resolved_worker_id)]
          : -1;
  if (resolved_pe_id >= 0 &&
      static_cast<size_t>(resolved_pe_id) < completed_grus_per_pe_.size())
  {
    const size_t pe = static_cast<size_t>(resolved_pe_id);
    completed_grus_per_pe_[pe]++;
  }
  if (resolved_worker_id >= 0 &&
      static_cast<size_t>(resolved_worker_id) < inflight_tasks_per_worker_.size() &&
      inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)] > 0)
  {
    inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)]--;
  }
  if (job_index >= 0 && static_cast<size_t>(job_index) < job_to_worker_.size())
  {
    job_to_worker_[job_index] = -1;
  }
  gru_struc_->incrementNumGruDone();
  gru->setSuccess();
  success_logger_->logSuccess(gru->getIndexNetcdf(),
                              gru->getIndexJob(),
                              MISSING_DOUBLE, MISSING_DOUBLE,  // Using default rel_tol and abs_tol values
                              tolerance_settings_.rel_tol_temp_cas_, tolerance_settings_.rel_tol_temp_veg_,
                              tolerance_settings_.rel_tol_wat_veg_, tolerance_settings_.rel_tol_temp_soil_snow_,
                              tolerance_settings_.rel_tol_wat_snow_, tolerance_settings_.rel_tol_matric_,
                              tolerance_settings_.rel_tol_aquifr_, tolerance_settings_.abs_tol_temp_cas_,
                              tolerance_settings_.abs_tol_temp_veg_, tolerance_settings_.abs_tol_wat_veg_,
                              tolerance_settings_.abs_tol_temp_soil_snow_, tolerance_settings_.abs_tol_wat_snow_,
                              tolerance_settings_.abs_tol_matric_, tolerance_settings_.abs_tol_aquifr_,
                              default_tol_, gru->getRunTime(), resolved_worker_id,
                              resolved_pe_id);
  std::string update_str =
      "GRU Finished: " + std::to_string(gru_struc_->getNumGruDone()) + "/" +
      std::to_string(gru_struc_->getNumGru()) + " -- GlobalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexNetcdf()) +
      " -- LocalGRU=" +
      std::to_string(gru_struc_->getGRU(job_index)->getIndexJob()) +
      " -- NumFailed=" + std::to_string(gru_struc_->getNumGruFailed()) + "\n";
  logger_->log(update_str);
  CkPrintf("%s", update_str.c_str());

  if (gru_struc_->isDone())
  {
    gru_struc_->hasFailures() && gru_struc_->shouldRetry() ? restartFailures() : finalizeJob();
    return;
  }
  if (resolved_worker_id >= 0 &&
      static_cast<size_t>(resolved_worker_id) < inflight_tasks_per_worker_.size())
  {
    while (inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)] <
           worker_prefetch_depth_effective_)
    {
      if (!assignNextTask(resolved_worker_id))
      {
        break;
      }
    }
  }
}

void JobChare::restartFailures()
{
  logger_->log("Async Mode: Restarting Failed GRUs");
  // CkPrintf("Async Mode: Restarting Failed GRUs\n");

  
      auto tighten_tol = [&](double& tol, const double& min_tol, const std::string& name){
        if (tol > min_tol) {
          tol /= 10;
          // CkPrintf("Async Mode: Tightening tolerance\n");
          // CkPrintf("Async Mode: %s = %f\n", name.c_str(), tol);
          return true;
        }
        return false;
      };

      // Update tolerances (general and specific)
      bool tol_updated = false;

      tolerance_settings_.be_steps_ = tolerance_settings_.be_steps_ * 2;
      // CkPrintf("Async Mode: Tightening be steps: %d\n", tolerance_settings_.be_steps_);
      
      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_cas_, MIN_REL_TOL, "rel_tol_temp_cas_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_veg_, MIN_REL_TOL, "rel_tol_temp_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_wat_veg_, MIN_REL_TOL, "rel_tol_wat_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_temp_soil_snow_, MIN_REL_TOL, "rel_tol_temp_soil_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_wat_snow_, MIN_REL_TOL, "rel_tol_wat_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_matric_, MIN_REL_TOL, "rel_tol_matric_");

      tol_updated |= tighten_tol(tolerance_settings_.rel_tol_aquifr_, MIN_REL_TOL, "rel_tol_aquifr_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_cas_, MIN_ABS_TOL, "abs_tol_temp_cas_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_veg_, MIN_ABS_TOL, "abs_tol_temp_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_wat_veg_, MIN_ABS_TOL, "abs_tol_wat_veg_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_temp_soil_snow_, MIN_ABS_TOL, "abs_tol_temp_soil_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_wat_snow_, MIN_ABS_TOL, "abs_tol_wat_snow_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_matric_, MIN_ABS_TOL, "abs_tol_matric_");

      tol_updated |= tighten_tol(tolerance_settings_.abs_tol_aquifr_, MIN_ABS_TOL, "abs_tol_aquifr_");

  // notify file_access_chare
  // TODO: Make it sync type in .ci so it waits untill finishing reconstruct()
  int sleep = file_access_chare_.restartFailures();

  err_logger_->nextAttempt();
  success_logger_->nextAttempt();

  pending_jobs_global_.clear();
  std::fill(inflight_tasks_per_worker_.begin(), inflight_tasks_per_worker_.end(), 0);

  while (gru_struc_->getNumGruFailed() > 0)
  {
    int job_index = gru_struc_->getFailedIndex();
    if (job_index < 1) {
      // CkPrintf("Async Mode: No failed GRU index found, breaking.\n");
      break;
    }
    logger_->log("Async Mode: Restarting GRU: " + std::to_string(job_index));
    // CkPrintf("Async Mode: Restarting GRU: %s", std::to_string(job_index).c_str());
    if (auto *gru = gru_struc_->getGRU(job_index)) {
      gru->setRestarted();
    }
    if (static_cast<size_t>(job_index) < job_to_worker_.size())
    {
      job_to_worker_[job_index] = -1;
    }
    enqueueJob(job_index);
    gru_struc_->decrementNumGruFailed();
  }
  gru_struc_->decrementRetryAttempts();
  for (int worker = 0; worker < num_workers_; ++worker)
  {
    while (static_cast<size_t>(worker) < inflight_tasks_per_worker_.size() &&
           inflight_tasks_per_worker_[static_cast<size_t>(worker)] <
               worker_prefetch_depth_effective_)
    {
      if (!assignNextTask(worker))
      {
        break;
      }
    }
  }
}

// Array communication forwarding methods
void JobChare::forwardNewForcingFile(int job_index, int num_forc_steps, int iFile) {
  if (job_index < 0 || static_cast<size_t>(job_index) >= job_to_worker_.size()) {
    return;
  }
  const int worker_id = job_to_worker_[job_index];
  if (worker_id < 0) {
    return;
  }
  gru_worker_array_[worker_id].newForcingFile(job_index, num_forc_steps, iFile);
}

void JobChare::forwardSetNumStepsBeforeWrite(int job_index, int num_steps) {
  std::vector<int> one_job(1, job_index);
  forwardSetNumStepsBeforeWriteBatch(one_job, num_steps);
}

void JobChare::forwardSetNumStepsBeforeWriteBatch(
    std::vector<int> job_indices, int num_steps)
{
  if (job_indices.empty() || num_workers_ <= 0)
  {
    return;
  }

  std::vector<std::vector<int>> jobs_by_worker(
      static_cast<size_t>(num_workers_));
  for (int job_index : job_indices)
  {
    if (job_index < 0 || static_cast<size_t>(job_index) >= job_to_worker_.size())
    {
      continue;
    }
    const int worker_id = job_to_worker_[job_index];
    if (worker_id < 0 || worker_id >= num_workers_)
    {
      continue;
    }
    jobs_by_worker[static_cast<size_t>(worker_id)].push_back(job_index);
  }

  for (int worker_id = 0; worker_id < num_workers_; ++worker_id)
  {
    std::vector<int> &jobs = jobs_by_worker[static_cast<size_t>(worker_id)];
    if (jobs.empty())
    {
      continue;
    }
    gru_worker_array_[worker_id].setNumStepsBeforeWriteBatch(jobs, num_steps);
  }
}

void JobChare::handleGruChareError(int job_index, int timestep, int err_code,
                                   std::string err_msg)
{
  (job_index == 0) ? handleFileAccessError(err_code, err_msg) : handleGRUError(err_code, job_index, timestep, err_msg);
}

// ------------------------ERROR HANDLING FUNCTIONS ------------------------
void JobChare::handleGRUError(int err_code, int job_index, int timestep,
                              std::string err_msg)
{
  if (!gru_struc_) {
    return;
  }
  auto *gru = gru_struc_->getGRU(job_index);
  if (!gru) {
    return;
  }
  if (gru->getStatus() == gru_state::failed ||
      gru->getStatus() == gru_state::succeeded) {
    return;
  }
  gru->setFailed();
  gru_struc_->incrementNumGruFailed();
  file_access_chare_.runFailure(job_index);
  int resolved_worker_id = -1;
  if (job_index > 0 && static_cast<size_t>(job_index) < job_to_worker_.size())
  {
    resolved_worker_id = job_to_worker_[job_index];
  }
  if (resolved_worker_id >= 0 &&
      static_cast<size_t>(resolved_worker_id) < inflight_tasks_per_worker_.size() &&
      inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)] > 0)
  {
    inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)]--;
  }
  if (job_index > 0 && static_cast<size_t>(job_index) < job_to_worker_.size())
  {
    job_to_worker_[job_index] = -1;
  }
  if (gru_struc_->isDone())
  {
    gru_struc_->hasFailures() && gru_struc_->shouldRetry() ? restartFailures() : finalizeJob();
    return;
  }

  if (resolved_worker_id >= 0 &&
      static_cast<size_t>(resolved_worker_id) < inflight_tasks_per_worker_.size())
  {
    while (inflight_tasks_per_worker_[static_cast<size_t>(resolved_worker_id)] <
           worker_prefetch_depth_effective_)
    {
      if (!assignNextTask(resolved_worker_id))
      {
        break;
      }
    }
  }
}

void JobChare::handleFileAccessError(int err_code, std::string err_msg)
{
  logger_->log("JobChare: File_Access_Chare Error:" + err_msg);
  CkPrintf("JobChare: File_Access_Chare Error: %s\n", err_msg.c_str());
  if (err_code != -1)
  {
    logger_->log("JobChare: Have to Quit");
    CkPrintf("JobChare: Have to Quit");
    return;
  }
}
