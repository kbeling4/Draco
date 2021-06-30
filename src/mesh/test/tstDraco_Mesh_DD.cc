//--------------------------------------------*-C++-*---------------------------------------------//
/*!
 * \file   mesh/test/tstDraco_Mesh_DD.cc
 * \author Ryan Wollaeger <wollaeger@lanl.gov>
 * \date   Sunday, Jun 24, 2018, 14:38 pm
 * \brief  Draco_Mesh class unit test.
 * \note   Copyright (C) 2018-2020 Triad National Security, LLC., All rights reserved. */
//------------------------------------------------------------------------------------------------//

#include "Test_Mesh_Interface.hh"
#include "c4/ParallelUnitTest.hh"
#include "ds++/Release.hh"

using rtt_mesh::Draco_Mesh;
using rtt_mesh_test::Test_Mesh_Interface;

//------------------------------------------------------------------------------------------------//
// TESTS
//------------------------------------------------------------------------------------------------//

// 2D Cartesian domain-decomposed mesh construction test
void cartesian_mesh_2d_dd(rtt_c4::ParallelUnitTest &ut) {

  Insist(rtt_c4::nodes() == 2, "This test only uses 2 PE.");

  // use Cartesian geometry
  const Draco_Mesh::Geometry geometry = Draco_Mesh::Geometry::CARTESIAN;

  //>>> SET UP CELL AND NODE DATA

  // set the number of cells and nodes
  const size_t num_xdir = 1;
  const size_t num_ydir = 1;

  // generate a constainer for data needed in mesh construction
  std::shared_ptr<Test_Mesh_Interface> mesh_iface;
  if (rtt_c4::node() == 0) {
    std::vector<unsigned> global_node_number = {0, 1, 3, 4};
    mesh_iface = std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number);
  } else {
    std::vector<unsigned> global_node_number = {1, 2, 4, 5};
    mesh_iface =
        std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number, 1.0, 0.0);
  }

  // set ghost data
  std::vector<unsigned> ghost_cell_type = {2};
  std::vector<int> ghost_cell_number = {0};
  std::vector<unsigned> ghost_cell_to_node_linkage(2);
  std::vector<int> ghost_cell_rank(1);
  if (rtt_c4::node() == 0) {
    ghost_cell_to_node_linkage = {1, 3};
    ghost_cell_rank = {1};
  } else {
    ghost_cell_to_node_linkage = {2, 0};
    ghost_cell_rank = {0};
  }

  // short-cut to some arrays
  const std::vector<unsigned> &cell_type = mesh_iface->cell_type;
  const std::vector<unsigned> &cell_to_node_linkage = mesh_iface->cell_to_node_linkage;
  const std::vector<unsigned> &side_node_count = mesh_iface->side_node_count;
  const std::vector<unsigned> &side_to_node_linkage = mesh_iface->side_to_node_linkage;

  // instantiate the mesh
  std::shared_ptr<Draco_Mesh> mesh(new Draco_Mesh(
      mesh_iface->dim, geometry, cell_type, cell_to_node_linkage, mesh_iface->side_set_flag,
      side_node_count, side_to_node_linkage, mesh_iface->coordinates,
      mesh_iface->global_node_number, mesh_iface->face_type, ghost_cell_type,
      ghost_cell_to_node_linkage, ghost_cell_number, ghost_cell_rank));

  // check that the scalar data is correct
  FAIL_IF_NOT(mesh->get_dimension() == 2);
  FAIL_IF_NOT(mesh->get_geometry() == Draco_Mesh::Geometry::CARTESIAN);
  FAIL_IF_NOT(mesh->get_num_cells() == mesh_iface->num_cells);
  FAIL_IF_NOT(mesh->get_num_nodes() == mesh_iface->num_nodes);

  // check that the vector data is correct
  FAIL_IF_NOT(mesh->get_ghost_cell_numbers() == ghost_cell_number);
  FAIL_IF_NOT(mesh->get_ghost_cell_ranks() == ghost_cell_rank);

  // get the layout generated by the mesh
  const Draco_Mesh::Layout layout = mesh->get_cc_linkage();

  // cell-to-cell linkage on each node (1 cell on each node, 0 on-node nhbrs)
  FAIL_IF_NOT(layout.size() == 0);

  // get the boundary layout generated by the mesh
  const Draco_Mesh::Layout bd_layout = mesh->get_cs_linkage();

  // check that the boundary (or side) layout has been generated
  FAIL_IF_NOT(bd_layout.size() == mesh_iface->num_cells);

  // get the ghost layout generated by the mesh
  const Draco_Mesh::Layout go_layout = mesh->get_cg_linkage();

  // check that the ghost cell layout has been generated
  FAIL_IF_NOT(go_layout.size() == mesh_iface->num_cells);

  // check that cell-to-node linkage data is correct
  {
    std::vector<unsigned> test_cn_linkage =
        mesh_iface->flatten_cn_linkage(layout, bd_layout, go_layout);

    // check that cn_linkage is a permutation of the original cell-node linkage
    auto cn_first = cell_to_node_linkage.begin();
    auto test_cn_first = test_cn_linkage.begin();
    for (unsigned cell = 0; cell < mesh_iface->num_cells; ++cell) {

      // nodes must only be permuted at the cell level
      FAIL_IF_NOT(std::is_permutation(test_cn_first, test_cn_first + cell_type[cell], cn_first,
                                      cn_first + cell_type[cell]));

      // update the iterators
      cn_first += cell_type[cell];
      test_cn_first += cell_type[cell];
    }
  }

  // check that ghost-cell-to-node linkage data is correct
  {
    std::vector<unsigned> test_gn_linkage = mesh_iface->flatten_sn_linkage(go_layout);

    // check that cn_linkage is a permutation of the original cell-node linkage
    std::vector<unsigned>::const_iterator gn_first = ghost_cell_to_node_linkage.begin();
    std::vector<unsigned>::const_iterator test_gn_first = test_gn_linkage.begin();
    size_t const num_ghost_cells = ghost_cell_type.size();
    for (unsigned ghost = 0; ghost < num_ghost_cells; ++ghost) {

      // check that sn_linkage is a permutation of original ghost-node linkage
      FAIL_IF_NOT(std::is_permutation(test_gn_first, test_gn_first + ghost_cell_type[ghost],
                                      gn_first, gn_first + ghost_cell_type[ghost]));

      // update the iterators
      gn_first += ghost_cell_type[ghost];
      test_gn_first += ghost_cell_type[ghost];
    }
  }

  // check that node-to-ghost-cell linkage is correct
  {
    // access the layout
    const Draco_Mesh::Dual_Ghost_Layout ngc_layout = mesh->get_ngc_linkage();

    // check size (should be number of nodes per rank on processor boundaries)
    FAIL_IF_NOT(ngc_layout.size() == 2);

    // check sizes per node and data
    if (rtt_c4::node() == 0) {

      // check sizes at local node indices on right face (only one ghost cell overall)
      FAIL_IF_NOT(ngc_layout.at(1).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(3).size() == 1);

      // check that local cell index on other rank is 0 (there is only one)
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.first == 0);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower right node (index 1) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[1] == 2);
      // ... upper right node (index 3) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[1] == 3);

      // check that the other rank is rank 1
      FAIL_IF_NOT(ngc_layout.at(1)[0].second == 1);
      FAIL_IF_NOT(ngc_layout.at(3)[0].second == 1);

    } else {

      // check sizes at local node indices on right face (only one ghost cell overall)
      FAIL_IF_NOT(ngc_layout.at(0).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(2).size() == 1);

      // check that local cell index on other rank is 0 (there is only one)
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.first == 0);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower left node (index 0) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[1] == 0);
      // ... upper left node (index 2) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[1] == 1);

      // check that the other rank is rank 0
      FAIL_IF_NOT(ngc_layout.at(0)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[0].second == 0);
    }
  }

  // successful test output
  if (ut.numFails == 0)
    PASSMSG("2D domain-decomposed Draco_Mesh tests ok.");
  return;
}

