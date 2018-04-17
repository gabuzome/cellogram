////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "common.h"
#include <vector>
#include <Eigen/Dense>
////////////////////////////////////////////////////////////////////////////////

namespace cellogram {

class Mesh {

public:
	Mesh() { }

	Eigen::MatrixXd detected; // detected (unmoved) point positions
	Eigen::MatrixXd params; // parameters of detected
	Eigen::MatrixXd points; // relaxed point positions
	Eigen::VectorXi boundary; // list of vertices on the boundary

	Eigen::MatrixXi triangles; // triangular mesh
	std::vector<std::vector<int>> adj; // adjaceny list of triangluar mesh

	bool load(const std::string &path);
	bool load_params(const std::string &path);

	void relax_with_lloyd(const int lloyd_iterations, const Eigen::MatrixXd &hull_vertices, const Eigen::MatrixXi &hull_faces);
	void vertex_degree(Eigen::VectorXi &degree);

	//// Vertex
	void delete_vertex(const int index, bool recompute_triangulation = true);
	void add_vertex(Eigen::Vector3d &new_point);

	void local_update(Eigen::VectorXi &local2global, Eigen::MatrixXd &new_points, Eigen::MatrixXi &new_triangles, Eigen::VectorXi & old_triangles);
	void local_update(Eigen::VectorXi & local2global, const int global_to_remove, Eigen::MatrixXi & new_triangles);

	void reset();

private:
	void compute_triangulation();

};

} // namespace cellogram
