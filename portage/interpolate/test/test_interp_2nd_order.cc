/*
This file is part of the Ristra portage project.
Please see the license file at the root of this repository, or at:
    https://github.com/laristra/portage/blob/master/LICENSE
*/


#include <iostream>
#include <portage/search/search_kdtree.h>
#include <portage/intersect/intersect_r2d.h>
#include <portage/driver/mmdriver.h>

#include "gtest/gtest.h"

// portage includes
#include "portage/interpolate/interpolate_2nd_order.h"
#include "portage/intersect/simple_intersect_for_tests.h"
#include "portage/support/portage.h"
#include "portage/driver/coredriver.h"

// wonton includes
#include "wonton/mesh/simple/simple_mesh.h"
#include "wonton/mesh/simple/simple_mesh_wrapper.h"
#include "wonton/state/simple/simple_state.h"
#include "wonton/state/simple/simple_state_wrapper.h"
#include "wonton/support/Point.h"

double TOL = 1e-12;

/// Second order interpolation of constant cell-centered field with no
/// limiter in 2D

TEST(Interpolate_2nd_Order, Cell_Ctr_Const_BND_NOLIMITER_2D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(ncells_source, 1.25);
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars");

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  interpolator.set_interpolation_variable("cellvars", &gradients);


  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each cell on the target
  const double stdval = data[0];
  for (int c = 0; c < ncells_target; ++c)
    ASSERT_NEAR(stdval, outvals[c],TOL);

}


/// Second order interpolation of linear cell-centered field with no
/// limiting in 2D

TEST(Interpolate_2nd_Order, Cell_Ctr_Lin_BND_NOLIMITER_2D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

// Define a state vector with linear value and add it to the source state

  std::vector<double> data(ncells_source);
  for (int c = 0; c < ncells_source; ++c) {
    Wonton::Point<2> cen;
    sourceMeshWrapper.cell_centroid(c, &cen);
    data[c] = cen[0]+cen[1];
  }
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars");

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each cell on the target

  std::vector<double> stdvals(ncells_target);
  for (int c = 0; c < ncells_target; ++c) {
    Wonton::Point<2> cen;
    targetMeshWrapper.cell_centroid(c, &cen);
    stdvals[c] = cen[0]+cen[1];
  }

  for (int c = 0; c < ncells_target; ++c)
    ASSERT_NEAR(stdvals[c], outvals[c],TOL);

}


/*!
  @brief Second order interpolate of linear cell-centered field with
  Barth-Jespersen limiting in 2D
 */

TEST(Interpolate_2nd_Order, Cell_Ctr_Lin_BJ_Limiter_2D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with linear value and add it to the source state

  double minval = 1e+10, maxval = -1e+10;

  std::vector<double> data(ncells_source);
  for (int c = 0; c < ncells_source; ++c) {
    Wonton::Point<2> ccen;
    sourceMeshWrapper.cell_centroid(c, &ccen);
    if (ccen[0] < 0.5)
      data[c] = ccen[0]+ccen[1];
    else
      data[c] = 100*ccen[0];
    if (data[c] < minval) minval = data[c];
    if (data[c] > maxval) maxval = data[c];
  }
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals1(ncells_target);
  std::vector<double> outvals2(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>                                
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars", Portage::NOLIMITER);
  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals1.begin(), interpolator);

  gradients = driver.compute_source_gradient("cellvars", Portage::BARTH_JESPERSEN);
  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals2.begin(), interpolator);

  // COULD REMOVE
  // Make sure we retrieved the correct value for each cell on the target
  std::vector<double> stdvals(ncells_target);
  for (int c = 0; c < ncells_target; ++c) {
    Wonton::Point<2> cen;
    targetMeshWrapper.cell_centroid(c, &cen);
    stdvals[c] = cen[0]+cen[1];
  }

  // Check if we violated the bounds on at least one cell in the
  // unlimited interpolation and if we respected the bounds in all
  // cells in the limited case

  bool outofbounds_unlimited = false;
  for (int c = 0; c < ncells_target; ++c) {
    if (outvals1[c] < minval  || outvals1[c] > maxval) {
      outofbounds_unlimited = true;
      break;
    }
  }

  // Check if we preserved bounds on interior cells. Since we don't limit on
  // boundary cells we have no guarantee their values will be within bounds
  bool inbounds_limited = true;
  for (int c = 0; c < ncells_target; ++c) {
    if (!targetMeshWrapper.on_exterior_boundary(Wonton::Entity_kind::CELL, c)) {
      if (outvals2[c] < minval - TOL  || outvals2[c] > maxval + TOL) {
        inbounds_limited = false;
        break;
      }
    }
  }
  EXPECT_TRUE(outofbounds_unlimited && inbounds_limited);

}


