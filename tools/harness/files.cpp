#include "opz/Kit.hpp"
Plugin* pluginInstance;
using namespace opz;
int main(int argc, char** argv) {
	std::string d = argv[1], e;
	auto k = Kit::load(d + "/op1kit.aif", &e);
	if (!k) { printf("op1: %s\n", e.c_str()); return 1; }
	printf("op1 kit '%s' rate %.0f frames %zu\n", k->name.c_str(), k->rate, k->samples.size());
	for (int i = 0; i < 4; i++) {
		auto& s = k->slices[i];
		printf("  %d: %d..%d mode %d reverse %d speed %.3f gain %.2f first %.2f empty %d\n", i, s.start, s.end, s.mode, s.reverse, s.speed, s.gain, s.empty() ? 0.f : k->samples[s.start], s.empty());
	}
	auto f = Kit::loadFolder(d + "/folder", &e);
	if (!f) { printf("folder: %s\n", e.c_str()); return 1; }
	printf("folder '%s' used %d\n", f->name.c_str(), f->used);
	for (int i = 0; i < f->used; i++) {
		auto& s = f->slices[i];
		float peak = 0; for (int j = s.start; j < s.end; j++) peak = std::max(peak, std::fabs(f->samples[j]));
		printf("  %s: %d samples (%.0f ms) peak %.2f\n", s.name.c_str(), s.end - s.start, (s.end - s.start) * 1000.f / f->rate, peak);
	}
	auto bad = Kit::load(d + "/folder/notes.txt", &e);
	printf("text file: %s\n", bad ? "loaded?!" : e.c_str());
}
