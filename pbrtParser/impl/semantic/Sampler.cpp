#include "SemanticParser.h"

namespace pbrt {

void createSampler(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt)
{
    if (!pbrt->sampler) {
        std::cout << "pbrt-parser: sampler is missing\n";
        return;
    }

    ours->sampler = std::make_shared<Sampler>();

    if (pbrt->sampler->type == "halton") {
        ours->sampler->type = Sampler::Type::stratified; // use stratified sampler until halton is supported
        int n = pbrt->sampler->getParam1i("pixelsamples", 1);
        int k = (int)std::ceil(std::sqrt(n));
        ours->sampler->xSamples = k;
        ours->sampler->ySamples = k;
        ours->sampler->pixelSamples = k*k;
    }
    else if (pbrt->sampler->type == "sobol") {
        ours->sampler->type = Sampler::Type::stratified; // use stratified sampler until halton is supported
        int n = pbrt->sampler->getParam1i("pixelsamples", 1);
        int k = (int)std::ceil(std::sqrt(n));
        ours->sampler->xSamples = k;
        ours->sampler->ySamples = k;
        ours->sampler->pixelSamples = k*k;
    }
    else if (pbrt->sampler->type == "stratified") {
        ours->sampler->type = Sampler::Type::stratified;
        ours->sampler->xSamples = pbrt->sampler->getParam1i("xsamples", 1);
        ours->sampler->ySamples = pbrt->sampler->getParam1i("ysamples", 1);
        ours->sampler->pixelSamples = ours->sampler->xSamples * ours->sampler->ySamples;
    }
    else if (pbrt->sampler->type == "random") {
        ours->sampler->type = Sampler::Type::stratified; // use stratified sampler until random is supported
        int n = pbrt->sampler->getParam1i("pixelsamples", 1);
        int k = (int)std::ceil(std::sqrt(n));
        ours->sampler->xSamples = k;
        ours->sampler->ySamples = k;
        ours->sampler->pixelSamples = k*k;
    }
    else {
        std::cout << "pbrt-parser: unsupported sampler type: " + pbrt->sampler->type + "\n";
    }
}

} // namespace pbrt