// test 2D dual layouts in 4 cell mesh decomposed on 4 ranks
void dual_layout_2d_dd_4pe(rtt_c4::ParallelUnitTest &ut) {

  Insist(rtt_c4::nodes() == 4, "This test only uses 4 PE.");

  // use Cartesian geometry
  const Draco_Mesh::Geometry geometry = Draco_Mesh::Geometry::CARTESIAN;

  //>>> SET UP CELL AND NODE DATA

  // set the number of cells and nodes
  const size_t num_xdir = 1;
  const size_t num_ydir = 1;

  // generate a constainer for data needed in mesh construction
  std::shared_ptr<Test_Mesh_Interface> mesh_iface;
  if (rtt_c4::node() == 0) {
    std::vector<unsigned> global_node_number = {0, 1, 3, 4};
    mesh_iface = std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number);
  } else if (rtt_c4::node() == 1) {
    std::vector<unsigned> global_node_number = {1, 2, 4, 5};
    mesh_iface =
        std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number, 1.0, 0.0);
  } else if (rtt_c4::node() == 2) {
    std::vector<unsigned> global_node_number = {3, 4, 6, 7};
    mesh_iface =
        std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number, 0.0, 1.0);
  } else {
    std::vector<unsigned> global_node_number = {4, 5, 7, 8};
    mesh_iface =
        std::make_shared<Test_Mesh_Interface>(num_xdir, num_ydir, global_node_number, 1.0, 1.0);
  }

  // set ghost data
  std::vector<unsigned> ghost_cell_type = {2, 2};
  std::vector<int> ghost_cell_number = {0, 0};
  std::vector<unsigned> ghost_cell_to_node_linkage(4);
  std::vector<int> ghost_cell_rank(2);
  if (rtt_c4::node() == 0) {
    ghost_cell_to_node_linkage = {1, 3, 3, 2};
    ghost_cell_rank = {1, 2};
  } else if (rtt_c4::node() == 1) {
    ghost_cell_to_node_linkage = {2, 0, 2, 3};
    ghost_cell_rank = {0, 3};
  } else if (rtt_c4::node() == 2) {
    ghost_cell_to_node_linkage = {0, 1, 1, 3};
    ghost_cell_rank = {0, 3};
  } else {
    ghost_cell_to_node_linkage = {0, 1, 2, 0};
    ghost_cell_rank = {1, 2};
  }

  // short-cut to some arrays
  const std::vector<unsigned> &cell_type = mesh_iface->cell_type;
  const std::vector<unsigned> &cell_to_node_linkage = mesh_iface->cell_to_node_linkage;
  const std::vector<unsigned> &side_node_count = mesh_iface->side_node_count;
  const std::vector<unsigned> &side_to_node_linkage = mesh_iface->side_to_node_linkage;

  // instantiate the mesh
  std::shared_ptr<Draco_Mesh> mesh(new Draco_Mesh(
      mesh_iface->dim, geometry, cell_type, cell_to_node_linkage, mesh_iface->side_set_flag,
      side_node_count, side_to_node_linkage, mesh_iface->coordinates,
      mesh_iface->global_node_number, mesh_iface->face_type, ghost_cell_type,
      ghost_cell_to_node_linkage, ghost_cell_number, ghost_cell_rank));

  // check that the scalar data is correct
  FAIL_IF_NOT(mesh->get_dimension() == 2);
  FAIL_IF_NOT(mesh->get_geometry() == Draco_Mesh::Geometry::CARTESIAN);
  FAIL_IF_NOT(mesh->get_num_cells() == mesh_iface->num_cells);
  FAIL_IF_NOT(mesh->get_num_nodes() == mesh_iface->num_nodes);

  // check that the vector data is correct
  FAIL_IF_NOT(mesh->get_ghost_cell_numbers() == ghost_cell_number);
  FAIL_IF_NOT(mesh->get_ghost_cell_ranks() == ghost_cell_rank);

  // get the layout generated by the mesh
  const Draco_Mesh::Layout layout = mesh->get_cc_linkage();

  // cell-to-cell linkage on each node (1 cell on each node, 0 on-node nhbrs)
  FAIL_IF_NOT(layout.size() == 0);

  // get the boundary layout generated by the mesh
  const Draco_Mesh::Layout bd_layout = mesh->get_cs_linkage();

  // check that the boundary (or side) layout has been generated
  FAIL_IF_NOT(bd_layout.size() == mesh_iface->num_cells);

  // get the ghost layout generated by the mesh
  const Draco_Mesh::Layout go_layout = mesh->get_cg_linkage();

  // check that the ghost cell layout has been generated
  FAIL_IF_NOT(go_layout.size() == mesh_iface->num_cells);

  // check that cell-to-node linkage data is correct
  {
    std::vector<unsigned> test_cn_linkage =
        mesh_iface->flatten_cn_linkage(layout, bd_layout, go_layout);

    // check that cn_linkage is a permutation of the original cell-node linkage
    auto cn_first = cell_to_node_linkage.begin();
    auto test_cn_first = test_cn_linkage.begin();
    for (unsigned cell = 0; cell < mesh_iface->num_cells; ++cell) {

      // nodes must only be permuted at the cell level
      FAIL_IF_NOT(std::is_permutation(test_cn_first, test_cn_first + cell_type[cell], cn_first,
                                      cn_first + cell_type[cell]));

      // update the iterators
      cn_first += cell_type[cell];
      test_cn_first += cell_type[cell];
    }
  }

  // check that ghost-cell-to-node linkage data is correct
  {
    std::vector<unsigned> test_gn_linkage = mesh_iface->flatten_sn_linkage(go_layout);

    // check that cn_linkage is a permutation of the original cell-node linkage
    std::vector<unsigned>::const_iterator gn_first = ghost_cell_to_node_linkage.begin();
    std::vector<unsigned>::const_iterator test_gn_first = test_gn_linkage.begin();
    size_t const num_ghost_cells = ghost_cell_type.size();
    for (unsigned ghost = 0; ghost < num_ghost_cells; ++ghost) {

      // check that sn_linkage is a permutation of original ghost-node linkage
      FAIL_IF_NOT(std::is_permutation(test_gn_first, test_gn_first + ghost_cell_type[ghost],
                                      gn_first, gn_first + ghost_cell_type[ghost]));

      // update the iterators
      gn_first += ghost_cell_type[ghost];
      test_gn_first += ghost_cell_type[ghost];
    }
  }

  // check that node-to-ghost-cell linkage is correct
  {
    // access the layout
    const Draco_Mesh::Dual_Ghost_Layout ngc_layout = mesh->get_ngc_linkage();

    // check size (should be number of nodes per rank on processor boundaries)
    FAIL_IF_NOT(ngc_layout.size() == 3);

    // check sizes per node and data
    if (rtt_c4::node() == 0) {

      // check sizes at local node indices
      FAIL_IF_NOT(ngc_layout.at(1).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(3).size() == 3);
      FAIL_IF_NOT(ngc_layout.at(2).size() == 1);

      // check local cell index on other rank
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[1].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[2].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.first == 0);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower right node (index 1) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[1] == 2);
      // ... upper right node (index 3) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[1] == 3);
      // ... upper right node (index 3) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(3)[1].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(3)[1].first.second[1] == 0);
      // ... upper right node (index 3) to rank 3 cell
      FAIL_IF_NOT(ngc_layout.at(3)[2].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(3)[2].first.second[1] == 2);
      // ... upper left node (index 2) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[1] == 2);

      // check the other ranks
      FAIL_IF_NOT(ngc_layout.at(1)[0].second == 1);
      FAIL_IF_NOT(ngc_layout.at(3)[0].second == 1);
      FAIL_IF_NOT(ngc_layout.at(3)[1].second == 2);
      FAIL_IF_NOT(ngc_layout.at(3)[2].second == 3);
      FAIL_IF_NOT(ngc_layout.at(2)[0].second == 2);

    } else if (rtt_c4::node() == 1) {

      // check sizes at local node indices
      FAIL_IF_NOT(ngc_layout.at(0).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(2).size() == 3);
      FAIL_IF_NOT(ngc_layout.at(3).size() == 1);

      // check local cell index on other rank
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[1].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[2].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.first == 0);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower left node (index 0) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[1] == 0);
      // ... upper left node (index 2) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[1] == 1);
      // ... upper left node (index 2) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(2)[1].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(2)[1].first.second[1] == 0);
      // ... upper left node (index 2) to rank 3 cell
      FAIL_IF_NOT(ngc_layout.at(2)[2].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(2)[2].first.second[1] == 2);
      // ... upper right node (index 3) to rank 3 cell
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[1] == 0);

      // check the other ranks
      FAIL_IF_NOT(ngc_layout.at(0)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(2)[1].second == 2);
      FAIL_IF_NOT(ngc_layout.at(2)[2].second == 3);
      FAIL_IF_NOT(ngc_layout.at(3)[0].second == 3);

    } else if (rtt_c4::node() == 2) {

      // check sizes at local node indices
      FAIL_IF_NOT(ngc_layout.at(0).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(1).size() == 3);
      FAIL_IF_NOT(ngc_layout.at(3).size() == 1);

      // check local cell index on other rank
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[1].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[2].first.first == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.first == 0);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower left node (index 0) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[1] == 3);
      // ... lower right node (index 1) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[1] == 1);
      // ... lower right node (index 1) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(1)[1].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[1].first.second[1] == 3);
      // ... lower right node (index 1) to rank 3 cell
      FAIL_IF_NOT(ngc_layout.at(1)[2].first.second[0] == 1);
      FAIL_IF_NOT(ngc_layout.at(1)[2].first.second[1] == 2);
      // ... upper right node (index 3) to rank 3 cell
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(3)[0].first.second[1] == 3);

      // check the other ranks
      FAIL_IF_NOT(ngc_layout.at(0)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(1)[1].second == 1);
      FAIL_IF_NOT(ngc_layout.at(1)[2].second == 3);
      FAIL_IF_NOT(ngc_layout.at(3)[0].second == 3);

    } else if (rtt_c4::node() == 3) {

      // check sizes at local node indices
      FAIL_IF_NOT(ngc_layout.at(0).size() == 3);
      FAIL_IF_NOT(ngc_layout.at(1).size() == 1);
      FAIL_IF_NOT(ngc_layout.at(2).size() == 1);

      // check nodes (with indexing local to other rank) adjacent to layout nodes ...
      // ... lower left node (index 0) to rank 0 cell
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(0)[0].first.second[1] == 1);
      // ... lower left node (index 0) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(0)[1].first.second[0] == 0);
      FAIL_IF_NOT(ngc_layout.at(0)[1].first.second[1] == 3);
      // ... lower left node (index 0) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(0)[2].first.second[0] == 3);
      FAIL_IF_NOT(ngc_layout.at(0)[2].first.second[1] == 0);
      // ... lower right node (index 1) to rank 1 cell
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(1)[0].first.second[1] == 1);
      // ... upper right node (index 2) to rank 2 cell
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[0] == 2);
      FAIL_IF_NOT(ngc_layout.at(2)[0].first.second[1] == 1);

      // check the other ranks
      FAIL_IF_NOT(ngc_layout.at(0)[0].second == 0);
      FAIL_IF_NOT(ngc_layout.at(0)[1].second == 1);
      FAIL_IF_NOT(ngc_layout.at(0)[2].second == 2);
      FAIL_IF_NOT(ngc_layout.at(1)[0].second == 1);
      FAIL_IF_NOT(ngc_layout.at(2)[0].second == 2);
    }
  }

  // successful test output
  if (ut.numFails == 0)
    PASSMSG("2D domain-decomposed Draco_Mesh tests ok.");
  return;
}

//------------------------------------------------------------------------------------------------//

int main(int argc, char *argv[]) {
  rtt_c4::ParallelUnitTest ut(argc, argv, rtt_dsxx::release);
  try {
    if (rtt_c4::nodes() == 2) {
      cartesian_mesh_2d_dd(ut);
    } else if (rtt_c4::nodes() == 4) {
      dual_layout_2d_dd_4pe(ut);
    } else {
      Insist(false, "This test only uses 2 or 4 PE.");
    }
  }
  UT_EPILOG(ut);
}

//------------------------------------------------------------------------------------------------//
// end of mesh/test/tstDraco_Mesh_DD.cc
//------------------------------------------------------------------------------------------------//
