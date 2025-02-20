/* ---------------------------------------------------------------------
 *
 * Copyright (C) 1999 - 2020 by the deal.II authors
 *
 * This file is part of the deal.II library.
 *
 * The deal.II library is free software; you can use it, redistribute
 * it, and/or modify it under the terms of the GNU Lesser General
 * Public License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * The full text of the license can be found in the file LICENSE.md at
 * the top level directory of deal.II.
 *
 * ---------------------------------------------------------------------
 *
 * Authors: Wolfgang Bangerth, 1999,
 *          Guido Kanschat, 2011
 *          Luca Heltai, 2021
 */
#include "step-3.h"

#include <deal.II/base/function.h>
#include <deal.II/base/quadrature_lib.h>

#include <deal.II/dofs/dof_tools.h>

#include <deal.II/fe/fe_values.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/grid_out.h>

#include <deal.II/lac/full_matrix.h>
#include <deal.II/lac/precondition.h>
#include <deal.II/lac/solver_cg.h>
#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>
#include <deal.II/numerics/matrix_tools.h>
#include <deal.II/numerics/vector_tools.h>

#include <fstream>
#include <iostream>

using namespace dealii;

Step3::Step3()
  : fe(1)
  , dof_handler(triangulation)
{}



void
Step3::make_grid()
{
  GridGenerator::hyper_cube(triangulation, -1, 1);
  // triangulation.begin_active()->face(0)->set_boundary_id(1);
  std::cout << "Number of active cells: " << triangulation.n_active_cells()
            << std::endl;
  const Point<2> center(0.0, 0.0);

  for (unsigned int step = 0; step < 6; ++step)
    {
      for (auto &cell : triangulation.active_cell_iterators())
        {
          if (std::fabs(center.distance(cell->center())) < 0.8)
            {
              cell->set_refine_flag();
            }
        }
      triangulation.execute_coarsening_and_refinement();
      std::ofstream out("grid-2-" + std::to_string(step) + ".vtk");
      GridOut       grid_out;
      grid_out.write_vtk(triangulation, out);
      std::cout << "Grid written to grid-3-" + std::to_string(step) + ".vtk"
                << std::endl;
    }
}


void
Step3::setup_system()
{
  dof_handler.distribute_dofs(fe);
  std::cout << "Number of degrees of freedom: " << dof_handler.n_dofs()
            << std::endl;

  constraints.clear();
  DoFTools::make_hanging_node_constraints(dof_handler, constraints);

  VectorTools::interpolate_boundary_values(dof_handler,
                                           0,
                                           Functions::ConstantFunction<2>(-0.5),
                                           constraints);

  constraints.close();

  DynamicSparsityPattern dsp(dof_handler.n_dofs());
  DoFTools::make_sparsity_pattern(dof_handler, dsp, constraints);
  sparsity_pattern.copy_from(dsp);
  system_matrix.reinit(sparsity_pattern);
  solution.reinit(dof_handler.n_dofs());
  system_rhs.reinit(dof_handler.n_dofs());
}



void
Step3::assemble_system()
{
  QGauss<2>          quadrature_formula(fe.degree + 1);
  FEValues<2>        fe_values(fe,
                        quadrature_formula,
                        update_values | update_gradients | update_JxW_values);
  const unsigned int dofs_per_cell = fe.n_dofs_per_cell();
  FullMatrix<double> cell_matrix(dofs_per_cell, dofs_per_cell);
  Vector<double>     cell_rhs(dofs_per_cell);
  std::vector<types::global_dof_index> local_dof_indices(dofs_per_cell);
  for (const auto &cell : dof_handler.active_cell_iterators())
    {
      fe_values.reinit(cell);
      cell_matrix = 0;
      cell_rhs    = 0;
      for (const unsigned int q_index : fe_values.quadrature_point_indices())
        {
          for (const unsigned int i : fe_values.dof_indices())
            for (const unsigned int j : fe_values.dof_indices())
              cell_matrix(i, j) +=
                (fe_values.shape_grad(i, q_index) * // grad phi_i(x_q)
                 fe_values.shape_grad(j, q_index) * // grad phi_j(x_q)
                 fe_values.JxW(q_index));           // dx
          for (const unsigned int i : fe_values.dof_indices())
            cell_rhs(i) += (fe_values.shape_value(i, q_index) * // phi_i(x_q)
                            1.5 *                               // f(x_q)
                            fe_values.JxW(q_index));            // dx
        }
      cell->get_dof_indices(local_dof_indices);
      for (const unsigned int i : fe_values.dof_indices())
        for (const unsigned int j : fe_values.dof_indices())
          system_matrix.add(local_dof_indices[i],
                            local_dof_indices[j],
                            cell_matrix(i, j));
      for (const unsigned int i : fe_values.dof_indices())
        system_rhs(local_dof_indices[i]) += cell_rhs(i);
      cell->get_dof_indices(local_dof_indices);
      constraints.distribute_local_to_global(
        cell_matrix, cell_rhs, local_dof_indices, system_matrix, system_rhs);
    }



  // std::map<types::global_dof_index, double> boundary_values;
  // VectorTools::interpolate_boundary_values(dof_handler,
  //                                          1,
  //                                          ConstantFunction<2>(-0.5),
  //                                          boundary_values);

  // MatrixTools::apply_boundary_values(boundary_values,
  //                                    system_matrix,
  //                                    solution,
  //                                    system_rhs);
}



void
Step3::solve()
{
  SolverControl            solver_control(1000, 1e-12);
  SolverCG<Vector<double>> solver(solver_control);
  solver.solve(system_matrix, solution, system_rhs, PreconditionIdentity());
  constraints.distribute(solution);
}



void
Step3::output_results() const
{
  DataOut<2> data_out;
  data_out.attach_dof_handler(dof_handler);
  data_out.add_data_vector(solution, "solution");
  data_out.build_patches();
  std::ofstream output("solution.vtk");
  data_out.write_vtk(output);
}



void
Step3::run()
{
  make_grid();
  setup_system();
  assemble_system();
  solve();
  output_results();
}
