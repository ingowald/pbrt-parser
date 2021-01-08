#include "SemanticParser.h"

namespace pbrt {

void createIntegrator(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt) 
{
    if (!pbrt->integrator) {
        std::cout << "pbrt-parser: integrator is missing\n";
        return;
    }

    ours->integrator = std::make_shared<Integrator>();

    if (pbrt->integrator->type == "directlighting") {
        ours->integrator->type = Integrator::Type::direct_lighting;
        ours->integrator->maxDepth = pbrt->integrator->getParam1i("maxdepth", 5);
    }
    else if (pbrt->integrator->type == "path") {
        ours->integrator->type = Integrator::Type::path_tracer;
        ours->integrator->maxDepth = pbrt->integrator->getParam1i("maxdepth", 5);
    }
    else {
        std::cout << "pbrt-parser: unsupported integrator type: " + pbrt->sampler->type + "\n";
    }
}

} // namespace pbrt