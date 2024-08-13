#pragma once

#include "Shaders.h"


//Screenspace shaders:
extern ShaderCore *frag_point_light, *frag_bloom_separate, *frag_bloom, *frag_final_color;
//Debugging shaders:
extern ShaderCore *frag_dump_texture, *frag_dump_cubemap, *frag_dump_texture1d;

void init_torus_world_shaders();
