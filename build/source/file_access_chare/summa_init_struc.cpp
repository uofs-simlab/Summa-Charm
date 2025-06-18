#include "summa_init_struc.hpp"
#include <memory>
#include <iostream>

int SummaInitStruc::allocate(int num_gru) {
  int err = 0;
  std::unique_ptr<char[]> err_msg(new char[1024]);
  f_allocate(num_gru, err, err_msg.get());
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::allocate() - " << err_msg.get() << std::endl;
  }
  return err;
}

int SummaInitStruc::summa_paramSetup() {
  int err = 0;
  std::unique_ptr<char[]> err_msg(new char[1024]);
  f_paramSetup(err, err_msg.get());
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::summa_paramSetup() - " << err_msg.get() << std::endl;
  }
  return err;
}

int SummaInitStruc::summa_readRestart() {
  int err = 0;
  std::unique_ptr<char[]> err_msg(new char[1024]);
  f_readRestart(err, err_msg.get());
  if (err != 0) {
    std::cout << "ERROR: SummaInitStruc::summa_readRestart() - " << err_msg.get() << std::endl;
  }
  return err;
}

void SummaInitStruc::getInitTolerance(double rel_tol, double abs_tol) {
  f_getInitTolerance(rel_tol, abs_tol);
}
