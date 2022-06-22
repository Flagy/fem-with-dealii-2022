#include "step-3.h"

int
main()
{
  dealii::deallog.depth_console(2);
  Step3 laplace_problem;
  laplace_problem.run();
  return 0;
}