/*!
  @brief Second order interpolate of nonlinear cell-centered field with
  Barth-Jespersen inside and no/zero gradient limiting on the boundary of 
  a 2D domain
 */

TEST(Interpolate_2nd_Order, Cell_Ctr_Non_Lin_BJ_Limiter_ZG_Bnd_Limiter_2D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 7, 7);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with nonlinear (exponential) values 
  // and add it to the source state

  double minval = 1e+10, maxval = -1e+10;

  std::vector<double> data(ncells_source);
  for (int c = 0; c < ncells_source; ++c) {
    Wonton::Point<2> ccen;
    sourceMeshWrapper.cell_centroid(c, &ccen);
    data[c] = exp(10*ccen[0]);

    if (data[c] < minval) minval = data[c];
    if (data[c] > maxval) maxval = data[c];
  }
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals1(ncells_target);
  std::vector<double> outvals2(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_cell_coords[c],
                                           source_cell_coords,
                                           &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars",
                                                  Portage::BARTH_JESPERSEN,
                                                  Portage::BND_NOLIMITER);
  interpolator.set_interpolation_variable("cellvars", &gradients);


  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals1.begin(), interpolator);

  // recompute gradient field using correct limiters
  gradients = driver.compute_source_gradient("cellvars",
                                             Portage::BARTH_JESPERSEN,
                                             Portage::BND_ZERO_GRADIENT);
  interpolator.set_interpolation_variable("cellvars", &gradients);


  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals2.begin(), interpolator);

  // Check if we violated the bounds on at least one cell in the
  // unlimited interpolation and if we respected the bounds in all
  // cells in the limited case

  bool outofbounds_unlimited_bnd = false;
  for (int c = 0; c < ncells_target; ++c) {
    if (outvals1[c] < minval  || outvals1[c] > maxval) {
      outofbounds_unlimited_bnd = true;
      break;
    }
  }

  // Check if we preserved bounds on boundary cells as well when using 
  // BND_ZERO_GRADIENT
  bool inbounds_limited_bnd = true;
  for (int c = 0; c < ncells_target; ++c) {
    if (outvals2[c] < minval - TOL  || outvals2[c] > maxval + TOL) {
      inbounds_limited_bnd = false;
      break;
    }
  }
  EXPECT_TRUE(outofbounds_unlimited_bnd && inbounds_limited_bnd);

}

/// Second order interpolation of constant node-centered field with no
/// limiting in 2D

