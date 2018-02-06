/* mit license here ... */

#include "biff.h"
// for mkdir
#include <sys/stat.h>
#include <sys/types.h>

namespace biff {
  Writer::Writer(const std::string &outputPath)
  {
    mkdir(outputPath.c_str(),S_IRWXU);
    triMeshFile.open(outputPath+"/tri_meshes.bin");
    instanceFile.open(outputPath+"/instances.bin");
    textureFile.open(outputPath+"/textures.bin");
    flatCurveFile.open(outputPath+"/flat_curves.bin");
    roundCurveFile.open(outputPath+"/round_curves.bin");
    materialFile.open(outputPath+"/materials.xml");
    sceneFile.open(outputPath+"/scene.xml");
  }

}
