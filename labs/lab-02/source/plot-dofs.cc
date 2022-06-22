#include <deal.II/base/config.h>

#include <deal.II/dofs/dof_handler.h>

#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/mapping_q_generic.h>

#include <deal.II/grid/grid_generator.h>
#include <deal.II/grid/tria.h>

#include <deal.II/lac/vector.h>

#include <deal.II/numerics/data_out.h>

#include <fstream>

using namespace dealii;

int
main(int argc, char **argv)
{
  unsigned int fe_order      = 0;
  unsigned int mapping_order = 0;
  unsigned int basis_index   = 0;

  if (argc > 1)
    fe_order = std::atoi(argv[2]);
  if (argc > 2)
    mapping_order = std::atoi(argv[2]);
  if (argc > 3)
    basis_index = std::atoi(argv[3]);

  Triangulation<2> triangulation;
  Point<2>         center;
  double           inner_radius = 0.5, outer_radius = 1.0;
  GridGenerator::hyper_shell(
    triangulation, center, inner_radius, outer_radius, 5);

  FE_Q<2>            fe(fe_order);
  MappingQGeneric<2> mapping(mapping_order);

  DoFHandler<2> dof_handler(triangulation);

  dof_handler.distribute_dofs(fe);

  Vector<double> example_basis(dof_handler.n_dofs());
  example_basis              = 0.0;
  example_basis[basis_index] = 1.0;
  DataOut<2> data_out;
  data_out.attach_dof_handler(dof_handler);
  data_out.add_data_vector(example_basis, "example_basis");
  data_out.build_patches(mapping,
                         std::max(fe_order, mapping_order),
                         DataOut<2>::curved_inner_cells);
  std::ofstream outfile("example_basis.vtk");
  data_out.write_vtk(outfile);
}