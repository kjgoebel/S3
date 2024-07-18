#pragma once

double frand();
double fsrand();
double current_time();

char* read_file(const char* filename);

void error(const char* fmt, ...);
