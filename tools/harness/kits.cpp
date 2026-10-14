#include "opz/Kit.hpp"
#include <chrono>
Plugin* pluginInstance;
using namespace opz;
int main(int argc, char** argv) {
	auto t0 = std::chrono::steady_clock::now();
	for (int t = 0; t < 4; t++) builtinKit(t);
	auto t1 = std::chrono::steady_clock::now();
	printf("render %.0f ms\n", std::chrono::duration<double, std::milli>(t1 - t0).count());
	size_t total = 0;
	for (int t = 0; t < 4; t++) {
		const Kit& k = builtinKit(t);
		total += k.samples.size();
		printf("%s: %.2f s\n", k.name.c_str(), k.samples.size() / k.rate);
		for (int i = 0; i < Kit::SLICES; i++) {
			const Kit::Slice& s = k.slices[i];
			float peak = 0, rms = 0;
			for (int j = s.start; j < s.end; j++) { peak = std::max(peak, std::fabs(k.samples[j])); rms += k.samples[j] * k.samples[j]; }
			printf("  %2d %-14s %5.0f ms peak %.2f rms %.3f\n", i, s.name.c_str(), (s.end - s.start) * 1000.f / k.rate, peak, std::sqrt(rms / std::max(1, s.end - s.start)));
		}
		system::writeFile(string::f("%s/kit%d.wav", argv[1], t), k.toWav());
		printf("bounds%d ", t);
		for (int i = 0; i < Kit::SLICES; i++) printf("%d,", k.slices[i].start);
		printf("%d\n", k.slices[23].end);
	}
	printf("total %.1f MB\n", total * 4 / 1e6);
	// round trip through the saved form and the loaders
	const Kit& k = builtinKit(1);
	json_t* j = k.toJson();
	auto back = Kit::fromSaved(k.toWav(), j);
	printf("roundtrip %d %d %s %d\n", back && back->samples == k.samples, back && back->slices[5].end == k.slices[5].end, back ? back->slices[5].name.c_str() : "-", back ? (int) back->rate : 0);
	std::string err;
	auto file = Kit::load(string::f("%s/kit1.wav", argv[1]), &err);
	printf("load %s even slices: %d..%d\n", file ? "ok" : err.c_str(), file ? file->slices[1].start : -1, file ? file->slices[1].end : -1);
}
