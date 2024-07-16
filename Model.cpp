#include "Model.h"


Model* geodesic_model = NULL;


#define GEODESIC_TUBE_RADIUS			(0.005)
#define GEODESIC_LONGITUDINAL_SEGMENTS	(32)
#define GEODESIC_TRANSVERSE_SEGMENTS	(8)

void init_models()
{
	int verts_per_strip = 2 * (GEODESIC_TRANSVERSE_SEGMENTS + 1);

	geodesic_model = new Model(
		GL_QUAD_STRIP,
		GEODESIC_LONGITUDINAL_SEGMENTS * GEODESIC_TRANSVERSE_SEGMENTS,
		NULL,
		verts_per_strip,
		GEODESIC_LONGITUDINAL_SEGMENTS
	);

	double normalization_factor = 1.0 / sqrt(1.0 + GEODESIC_TUBE_RADIUS * GEODESIC_TUBE_RADIUS);
	for(int i = 0; i < GEODESIC_LONGITUDINAL_SEGMENTS; i++)
	{
		double theta = (double)i * TAU / GEODESIC_LONGITUDINAL_SEGMENTS;
		for(int j = 0; j < GEODESIC_TRANSVERSE_SEGMENTS; j++)
		{
			double phi = (double)j * TAU / GEODESIC_TRANSVERSE_SEGMENTS;
			geodesic_model->vertices[i * GEODESIC_TRANSVERSE_SEGMENTS + j] = normalization_factor * Vec4(
				GEODESIC_TUBE_RADIUS * cos(phi),
				GEODESIC_TUBE_RADIUS * sin(phi),
				sin(theta),
				cos(theta)
			);
			
			printf("%d: %d\n", i * verts_per_strip + 2 * j, i * GEODESIC_TRANSVERSE_SEGMENTS + j);
			printf("%d: %d\n", i * verts_per_strip + 2 * j + 1, ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS + j);
			geodesic_model->primitives[i * verts_per_strip + 2 * j] = i * GEODESIC_TRANSVERSE_SEGMENTS + j;
			geodesic_model->primitives[i * verts_per_strip + 2 * j + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS + j;
		}
		
		printf("%d: %d\n", i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS, i * GEODESIC_TRANSVERSE_SEGMENTS);
		printf("%d: %d\n", i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS + 1, ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS);
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS] = i * GEODESIC_TRANSVERSE_SEGMENTS;
		geodesic_model->primitives[i * verts_per_strip + 2 * GEODESIC_TRANSVERSE_SEGMENTS + 1] = ((i + 1) % GEODESIC_LONGITUDINAL_SEGMENTS) * GEODESIC_TRANSVERSE_SEGMENTS;
	}

	printf("%d %d\n", geodesic_model->num_vertices, geodesic_model->num_primitives * geodesic_model->vertices_per_primitive);

	for(int i = 0; i < geodesic_model->num_vertices; i++)
		print_vector(geodesic_model->vertices[i]);
	for(int i = 0; i < geodesic_model->num_primitives; i++)
	{
		for(int j = 0; j < geodesic_model->vertices_per_primitive; j++)
			printf("%d  ", geodesic_model->primitives[i * geodesic_model->vertices_per_primitive + j]);
		printf("\n");
	}
	printf("Done.\n");
	//exit(0);
}
