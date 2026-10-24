#include "Plugs.hpp"
#include <mutex>

namespace opz {

namespace {

std::vector<Plug>& registry() {
	static std::vector<Plug> r;
	return r;
}

/** The plugs' dials, each pair on a module of its own, made the first time any are wanted. */
struct Dials {
	std::vector<std::unique_ptr<Module>> modules;
	std::vector<const Plug*> byKind[KINDS];
	const Plug* breakouts[KINDS] = {};

	Dials() {
		std::vector<Plug>& r = registry();
		std::sort(r.begin(), r.end(), [](const Plug& a, const Plug& b) { return a.slug < b.slug; });
		for (const Plug& p : r) {
			auto m = std::make_unique<Module>();
			m->config(2, 0, 0, 0);
			p.configSound(m.get(), 0, 1);
			modules.push_back(std::move(m));
			if (p.breakout)
				breakouts[p.kind] = &p;
			else
				byKind[p.kind].push_back(&p);
		}
	}

	Module* of(const Plug* p) {
		return modules[p - registry().data()].get();
	}
};

Dials& dials() {
	static Dials d;
	return d;
}

} // namespace


PlugRegistration::PlugRegistration(const Plug& p) {
	registry().push_back(p);
}

ParamQuantity* Plug::dial(int i) const {
	return dials().of(this)->paramQuantities[i];
}

SoundRange Plug::range(int i) const {
	ParamQuantity* q = dial(i);
	return {q->minValue, q->maxValue, q->snapEnabled};
}

float Plug::defaultShare(int i) const {
	ParamQuantity* q = dial(i);
	return q->maxValue > q->minValue ? (q->defaultValue - q->minValue) / (q->maxValue - q->minValue) : 0.f;
}

const std::vector<const Plug*>& plugs(int kind) {
	return dials().byKind[kind];
}

const Plug* breakoutPlug(int kind) {
	return dials().breakouts[kind];
}

int plugNumber(const Plug* p) {
	dials();
	return int(p - registry().data());
}

const Plug* numberedPlug(int n) {
	dials();
	return &registry()[n];
}

int plugCount() {
	dials();
	return (int) registry().size();
}

int plugIndex(int kind, const std::string& slug) {
	const std::vector<const Plug*>& list = plugs(kind);
	for (size_t i = 0; i < list.size(); i++)
		if (list[i]->slug == slug)
			return (int) i;
	return -1;
}

} // namespace opz
