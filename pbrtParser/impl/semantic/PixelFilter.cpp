#include "SemanticParser.h"

namespace pbrt {

void createPixelFilter(Scene::SP ours, pbrt::syntactic::Scene::SP pbrt) 
{
    if (!pbrt->pixelFilter) {
        std::cout << "pbrt-parser: pixel filter is missing\n";
        return;
    }

    ours->pixelFilter = std::make_shared<PixelFilter>();

    if (pbrt->pixelFilter->type == "box") {
        ours->pixelFilter->type = PixelFilter::Type::box;
        ours->pixelFilter->radius = pbrt->pixelFilter->getParam1f("xwidth", 0.5f);
    }
    else if (pbrt->pixelFilter->type == "gaussian") {
        ours->pixelFilter->type = PixelFilter::Type::gaussian;
        ours->pixelFilter->radius = pbrt->pixelFilter->getParam1f("xwidth", 2.f);
        ours->pixelFilter->alpha = pbrt->pixelFilter->getParam1f("alpha", 2.f);
    }
    else if (pbrt->pixelFilter->type == "triangle") {
        ours->pixelFilter->type = PixelFilter::Type::triangle;
        ours->pixelFilter->radius = pbrt->pixelFilter->getParam1f("xwidth", 2.f);
    }
    else {
        std::cout << "pbrt-parser: unsupported pixel filter type: " + pbrt->pixelFilter->type + "\n";
        assert(false);
    }
}

} // namespace pbrt