TEST(Interpolate_2nd_Order, Node_Ctr_Const_BND_NOLIMITER) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count nodes

  const int nnodes_source =
      sourceMeshWrapper.num_owned_nodes();
  const int nnodes_target =
      targetMeshWrapper.num_owned_nodes();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(nnodes_source, 1.25);
  source_state.add("nodevars", Wonton::Entity_kind::NODE, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_dualcell_coords(nnodes_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_dualcell_coords(nnodes_target);

  // Actually get the Wonton::Points for the dual cells

  for (int n = 0; n < nnodes_source; ++n)
	  sourceMeshWrapper.dual_cell_get_coordinates(n, &source_dualcell_coords[n]);
  for (int n = 0; n < nnodes_target; ++n)
	  targetMeshWrapper.dual_cell_get_coordinates(n, &target_dualcell_coords[n]);

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(nnodes_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(nnodes_target);

  // Loop over target nodes

  for (int c = 0; c < nnodes_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_dualcell_coords[c],
                                        source_dualcell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::NODE,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("nodevars");

  interpolator.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each node on the
  // target. Second order interpolation should preserve a linear field
  // exactly but node-centered interpolation has some quirks - the field
  // does not get preserved exactly at the boundary because the source
  // values for boundary dual cells are not specified inside the dual
  // cells but at one of their vertices or edges. So, check only
  // interior nodes

  const double stdval = data[0];
  for (int n = 0; n < nnodes_target; ++n) {
    Wonton::Point<2> coord;
    targetMeshWrapper.node_get_coordinates(n, &coord);
    if (fabs(coord[0]) < 1e-16 || fabs(1-coord[0]) < 1e-16 ||
        fabs(coord[1]) < 1e-16 || fabs(1-coord[1]) < 1.e-16)
      continue;
    ASSERT_NEAR(stdval, outvals[n], TOL);
  }


  // Make sure we retrieved the correct value for each cell on the target
  // this block seems to work as well, despite the note above
  // const double stdval = data[0];
  // for (int c = 0; c < nnodes_target; ++c) ASSERT_NEAR(stdval, outvals[c],TOL);

}


// Second order interpolation of linear node-centered field with no
// limiting in 2D

TEST(Interpolate_2nd_Order, Node_Ctr_Lin_BND_NOLIMITER) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 1.0, 1.0, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count nodes

  const int nnodes_source =
      sourceMeshWrapper.num_owned_nodes();
  const int nnodes_target =
      targetMeshWrapper.num_owned_nodes();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(nnodes_source);
  for (int n = 0; n < nnodes_source; ++n) {
    Wonton::Point<2> coord;
    sourceMeshWrapper.node_get_coordinates(n, &coord);
    data[n] = coord[0]+coord[1];
  }
  source_state.add("nodevars", Wonton::Entity_kind::NODE, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<2>>>
      source_dualcell_coords(nnodes_source);
  std::vector<std::vector<Wonton::Point<2>>>
      target_dualcell_coords(nnodes_target);

  // Actually get the Wonton::Points for the dual cells

  for (int n = 0; n < nnodes_source; ++n)
	  sourceMeshWrapper.dual_cell_get_coordinates(n, &source_dualcell_coords[n]);
  for (int n = 0; n < nnodes_target; ++n)
	  targetMeshWrapper.dual_cell_get_coordinates(n, &target_dualcell_coords[n]);

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(nnodes_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(nnodes_target);

  // Loop over target nodes

  for (int c = 0; c < nnodes_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<2>(target_dualcell_coords[c],
                                        source_dualcell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<2>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<2, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<2, Wonton::Entity_kind::NODE,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("nodevars");

  interpolator.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each node on the
  // target. Second order interpolation should preserve a linear field
  // exactly but node-centered interpolation has some quirks - the field
  // does not get preserved exactly at the boundary because the source
  // values for boundary dual cells are not specified inside the dual
  // cells but at one of their vertices or edges. So, check only
  // interior nodes

  std::vector<double> stdvals(nnodes_target);
  for (int n = 0; n < nnodes_target; ++n) {
    Wonton::Point<2> coord;
    targetMeshWrapper.node_get_coordinates(n, &coord);
    if (fabs(coord[0]) < 1e-16 || fabs(1-coord[0]) < 1e-16 ||
        fabs(coord[1]) < 1e-16 || fabs(1-coord[1]) < 1.e-16)
      continue;
    stdvals[n] = coord[0]+coord[1];
    EXPECT_NEAR(stdvals[n], outvals[n], TOL);
  }

}


/// Second order interpolation of constant cell-centered field with no
/// limiting in 3D

TEST(Interpolate_2nd_Order, Cell_Ctr_Const_BND_NOLIMITER_3D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(ncells_source, 1.25);
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<3>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);
    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars");

  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each cell on the target
  const double stdval = data[0];
  for (int c = 0; c < ncells_target; ++c)
    ASSERT_NEAR(stdval, outvals[c],TOL);

}

/// Second order interpolation of linear cell-centered field with no
/// limiting in 3D

TEST(Interpolate_2nd_Order, Cell_Ctr_Lin_BND_NOLIMITER_3D) {
  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

// Define a state vector with linear value and add it to the source state

  std::vector<double> data(ncells_source);
  for (int c = 0; c < ncells_source; ++c) {
    Wonton::Point<3> cen;
    sourceMeshWrapper.cell_centroid(c, &cen);
    data[c] = cen[0]+cen[1]+cen[2];
  }
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<3>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);
    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars");

  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each cell on the target

  std::vector<double> stdvals(ncells_target);
  for (int c = 0; c < ncells_target; ++c) {
    Wonton::Point<3> cen;
    targetMeshWrapper.cell_centroid(c, &cen);
    stdvals[c] = cen[0]+cen[1]+cen[2];
  }

  double TOL = 1e-12;
  for (int c = 0; c < ncells_target; ++c)
    ASSERT_NEAR(stdvals[c], outvals[c], TOL);

}

/// Second order interpolation of discontinuous cell-centered linear field with
/// Barth-Jespersen limiting in 3D

TEST(Interpolate_2nd_Order, Cell_Ctr_BJ_Limiter_3D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);


  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count cells

  const int ncells_source =
      sourceMeshWrapper.num_owned_cells();
  const int ncells_target =
      targetMeshWrapper.num_owned_cells();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with linear value and add it to the source state

  double minval = 1e+10, maxval = -1e+10;

  std::vector<double> data(ncells_source);
  for (int c = 0; c < ncells_source; ++c) {
    Wonton::Point<3> ccen;
    sourceMeshWrapper.cell_centroid(c, &ccen);
    if (ccen[0] < 0.5)
      data[c] = ccen[0]+ccen[1]+ccen[2];
    else
      data[c] = 100*ccen[0];
    if (data[c] < minval) minval = data[c];
    if (data[c] > maxval) maxval = data[c];
  }
  source_state.add("cellvars", Wonton::Entity_kind::CELL, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_cell_coords(ncells_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_cell_coords(ncells_target);

  // Actually get the Wonton::Points

  for (int c = 0; c < ncells_source; ++c)
    sourceMeshWrapper.cell_get_coordinates(c, &(source_cell_coords[c]));
  for (int c = 0; c < ncells_target; ++c)
    targetMeshWrapper.cell_get_coordinates(c, &(target_cell_coords[c]));

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals1(ncells_target);
  std::vector<double> outvals2(ncells_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(ncells_target);

  // Loop over target cells

  for (int c = 0; c < ncells_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<3>(target_cell_coords[c],
                                        source_cell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::CELL,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::CELL,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("cellvars",
                                                  Portage::NOLIMITER,
                                                  Portage::DEFAULT_BND_LIMITER);

  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals1.begin(), interpolator);

  // recompute gradient field using correct limiters
  gradients = driver.compute_source_gradient("cellvars",
                                             Portage::BARTH_JESPERSEN,
                                             Portage::DEFAULT_BND_LIMITER);

  interpolator.set_interpolation_variable("cellvars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::CELL),
                     targetMeshWrapper.end(Wonton::Entity_kind::CELL),
                     sources_and_weights.begin(),
                     outvals2.begin(), interpolator);

  // Check if we violated the bounds on at least one cell in the
  // unlimited interpolation and if we respected the bounds in all
  // cells in the limited case

  bool outofbounds_unlimited = false;
  for (int c = 0; c < ncells_target; ++c) {
    if (outvals1[c] < minval  || outvals1[c] > maxval) {
      outofbounds_unlimited = true;
      break;
    }
  }

  // Check if we preserved bounds on interior cells. Since we don't limit on
  // boundary cells we have no guarantee their values will be within bounds
  bool inbounds_limited = true;
  for (int c = 0; c < ncells_target; ++c) {
    if (!targetMeshWrapper.on_exterior_boundary(Wonton::Entity_kind::CELL, c)) {
      if (outvals2[c] < minval - TOL  || outvals2[c] > maxval + TOL) {
        inbounds_limited = false;
        break;
      }
    }
  }
  EXPECT_TRUE(outofbounds_unlimited);
  EXPECT_TRUE(inbounds_limited);

}




/// Second order interpolation of constant node-centered field with no
/// limiting in 3D

TEST(Interpolate_2nd_Order, Node_Ctr_Const_BND_NOLIMITER_3D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count nodes

  const int nnodes_source =
      sourceMeshWrapper.num_owned_nodes();
  const int nnodes_target =
      targetMeshWrapper.num_owned_nodes();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(nnodes_source, 1.25);
  source_state.add("nodevars", Wonton::Entity_kind::NODE, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_dualcell_coords(nnodes_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_dualcell_coords(nnodes_target);


  // Actually get the Wonton::Points for the dual cells

  for (int n = 0; n < nnodes_source; ++n)
	  sourceMeshWrapper.dual_cell_get_coordinates(n, &source_dualcell_coords[n]);
  for (int n = 0; n < nnodes_target; ++n)
	  targetMeshWrapper.dual_cell_get_coordinates(n, &target_dualcell_coords[n]);

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(nnodes_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(nnodes_target);

  // Loop over target nodes

  for (int c = 0; c < nnodes_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<3>(target_dualcell_coords[c],
                                        source_dualcell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

   // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::NODE,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("nodevars",
                                                  Portage::NOLIMITER,
                                                  Portage::DEFAULT_BND_LIMITER);

  interpolator.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each node on the
  // target. Second order interpolation should preserve a linear field
  // exactly but node-centered interpolation has some quirks - the field
  // does not get preserved exactly at the boundary because the source
  // values for boundary dual cells are not specified inside the dual
  // cells but at one of their vertices or edges. So, check only
  // interior nodes

  const double stdval = data[0];
  for (int n = 0; n < nnodes_target; ++n) {
    Wonton::Point<3> coord;
    targetMeshWrapper.node_get_coordinates(n, &coord);
    if (fabs(coord[0]) < 1e-16 || fabs(1-coord[0]) < 1e-16 ||
        fabs(coord[1]) < 1e-16 || fabs(1-coord[1]) < 1.e-16)
      continue;
    ASSERT_NEAR(stdval, outvals[n], TOL);
  }

}


TEST(Interpolate_2nd_Order, Node_Ctr_Lin_BND_NOLIMITER_3D) {


  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count nodes

  const int nnodes_source =
      sourceMeshWrapper.num_owned_nodes();
  const int nnodes_target =
      targetMeshWrapper.num_owned_nodes();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(nnodes_source);
  for (int n = 0; n < nnodes_source; ++n) {
    Wonton::Point<3> coord;
    sourceMeshWrapper.node_get_coordinates(n, &coord);
    data[n] = coord[0]+coord[1]+coord[2];
  }
  source_state.add("nodevars", Wonton::Entity_kind::NODE, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_dualcell_coords(nnodes_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_dualcell_coords(nnodes_target);


  // Actually get the Wonton::Points for the dual cells

  for (int n = 0; n < nnodes_source; ++n)
	  sourceMeshWrapper.dual_cell_get_coordinates(n, &source_dualcell_coords[n]);
  for (int n = 0; n < nnodes_target; ++n)
	  targetMeshWrapper.dual_cell_get_coordinates(n, &target_dualcell_coords[n]);

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals(nnodes_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(nnodes_target);

  // Loop over target nodes

  for (int c = 0; c < nnodes_target; ++c) {

    std::vector<int> xcells;
    std::vector<std::vector<double>> xwts;

		// Compute the moments
		// xcells is the source cell indices that intersect
		// xwts is the moments vector for each cell that intersects

    BOX_INTERSECT::intersection_moments<3>(target_dualcell_coords[c],
                                        source_dualcell_coords,
                                        &xcells, &xwts);

    // Pack the results into a vector of true Portage::Weights_t
    int const num_intersect_cells = xcells.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_cells);

    for (int i = 0; i < num_intersect_cells; ++i) {
      wtsvec[i].entityID = xcells[i];
      wtsvec[i].weights = xwts[i];
    }

    // Put the weights in final form

    sources_and_weights[c] = wtsvec;
  }

  // Now do it the Portage way

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation object

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
      interpolator(sourceMeshWrapper, targetMeshWrapper, sourceStateWrapper,
                   num_tols);

  interpolator.set_interpolation_variable("nodevars");

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::NODE,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  auto gradients = driver.compute_source_gradient("nodevars",
                                                  Portage::DEFAULT_LIMITER,
                                                  Portage::BND_NOLIMITER);

  interpolator.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals.begin(), interpolator);

  // Make sure we retrieved the correct value for each node on the
  // target. Second order interpolation should preserve a linear field
  // exactly but node-centered interpolation has some quirks - the field
  // does not get preserved exactly at the boundary because the source
  // values for boundary dual cells are not specified inside the dual
  // cells but at one of their vertices or edges. So, check only
  // interior nodes

  std::vector<double> stdvals(nnodes_target);
  for (int n = 0; n < nnodes_target; ++n) {
    Wonton::Point<3> coord;
    targetMeshWrapper.node_get_coordinates(n, &coord);
    if (fabs(coord[0]) < 1e-16 || fabs(1-coord[0]) < 1e-16 ||
        fabs(coord[1]) < 1e-16 || fabs(1-coord[1]) < 1.e-16 ||
        fabs(coord[2]) < 1e-16 || fabs(1-coord[2]) < 1.e-16)
      continue;
    stdvals[n] = coord[0]+coord[1]+coord[2];
    EXPECT_NEAR(stdvals[n], outvals[n], TOL);
  }

}


// Second order interpolation of discontinuous node-centered field with
// Barth-Jespersen limiting in 3D

TEST(Interpolate_2nd_Order, Node_Ctr_BJ_Limiter_3D) {

  // Create simple meshes

  std::shared_ptr<Wonton::Simple_Mesh> source_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 4, 4, 4);
  std::shared_ptr<Wonton::Simple_Mesh> target_mesh =
    std::make_shared<Wonton::Simple_Mesh>(0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 5, 5, 5);

  // Create mesh wrappers

  Wonton::Simple_Mesh_Wrapper sourceMeshWrapper(*source_mesh);
  Wonton::Simple_Mesh_Wrapper targetMeshWrapper(*target_mesh);

  // count nodes

  const int nnodes_source =
      sourceMeshWrapper.num_owned_nodes();
  const int nnodes_target =
      targetMeshWrapper.num_owned_nodes();

  // Create a state object

  Wonton::Simple_State source_state(source_mesh);
  Wonton::Simple_State target_state(target_mesh);

  // Define a state vector with constant value and add it to the source state

  std::vector<double> data(nnodes_source);
  double minval = 1e+10, maxval = -1e+10;
  for (int n = 0; n < nnodes_source; ++n) {
    Wonton::Point<3> coord;
    sourceMeshWrapper.node_get_coordinates(n, &coord);
    if (coord[0] >= 0.5)
      data[n] = 100*(coord[0]+coord[1]+coord[2]);
    else
      data[n] = coord[0]+coord[1]+coord[2];
    if (data[n] < minval) minval = data[n];
    if (data[n] > maxval) maxval = data[n];
  }
  source_state.add("nodevars", Wonton::Entity_kind::NODE, &(data[0]));

  // Create state wrapper

  Wonton::Simple_State_Wrapper sourceStateWrapper(source_state);
  Wonton::Simple_State_Wrapper targetStateWrapper(target_state);

  // Gather the cell coordinates as Portage Points for source and target meshes
  // for intersection. The outer vector is the cells, the inner vector is the
  // points of the vertices of that cell.

  std::vector<std::vector<Wonton::Point<3>>>
      source_dualcell_coords(nnodes_source);
  std::vector<std::vector<Wonton::Point<3>>>
      target_dualcell_coords(nnodes_target);


  // Actually get the Wonton::Points for the dual cells

  for (int n = 0; n < nnodes_source; ++n)
	  sourceMeshWrapper.dual_cell_get_coordinates(n, &source_dualcell_coords[n]);
  for (int n = 0; n < nnodes_target; ++n)
	  targetMeshWrapper.dual_cell_get_coordinates(n, &target_dualcell_coords[n]);

  // Interpolate from source to target mesh using the independent calculation
  // in simple_intersect_for_tests.h

  std::vector<double> outvals1(nnodes_target);
  std::vector<double> outvals2(nnodes_target);
  std::vector<std::vector<Portage::Weights_t>>
      sources_and_weights(nnodes_target);

  for (int n = 0; n < nnodes_target; ++n) {
    std::vector<int> xnodes;
    std::vector<std::vector<double>> xweights;

    BOX_INTERSECT::intersection_moments<3>(target_dualcell_coords[n],
                                        source_dualcell_coords,
                                        &xnodes, &xweights);

    int const num_intersect_nodes = xnodes.size();
    std::vector<Portage::Weights_t> wtsvec(num_intersect_nodes);

    for (int i = 0; i < num_intersect_nodes; ++i) {
      wtsvec[i].entityID = xnodes[i];
      wtsvec[i].weights = xweights[i];
    }
    sources_and_weights[n] = wtsvec;
  }

  // use default tolerances
  Portage::NumericTolerances_t num_tols = Portage::DEFAULT_NUMERIC_TOLERANCES<3>;

  // Create Interpolation objects - one with no limiter and one with limiter

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
                         interpolator1(sourceMeshWrapper, targetMeshWrapper,
                                       sourceStateWrapper, num_tols);

  Portage::Interpolate_2ndOrder<3, Wonton::Entity_kind::NODE,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_Mesh_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                Wonton::Simple_State_Wrapper,
                                double>
                         interpolator2(sourceMeshWrapper, targetMeshWrapper,
                                       sourceStateWrapper, num_tols);

  // compute gradient field to pass to the interpolator
  using Driver = Portage::CoreDriver<3, Wonton::Entity_kind::NODE,
                                     Wonton::Simple_Mesh_Wrapper,
                                     Wonton::Simple_State_Wrapper>;

  Driver driver(sourceMeshWrapper, sourceStateWrapper,
                targetMeshWrapper, targetStateWrapper);

  // Compute the target mesh vals
  auto gradients = driver.compute_source_gradient("nodevars",
                                                  Portage::NOLIMITER,
                                                  Portage::DEFAULT_BND_LIMITER);

  interpolator1.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals1.begin(), interpolator1);

  // Compute the target mesh vals
  gradients = driver.compute_source_gradient("nodevars",
                                             Portage::BARTH_JESPERSEN,
                                             Portage::DEFAULT_BND_LIMITER);

  interpolator2.set_interpolation_variable("nodevars", &gradients);

  Portage::transform(targetMeshWrapper.begin(Wonton::Entity_kind::NODE),
                     targetMeshWrapper.end(Wonton::Entity_kind::NODE),
                     sources_and_weights.begin(),
                     outvals2.begin(), interpolator2);

  // Check if we violated the bounds on at least one node in the
  // unlimited interpolate and if we respected the bounds on all nodes in
  // the limited case

  bool outofbounds_unlimited = false;
  for (int n = 0; n < nnodes_target; ++n) {
    if (outvals1[n] < minval  || outvals1[n] > maxval) {
      outofbounds_unlimited = true;
      break;
    }
  }

  // Check if we preserved bounds on interior nodes. Since we don't limit at
  // boundary nodes we have no guarantee their values will be within bounds
  bool inbounds_limited = true;
  for (int n = 0; n < nnodes_target; ++n) {
    if (!targetMeshWrapper.on_exterior_boundary(Wonton::Entity_kind::NODE, n)) {
      if (outvals2[n] < minval  || outvals2[n] > maxval) {
        inbounds_limited = false;
        break;
      }
    }
  }

  EXPECT_TRUE(outofbounds_unlimited);
  EXPECT_TRUE(inbounds_limited);

}
