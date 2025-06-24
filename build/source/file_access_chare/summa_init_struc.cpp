#include "summa_init_struc.hpp"
#include <memory>
#include <iostream>

int SummaInitStruc::allocate(int num_gru) {
  int err = 0;
  void *err_msg_ptr = nullptr;
  f_allocate(num_gru, err, &err_msg_ptr);
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::allocate() - ";
    if (err_msg_ptr) {
      const char *msg = static_cast<const char *>(err_msg_ptr);
      std::cout << msg << std::endl;
    } else {
      std::cout << "Unknown error" << std::endl;
    }
  }
  return err;
}

int SummaInitStruc::summa_paramSetup() {
  int err = 0;
  void *err_msg_ptr = nullptr;
  f_paramSetup(err, &err_msg_ptr);
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::summa_paramSetup() - ";
    if (err_msg_ptr) {
      const char *msg = static_cast<const char *>(err_msg_ptr);
      std::cout << msg << std::endl;
    } else {
      std::cout << "Unknown error" << std::endl;
    }
  }
  return err;
}

int SummaInitStruc::summa_readRestart() {
  int err = 0;
  void *err_msg_ptr = nullptr;
  f_readRestart(err, &err_msg_ptr);
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::summa_readRestart() - ";
    if (err_msg_ptr) {
      const char *msg = static_cast<const char *>(err_msg_ptr);
      std::cout << msg << std::endl;
    } else {
      std::cout << "Unknown error" << std::endl;
    }
  }
  return err;
}

void SummaInitStruc::getInitTolerance(double rel_tol, double abs_tol) {
  f_getInitTolerance(rel_tol, abs_tol);
}
