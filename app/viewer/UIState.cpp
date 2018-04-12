////////////////////////////////////////////////////////////////////////////////
#include "UIState.h"

#include <cellogram/PolygonUtils.h>
#include <igl/colon.h>
#include <igl/unproject_onto_mesh.h>
#include <igl/jet.h>
#include <igl/png/readPNG.h>
////////////////////////////////////////////////////////////////////////////////

namespace cellogram {

// -----------------------------------------------------------------------------

UIState::UIState()
	: state(State::state())
{ }

UIState &UIState::ui_state() {
	static UIState instance;
	return instance;
}

void UIState::initialize() {

	//// KEY and MOUSE handling example
	//viewer.callback_key_down = [&](igl::opengl::glfw::Viewer& viewer, int key, int mod)->bool
	//{
	//	std::cout << (char)key << " "<< mod << std::endl;
	//	return false;
	//};

	//viewer.callback_key_up = [&](igl::opengl::glfw::Viewer& viewer, int key, int mod)->bool
	//{
	//	std::cout << (char)key << " " << mod << std::endl;
	//	return false;
	//};


	viewer.callback_mouse_down = [&](igl::opengl::glfw::Viewer& viewer, int, int)->bool
	{
		if (!select_region && !add_vertex && !delete_vertex)
			return false;

		int fid;
		Eigen::Vector3f bc;
		bool found = false;
		double x = viewer.current_mouse_x;
		double y = viewer.core.viewport(3) - viewer.current_mouse_y;
		if (igl::unproject_onto_mesh(Eigen::Vector2f(x, y), viewer.core.view * viewer.core.model,
			viewer.core.proj, viewer.core.viewport, state.points, state.triangles, fid, bc))
		{
			std::cout << fid << " " << bc.transpose() << std::endl;
			found = true;
		}

		if (!found)
			return false;


		if (select_region)
		{
			select_region = false;
			
			// set selected_region
			for (int i = 0; i < state.regions.size(); i++)
			{
				for (int  j = 0; j < state.regions[i].region_triangles.rows(); j++)
				{
					if (fid == state.regions[i].region_triangles(j))
					{
						selected_region = i;
						viewer_control();
						std::cout << "Region = " << i << std::endl;
						return true;
					}
				}
			}
		}
		else if(add_vertex)
		{ 
			add_vertex = false;
			double xNew = 0, yNew = 0, zNew = 0;
			for (int i = 0; i < 3; i++)
			{
				xNew += bc(i) * state.points(state.triangles(fid, i), 0);
				yNew += bc(i) * state.points(state.triangles(fid, i), 1);
				zNew += 0;
			}
			state.add_vertex(Eigen::Vector3d(xNew,yNew,zNew));
			viewer_control();

			std::cout << "\nDetect\n:" << state.detected.row(state.detected.rows()-1) << std::endl;
		}
		else if (delete_vertex)
		{
			delete_vertex = false;
			// find maximum barycenter coordinates and get vertex id
			int vid;
			int maxBC = 0;
			double maxBCVal = bc(maxBC);
			for (int i = 1; i < 3; i++)
			{
				if (maxBCVal < bc(i)) {
					maxBC = i;
					maxBCVal = bc(i);
				}
			}
			//std::cout << "\n max\n" <<  maxBC;
			vid = state.triangles(fid, maxBC);
			state.delete_vertex(vid);
			viewer_control();
		}
		return false;
	};

	viewer.plugins.push_back(this);

	/*state.load("C:\\Users\\Tobias\\Documents\\cellogram\\data\\small2.xyz");
	state.compute_hull();
	state.compute_triangulation();
	state.relax_with_lloyd();
	state.detect_bad_regions();
	state.fix_regions();
	state.resolve_regions();*/

	// Setup viewer parameters
	viewer.resize(1500, 1500);
	viewer.core.background_color.setOnes();
	// viewer.core.set_rotation_type(igl::opengl::ViewerCore::RotationType::ROTATION_TYPE_TRACKBALL);
	viewer.core.set_rotation_type(igl::opengl::ViewerCore::RotationType::ROTATION_TYPE_NO_ROTATION);
	viewer.core.orthographic = true;
	viewer.core.is_animating = true;
	viewer.core.is_animating = true;

	// Setup viewer data
	viewer.append_mesh();
	viewer.append_mesh();
	viewer.append_mesh();
	viewer.append_mesh();
	viewer.data_list[0].id = hull_id = 0;
	viewer.data_list[1].id = points_id = 1;
	viewer.data_list[2].id = image_id = 2;
	viewer.data_list[3].id = bad_region_id = 3;
	viewer.data_list[4].id = matching_id = 4;
}

void UIState::launch() {
	// Launch viewer
	viewer.launch();
}

////////////////////////////////////////////////////////////////////////////////

igl::opengl::ViewerData & UIState::mesh_by_id(int id) {
	size_t index = viewer.mesh_index(id);
	assert(viewer.data_list[index].id == id);
	return viewer.data_list[index];
}

bool UIState::load(std::string name) {
	if (!state.load(name)){ return false; }
	selected_region = -1;
	img.resize(0, 0);
	// reset flags

	mesh_color.resize(0,0);
	bool show_hull = false;
	bool image_loaded = false;
	bool show_points = false;
	bool show_mesh_fill = true;
	bool show_image = false;
	bool show_matching = false;
	bool show_bad_regions = false;

	// Show points and align camera
	points_data().clear();
	points_data().set_points(state.points, Eigen::RowVector3d(1, 0, 0));
	viewer.core.align_camera_center(state.points);
	double extent = (state.points.colwise().maxCoeff() - state.points.colwise().minCoeff()).maxCoeff();
	points_data().point_size = float(0.008 * extent);
	fix_color(points_data());

	// Compute and show convex hull + triangulation
	compute_hull();
	compute_triangulation();
	viewer_control();
	return true;
}

bool UIState::save(std::string name) {
	return state.save(name);
}

bool UIState::mouse_scroll(float delta_y) {
	viewer.scroll_position += delta_y;

	// Only zoom if there's actually a change
	if (delta_y != 0)
	{
		float mult = (1.0 + ((delta_y>0) ? 1. : -1.)*0.1);
		const float min_zoom = 0.1f;
		viewer.core.camera_zoom = (viewer.core.camera_zoom * mult > min_zoom ? viewer.core.camera_zoom * mult : min_zoom);
	}
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void UIState::compute_hull() {
	// State
	state.compute_hull();
	
	// UI
	show_hull = true;
}

// -----------------------------------------------------------------------------

void UIState::compute_triangulation() {
	// State
	state.compute_triangulation();

	// UI
	show_points = true;
}

//TODO refactor when more clear
void UIState::load_image(std::string fname) {
	Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> G, B, A; // Image
	igl::png::readPNG(fname, img, G, B, A);
	
	image_loaded = true;
	show_image = true;

}

void UIState::display_image()
{
	int xMax = img.cols();
	int yMax = img.rows();

	// Replace the mesh with a triangulated square
	Eigen::MatrixXd V(4, 3);
	V <<
		0, 0, 0,
		xMax, 0, 0,
		xMax, yMax, 0,
		0, xMax, 0;
	Eigen::MatrixXi F(2, 3);
	F <<
		0, 1, 2,
		2, 3, 0;
	Eigen::MatrixXd UV(4, 2);
	UV <<
		0, 1,
		1, 1,
		1, 0,
		0, 0;

	image_data().set_mesh(V, F);
	image_data().set_uv(UV);
	image_data().show_faces = true;
	image_data().show_lines = false;
	image_data().show_texture = true;
	// Use the image as a texture
	image_data().set_texture(img, img, img);
	image_data().set_colors(Eigen::RowVector3d(1, 1, 1));
}

void UIState::viewer_control()
{
	hull_data().clear();
	points_data().clear();
	image_data().clear();
	matching_data().clear();
	bad_region_data().clear();
	
	// Read all the UIState flags and update display
	// hull
	if (show_hull) {
		// Draw edges
		int n = (int)state.hull_polygon.rows();

		Eigen::MatrixXd P2; P2.resizeLike(state.hull_polygon);
		P2.topRows(n - 1) = state.hull_polygon.bottomRows(n - 1);
		P2.row(n - 1) = state.hull_polygon.row(0);
		hull_data().add_edges(state.hull_polygon, P2, Eigen::RowVector3d(0, 0, 0));

		hull_data().set_mesh(state.hull_vertices, state.hull_faces);
		hull_data().set_colors(Eigen::RowVector3d(52, 152, 219) / 255.0);
		hull_data().show_faces = false;
		hull_data().show_lines = false;
		hull_data().shininess = 0;
		hull_data().line_width = 3.0;
	}

	// points
	Eigen::MatrixXd V = t * state.points + (1 - t) * state.detected;
	points_data().set_mesh(V, state.triangles);
	if (image_loaded)
	{
		Eigen::MatrixXd UV(state.detected.rows(), 2);
		UV.col(0) = state.detected.col(0) / img.rows();
		UV.col(1) = 1 - state.detected.col(1).array() / img.cols();

		points_data().show_texture = true;
		points_data().set_uv(UV);
		points_data().set_texture(img, img, img);
		points_data().set_colors(Eigen::RowVector3d(1, 1, 1));
	}
	if (show_points)
	{
		if (state.regions.empty())
			points_data().set_points(V, vertex_color.cast<double>().head<3>());
		else
		{
			Eigen::MatrixXd C(V.rows(), 3);
			C.col(0).setConstant(vertex_color(0));
			C.col(1).setConstant(vertex_color(1));
			C.col(2).setConstant(vertex_color(2));

			for (auto &r : state.regions)
			{

				for (int i = 0; i < r.region_boundary.size(); ++i)
				{
					C.row(r.region_boundary(i)) = Eigen::RowVector3d(0, 0, 1);
				}

				for (int i = 0; i < r.region_interior.size(); ++i)
				{
					C.row(r.region_interior(i)) = Eigen::RowVector3d(0, 1, 0);
				}

			}


			points_data().set_points(V, C);
		}
	}

	// fill
	points_data().show_faces = show_mesh_fill;
	if (mesh_color.size() > 0) {
		if (selected_region >= 0) {
			for (int i = 0; i < state.regions[selected_region].region_interior.size(); ++i)
				mesh_color.row(state.regions[selected_region].region_interior[i]) = Eigen::RowVector3d(0, 1, 0);
		}
	}
	points_data().set_colors(mesh_color);

	// bad regions
	if (show_bad_regions)
	{
		Eigen::MatrixXd bad_P1, bad_P2;
		build_region_edges(V, bad_P1, bad_P2);
		bad_region_data().add_edges(bad_P1, bad_P2, Eigen::RowVector3d(0, 0, 0));
		bad_region_data().line_width = 3.0;
	}


	// image
	if (show_image && !image_loaded)
	{
		show_image = false;
	}
	else if (show_image && image_loaded) 
	{
		display_image();
	}

	// matching
	if (show_matching) {
		matching_data().clear();
		matching_data().add_edges(state.points, state.detected, Eigen::RowVector3d(0, 0, 0));
		matching_data().line_width = 3.0;
	}

	// Fix shininess for all layers
	fix_color(hull_data());
	fix_color(points_data());
	fix_color(image_data());
	fix_color(bad_region_data());
	fix_color(matching_data());
}

void UIState::draw_mesh()
{
	points_data().clear();
	points_data().set_points(state.points, Eigen::RowVector3d(1, 0, 0));
	points_data().set_mesh(state.points, state.triangles);

	fix_color(points_data());
}

void UIState::fix_color(igl::opengl::ViewerData &data) {
	data.F_material_specular.setZero();
	data.V_material_specular.setZero();
	data.dirty |= igl::opengl::MeshGL::DIRTY_DIFFUSE;

	data.V_material_ambient *= 2;
	data.F_material_ambient *= 2;
}


Eigen::VectorXd UIState::create_region_label()
{
	Eigen::VectorXd regionD(state.points.rows());
	regionD.setZero();
	for (int i = 0; i < state.regions.size(); ++i)
	{
		/*for (int j = 0; j < state.regions[i].region_boundary.size(); ++j)
		{
			regionD(state.regions[i].region_boundary(j)) = i + 10;
		}*/

		for (int j = 0; j < state.regions[i].region_interior.size(); ++j)
		{
			regionD(state.regions[i].region_interior(j)) = i+1;
		}
	}

	return regionD;
}

void UIState::build_region_edges(const Eigen::MatrixXd &pts, Eigen::MatrixXd &bad_P1, Eigen::MatrixXd &bad_P2)
{
	bad_P1.resize(pts.rows(), 3);
	bad_P2.resize(pts.rows(), 3);

	int index = 0;
	Eigen::MatrixXd local_bad_P1, local_bad_P2;
	for (auto &r : state.regions)
	{
		r.compute_edges(pts, local_bad_P1, local_bad_P2);
		bad_P1.block(index, 0, local_bad_P1.rows(), local_bad_P1.cols()) = local_bad_P1;
		bad_P2.block(index, 0, local_bad_P2.rows(), local_bad_P2.cols()) = local_bad_P2;

		index += local_bad_P2.rows();

	}
	bad_P1.conservativeResize(index, 3);
	bad_P2.conservativeResize(index, 3);
}

// -----------------------------------------------------------------------------

} // namespace cellogram
