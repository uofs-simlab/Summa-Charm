#include "mainChare.decl.h"
#include "settings_functions.hpp"
#include "summa_chare.hpp"
#include "job_chare.hpp"
#include "gru_chare.hpp"
#include "file_access_chare.hpp"
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

CProxy_Main mainProxy;

const std::string command_line_help =
    "Summa-Chares is in active development and some features may not be "
    "available.\n"
    "Usage: summa_chares -m master_file [-g startGRU countGRU] [-c "
    "config_file]\n"
    "Available options: \n"
    "\t-m, --master:         Define path/name of master file (can be specified "
    "in "
    "config)\n"
    "\t-g, --gru:            Run a subset of countGRU GRUs starting from index "
    "startGRU "
    "\n"
    "\t-c, --config:         Path name of the Summa-Chares config file "
    "(optional but "
    "recommended)\n"
    "\t-s  --suffix          Add fileSuffix to the output files\n"
    "\t    --gen-config:     Generate a config file \n"
    "\t-h, --help:           Print this help message \n"
    "\nUnimplemented Options: \n"
    "\t    --host:           Hostname of the server \n"
    "\t-b, --backup-server:  Start backup server, requires a server and "
    "config_file \n"
    "\t    --server-mode:    Enable server mode \n"
    "\t-n, --newFile         Define frequency [noNewFiles,newFileEveryOct1] of "
    "new "
    "output files\n"
    "\t-h, --hru             Run a single HRU with index of iHRU\n"
    "\t-r, --restart         Define frequency [y,m,d,e,never] to write restart "
    "files\n"
    "\t-p, --progress        Define frequency [m,d,h,never] to print progress\n"
    "\t-v, --version         Display version information of the current "
    "build\n";

class Main : public CBase_Main {
public:
  int startGRU = -1;
  int countGRU = -1;
  std::string master_file = "";
  std::string config_file = "";
  std::string output_file_suffix = "";
  bool generate_config = false;
  bool help = false;

  Main(CkArgMsg *m) {
    mainProxy = thisProxy;
    // Parse command-line arguments
    for (int it = 1; it < m->argc; ++it) {
      if ((strcmp(m->argv[it], "-m") == 0 ||
           strcmp(m->argv[it], "--master") == 0) &&
          it + 1 < m->argc) {
        master_file = m->argv[++it];
      } else if ((strcmp(m->argv[it], "-g") == 0 ||
                  strcmp(m->argv[it], "--gru") == 0) &&
                 it + 2 < m->argc) {
        startGRU = atoi(m->argv[++it]);
        countGRU = atoi(m->argv[++it]);
      } else if ((strcmp(m->argv[it], "-c") == 0 ||
                  strcmp(m->argv[it], "--config") == 0) &&
                 it + 1 < m->argc) {
        config_file = m->argv[++it];
      } else if (strcmp(m->argv[it], "-h") == 0 ||
                 strcmp(m->argv[it], "--help") == 0) {
        std::cout << command_line_help << std::endl;
        CkExit();
      }
    }

    Settings settings = Settings(config_file);
    if (generate_config) {
      settings.generateConfigFile();
      CkExit();
    }

    // Check if the master file was if not check if the config file was
    // specified
    if (!std::ifstream(master_file)) {
      if (!std::ifstream(config_file)) {
        CkPrintf("\n\n**** Config (-c) or Master File (-m) Does Not Exist or"
                 "Not Specified!! ****\n\nConfig File: %s \nMaster File: %s"
                 "\n\n%s",
                 config_file.c_str(), master_file.c_str(),
                 command_line_help.c_str());
        CkExit();
      }
    }

    // start the SummaChare with the given settings
    CProxy_SummaChare summaChareProxy = CProxy_SummaChare::ckNew(
        startGRU, countGRU, config_file, master_file, output_file_suffix);

    // Note: SummaChare will call finishExecution() when it's done
  }

  // Entry method called when simulation is complete
  void done() {
    CkExit();
  }
};

#include "mainChare.def.h"
#include "SummaChare.def.h"
#include "JobChare.def.h"
#include "GruChare.def.h"
#include "FileAccessChare.def.h"
