#pragma once

////////////////////////////////////////////////////////////////////////////////
#include "common.h"
#include "cellogram/Mesh.h"
#include <Eigen/Dense>
////////////////////////////////////////////////////////////////////////////////

namespace cellogram {
	class Region
	{
	public:
		static const int TOO_FEW_VERTICES = -4;
		static const int TOO_MANY_VERTICES = -3;
		static const int NOT_PROPERLY_CLOSED = -2;
		static const int NO_SOLUTION = -1;
		static const int OK = 0;
		int status = 0;

		bool verbose = true;
		Eigen::VectorXi region_boundary;
		Eigen::VectorXi region_interior;
		Eigen::VectorXi region_triangles;

		void compute_edges(const Eigen::MatrixXd &V, Eigen::MatrixXd &bad_P1, Eigen::MatrixXd &bad_P2);

		void grow(const Eigen::MatrixXd &V, const Eigen::MatrixXi &F);

		void find_points(const Eigen::VectorXi &region_ids, const int id);
		void find_triangles(const Eigen::MatrixXi &F, const Eigen::VectorXi &region_ids, const int id);
		void find_triangles(const Eigen::MatrixXi & F);
		void bounding(const Eigen::MatrixXi &F, const Eigen::MatrixXd &V);

		int resolve(const Eigen::MatrixXd &V_detected, const Eigen::MatrixXd &V_current, const Eigen::MatrixXi &F, const std::vector<std::vector<int>> &adj, const int perm_possibilities, Eigen::MatrixXd  &new_points, Eigen::MatrixXi &new_triangles);

		void fix_missing_points(const Eigen::MatrixXi & F);
		
		void delete_vertex(cellogram::Mesh &mesh, const int index);
		void triangluate_region(); 

		void local_to_global(Eigen::VectorXi &local2global);

		//todo
		//void reduce_index

	private:
		Eigen::MatrixXi get_triangulation(const Eigen::MatrixXi &F);
	};
} // namespace cellogram
