// ======================================================================== //
// Copyright 2015-2019 Ingo Wald                                            //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <sstream>

#include <gtest/gtest.h>

#include "Parser.h"
#include "SemanticParser.h"
#include "pbrtParser/Scene.h"

using namespace pbrt;

typedef syntactic::BasicParser<syntactic::IStream<std::stringstream>> IParser;
typedef syntactic::IStream<std::stringstream> Stream;

// Helper function, parses ascii and pbf to Scene::SPs
static void parse(const Stream::SP& pbrt, Scene::SP& ascii, Scene::SP& binary)
{
  // Parse ascii
  IParser parser;
  parser.parse<std::stringstream>(pbrt);

  syntactic::Scene::SP sc = parser.getScene();
  if (!sc)
    return;

  ascii = SemanticParser(sc).result;
  if (!ascii)
    return;

  createFilm(ascii, sc);
  for (auto cam : sc->cameras)
     ascii->cameras.push_back(createCamera(cam));

  // Convert to pbf
  std::stringstream pbf;
  ascii->saveTo(pbf);

  binary = Scene::loadFrom(pbf);
}


// =======================================================
// InfiniteLightSource
// =======================================================

TEST(PbrtParser, InfiniteLightSource)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            LightSource "infinite"
            "string mapname" "textures/light.exr"
            "rgb L" [.4 .45 .5]
            "rgb scale" [.1 .2 .3]
            "integer nsamples" [ 4711 ]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->lightSources.size(), 1);

  InfiniteLightSource::SP asciiILS = std::dynamic_pointer_cast<InfiniteLightSource>(ascii->world->lightSources[0]);
  ASSERT_NE(asciiILS, nullptr);

  EXPECT_EQ(asciiILS->mapName, "textures/light.exr");

  EXPECT_FLOAT_EQ(asciiILS->L.x, .4f);
  EXPECT_FLOAT_EQ(asciiILS->L.y, .45f);
  EXPECT_FLOAT_EQ(asciiILS->L.z, .5f);

  EXPECT_FLOAT_EQ(asciiILS->scale.x, .1f);
  EXPECT_FLOAT_EQ(asciiILS->scale.y, .2f);
  EXPECT_FLOAT_EQ(asciiILS->scale.z, .3f);

  EXPECT_EQ(asciiILS->nSamples, 4711);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->lightSources.size(), ascii->world->lightSources.size());

  InfiniteLightSource::SP binaryILS = std::dynamic_pointer_cast<InfiniteLightSource>(binary->world->lightSources[0]);
  ASSERT_NE(binaryILS, nullptr);

  EXPECT_EQ(asciiILS->mapName, binaryILS->mapName);

  EXPECT_FLOAT_EQ(asciiILS->L.x, binaryILS->L.x);
  EXPECT_FLOAT_EQ(asciiILS->L.y, binaryILS->L.y);
  EXPECT_FLOAT_EQ(asciiILS->L.z, binaryILS->L.z);

  EXPECT_FLOAT_EQ(asciiILS->scale.x, binaryILS->scale.x);
  EXPECT_FLOAT_EQ(asciiILS->scale.y, binaryILS->scale.y);
  EXPECT_FLOAT_EQ(asciiILS->scale.z, binaryILS->scale.z);

  EXPECT_EQ(asciiILS->nSamples, binaryILS->nSamples);
}


// =======================================================
// DistantLightSource
// =======================================================

TEST(PbrtParser, DistantLightSource)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            LightSource "distant"
            "point from" [.2 .2 .2]
            "point to" [.2 .2 .4]
            "rgb L" [.4 .45 .5]
            "rgb scale" [.1 .2 .3]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->lightSources.size(), 1);

  DistantLightSource::SP asciiDLS = std::dynamic_pointer_cast<DistantLightSource>(ascii->world->lightSources[0]);
  ASSERT_NE(asciiDLS, nullptr);

  EXPECT_FLOAT_EQ(asciiDLS->from.x, .2f);
  EXPECT_FLOAT_EQ(asciiDLS->from.y, .2f);
  EXPECT_FLOAT_EQ(asciiDLS->from.z, .2f);

  EXPECT_FLOAT_EQ(asciiDLS->to.x, .2f);
  EXPECT_FLOAT_EQ(asciiDLS->to.y, .2f);
  EXPECT_FLOAT_EQ(asciiDLS->to.z, .4f);

  EXPECT_FLOAT_EQ(asciiDLS->L.x, .4f);
  EXPECT_FLOAT_EQ(asciiDLS->L.y, .45f);
  EXPECT_FLOAT_EQ(asciiDLS->L.z, .5f);

  EXPECT_FLOAT_EQ(asciiDLS->scale.x, .1f);
  EXPECT_FLOAT_EQ(asciiDLS->scale.y, .2f);
  EXPECT_FLOAT_EQ(asciiDLS->scale.z, .3f);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->lightSources.size(), ascii->world->lightSources.size());

  DistantLightSource::SP binaryDLS = std::dynamic_pointer_cast<DistantLightSource>(binary->world->lightSources[0]);
  ASSERT_NE(binaryDLS, nullptr);

  EXPECT_FLOAT_EQ(asciiDLS->from.x, binaryDLS->from.x);
  EXPECT_FLOAT_EQ(asciiDLS->from.y, binaryDLS->from.y);
  EXPECT_FLOAT_EQ(asciiDLS->from.z, binaryDLS->from.z);

  EXPECT_FLOAT_EQ(asciiDLS->to.x, binaryDLS->to.x);
  EXPECT_FLOAT_EQ(asciiDLS->to.y, binaryDLS->to.y);
  EXPECT_FLOAT_EQ(asciiDLS->to.z, binaryDLS->to.z);

  EXPECT_FLOAT_EQ(asciiDLS->L.x, binaryDLS->L.x);
  EXPECT_FLOAT_EQ(asciiDLS->L.y, binaryDLS->L.y);
  EXPECT_FLOAT_EQ(asciiDLS->L.z, binaryDLS->L.z);

  EXPECT_FLOAT_EQ(asciiDLS->scale.x, binaryDLS->scale.x);
  EXPECT_FLOAT_EQ(asciiDLS->scale.y, binaryDLS->scale.y);
  EXPECT_FLOAT_EQ(asciiDLS->scale.z, binaryDLS->scale.z);
}


// =======================================================
// DiffuseAreaLightRGB
// =======================================================

TEST(PbrtParser, DiffuseAreaLightRGB)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            AreaLightSource "diffuse"
            "rgb L" [.5 .5 .5]
            "rgb scale" [.1 .2 .3]
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->lightSources.size(), 0); // not a LightSource..
  ASSERT_EQ(ascii->world->shapes.size(), 1);       // but a Shape!
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  DiffuseAreaLightRGB::SP asciiAL = std::dynamic_pointer_cast<DiffuseAreaLightRGB>(ascii->world->shapes[0]->areaLight);
  ASSERT_NE(asciiAL, nullptr);

  EXPECT_FLOAT_EQ(asciiAL->L.x, .5f);
  EXPECT_FLOAT_EQ(asciiAL->L.y, .5f);
  EXPECT_FLOAT_EQ(asciiAL->L.z, .5f);

//EXPECT_FLOAT_EQ(asciiAL->scale.x, .1f); // TODO!
//EXPECT_FLOAT_EQ(asciiAL->scale.y, .2f);
//EXPECT_FLOAT_EQ(asciiAL->scale.z, .3f);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->lightSources.size(), ascii->world->lightSources.size());
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  DiffuseAreaLightRGB::SP binaryAL = std::dynamic_pointer_cast<DiffuseAreaLightRGB>(binary->world->shapes[0]->areaLight);
  ASSERT_NE(binaryAL, nullptr);

  EXPECT_FLOAT_EQ(asciiAL->L.x, binaryAL->L.x);
  EXPECT_FLOAT_EQ(asciiAL->L.y, binaryAL->L.x);
  EXPECT_FLOAT_EQ(asciiAL->L.z, binaryAL->L.x);

//EXPECT_FLOAT_EQ(asciiAL->scale.x, binaryAL->scale.x); // TODO!
//EXPECT_FLOAT_EQ(asciiAL->scale.y, binaryAL->scale.y);
//EXPECT_FLOAT_EQ(asciiAL->scale.z, binaryAL->scale.z);
}


// =======================================================
// DiffuseAreaLightBB
// =======================================================

TEST(PbrtParser, DiffuseAreaLightBB)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            AreaLightSource "diffuse"
            "blackbody L" [2700 15]
            "rgb scale" [.1 .2 .3]
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->lightSources.size(), 0); // not a LightSource..
  ASSERT_EQ(ascii->world->shapes.size(), 1);       // but a Shape!
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  DiffuseAreaLightBB::SP asciiAL = std::dynamic_pointer_cast<DiffuseAreaLightBB>(ascii->world->shapes[0]->areaLight);
  ASSERT_NE(asciiAL, nullptr);

  EXPECT_FLOAT_EQ(asciiAL->temperature, 2700.f);
  EXPECT_FLOAT_EQ(asciiAL->scale, 15.f);

  // TODO: blackbody area light has a scale parameter that is not L.scale!
//EXPECT_FLOAT_EQ(asciiAL->scale.x, .1f);
//EXPECT_FLOAT_EQ(asciiAL->scale.y, .2f);
//EXPECT_FLOAT_EQ(asciiAL->scale.z, .3f);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->lightSources.size(), ascii->world->lightSources.size());
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  DiffuseAreaLightBB::SP binaryAL = std::dynamic_pointer_cast<DiffuseAreaLightBB>(binary->world->shapes[0]->areaLight);
  ASSERT_NE(binaryAL, nullptr);

  EXPECT_FLOAT_EQ(asciiAL->temperature, binaryAL->temperature);
  EXPECT_FLOAT_EQ(asciiAL->scale, binaryAL->scale);

//EXPECT_FLOAT_EQ(asciiAL->scale.x, binaryAL->scale.x); // TODO!
//EXPECT_FLOAT_EQ(asciiAL->scale.y, binaryAL->scale.y);
//EXPECT_FLOAT_EQ(asciiAL->scale.z, binaryAL->scale.z);
}


// =======================================================
// Spectrum
// =======================================================

TEST(PbrtParser, Spectrum)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            Material "metal" "spectrum eta" [ 300 .3  400 .6   410 .65  415 .8  500 .2  600 .1 ]
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), 1);
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  MetalMaterial::SP asciiMM = std::dynamic_pointer_cast<MetalMaterial>(ascii->world->shapes[0]->material);
  ASSERT_NE(asciiMM, nullptr);

  const Spectrum& asciiEta = asciiMM->spectrum_eta;
  ASSERT_EQ(asciiEta.spd.size(), 6);
  EXPECT_FLOAT_EQ(asciiEta.spd[0].first, 300.f); EXPECT_FLOAT_EQ(asciiEta.spd[0].second, .3f);
  EXPECT_FLOAT_EQ(asciiEta.spd[1].first, 400.f); EXPECT_FLOAT_EQ(asciiEta.spd[1].second, .6f);
  EXPECT_FLOAT_EQ(asciiEta.spd[2].first, 410.f); EXPECT_FLOAT_EQ(asciiEta.spd[2].second, .65f);
  EXPECT_FLOAT_EQ(asciiEta.spd[3].first, 415.f); EXPECT_FLOAT_EQ(asciiEta.spd[3].second, .8f);
  EXPECT_FLOAT_EQ(asciiEta.spd[4].first, 500.f); EXPECT_FLOAT_EQ(asciiEta.spd[4].second, .2f);
  EXPECT_FLOAT_EQ(asciiEta.spd[5].first, 600.f); EXPECT_FLOAT_EQ(asciiEta.spd[5].second, .1f);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  MetalMaterial::SP binaryMM = std::dynamic_pointer_cast<MetalMaterial>(binary->world->shapes[0]->material);
  ASSERT_NE(binaryMM, nullptr);

  const Spectrum& binaryEta = binaryMM->spectrum_eta;
  ASSERT_EQ(binaryEta.spd.size(), asciiEta.spd.size());

  for (std::size_t i = 0; i < binaryEta.spd.size(); ++i)
  {
    EXPECT_FLOAT_EQ(asciiEta.spd[i].first, binaryEta.spd[i].first);
    EXPECT_FLOAT_EQ(asciiEta.spd[i].second, binaryEta.spd[i].second);
  }
}


// =======================================================
// ImageTexture
// =======================================================

TEST(PbrtParser, ImageTexture)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            Texture "tex-name" "float" "imagemap" 
                "string filename" [ "textures/imagemap.png" ]

            Material "metal" "texture roughness" "tex-name"
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), 1);
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  MetalMaterial::SP asciiMM = std::dynamic_pointer_cast<MetalMaterial>(ascii->world->shapes[0]->material);
  ASSERT_NE(asciiMM, nullptr);

  ImageTexture::SP asciiIT = std::dynamic_pointer_cast<ImageTexture>(asciiMM->map_roughness);
  EXPECT_EQ(asciiIT->fileName, "textures/imagemap.png");

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  MetalMaterial::SP binaryMM = std::dynamic_pointer_cast<MetalMaterial>(binary->world->shapes[0]->material);
  ASSERT_NE(binaryMM, nullptr);

  ImageTexture::SP binaryIT = std::dynamic_pointer_cast<ImageTexture>(binaryMM->map_roughness);
  EXPECT_EQ(asciiIT->fileName, binaryIT->fileName);
}


// =======================================================
// ConstantTexture
// =======================================================

TEST(PbrtParser, ConstantTexture)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            Texture "tex-name" "color" "constant" 
                "rgb value" [ 0.12 0.08 0.04 ]

            Material "metal" "texture roughness" "tex-name"
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), 1);
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  MetalMaterial::SP asciiMM = std::dynamic_pointer_cast<MetalMaterial>(ascii->world->shapes[0]->material);
  ASSERT_NE(asciiMM, nullptr);

  ConstantTexture::SP asciiCT = std::dynamic_pointer_cast<ConstantTexture>(asciiMM->map_roughness);
  EXPECT_FLOAT_EQ(asciiCT->value.x, .12f);
  EXPECT_FLOAT_EQ(asciiCT->value.y, .08f);
  EXPECT_FLOAT_EQ(asciiCT->value.z, .04f);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  MetalMaterial::SP binaryMM = std::dynamic_pointer_cast<MetalMaterial>(binary->world->shapes[0]->material);
  ASSERT_NE(binaryMM, nullptr);

  ConstantTexture::SP binaryCT = std::dynamic_pointer_cast<ConstantTexture>(binaryMM->map_roughness);
  EXPECT_FLOAT_EQ(asciiCT->value.x, binaryCT->value.x);
  EXPECT_FLOAT_EQ(asciiCT->value.y, binaryCT->value.y);
  EXPECT_FLOAT_EQ(asciiCT->value.z, binaryCT->value.z);
}


// =======================================================
// DisneyMaterial
// =======================================================

TEST(PbrtParser, DisneyMaterial)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            MakeNamedMaterial "mat-name"
                "string type" ["disney"]
                "rgb color" [0.13 0.37 0.01]
                "float spectrans" [0.04]
                "float clearcoatgloss" [1.00]
                "float speculartint" [0.05]
                "float eta" [1.20]
                "float sheentint" [0.80]
                "float metallic" [0.5]
                "float anisotropic" [0.1]
                "float clearcoat" [0.3]
                "float roughness" [0.70]
                "float sheen" [0.10]
                "bool thin" ["true"]
                "float difftrans" [1.30]
                "float flatness" [0.21]

            NamedMaterial "mat-name"
            Shape "sphere" "float radius" [.25]
        WorldEnd
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), 1);
  ASSERT_NE(ascii->world->shapes[0], nullptr);

  DisneyMaterial::SP asciiDM = std::dynamic_pointer_cast<DisneyMaterial>(ascii->world->shapes[0]->material);
  ASSERT_NE(asciiDM, nullptr);

  EXPECT_FLOAT_EQ(asciiDM->anisotropic, .1f);
  EXPECT_FLOAT_EQ(asciiDM->clearCoat, .3f);
  EXPECT_FLOAT_EQ(asciiDM->clearCoatGloss, 1.f);
  EXPECT_FLOAT_EQ(asciiDM->color.x, .13f);
  EXPECT_FLOAT_EQ(asciiDM->color.y, .37f);
  EXPECT_FLOAT_EQ(asciiDM->color.z, .01f);
  EXPECT_FLOAT_EQ(asciiDM->diffTrans, 1.3f);
  EXPECT_FLOAT_EQ(asciiDM->eta, 1.2f);
  EXPECT_FLOAT_EQ(asciiDM->flatness, .21f);
  EXPECT_FLOAT_EQ(asciiDM->metallic, .5f);
  EXPECT_FLOAT_EQ(asciiDM->roughness, .7f);
  EXPECT_FLOAT_EQ(asciiDM->sheen, .1f);
  EXPECT_FLOAT_EQ(asciiDM->sheenTint, .8f);
  EXPECT_FLOAT_EQ(asciiDM->specTrans, .04f);
  EXPECT_FLOAT_EQ(asciiDM->specularTint, .05f);
  EXPECT_EQ(asciiDM->thin, true);

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(binary->world->shapes.size(), ascii->world->shapes.size());
  ASSERT_NE(binary->world->shapes[0], nullptr);

  DisneyMaterial::SP binaryDM = std::dynamic_pointer_cast<DisneyMaterial>(binary->world->shapes[0]->material);
  ASSERT_NE(binaryDM, nullptr);

  EXPECT_FLOAT_EQ(asciiDM->anisotropic, binaryDM->anisotropic);
  EXPECT_FLOAT_EQ(asciiDM->clearCoat, binaryDM->clearCoat);
  EXPECT_FLOAT_EQ(asciiDM->clearCoatGloss, binaryDM->clearCoatGloss);
  EXPECT_FLOAT_EQ(asciiDM->color.x, binaryDM->color.x);
  EXPECT_FLOAT_EQ(asciiDM->color.y, binaryDM->color.y);
  EXPECT_FLOAT_EQ(asciiDM->color.z, binaryDM->color.z);
  EXPECT_FLOAT_EQ(asciiDM->diffTrans, binaryDM->diffTrans);
  EXPECT_FLOAT_EQ(asciiDM->eta, binaryDM->eta);
  EXPECT_FLOAT_EQ(asciiDM->flatness, binaryDM->flatness);
  EXPECT_FLOAT_EQ(asciiDM->metallic, binaryDM->metallic);
  EXPECT_FLOAT_EQ(asciiDM->roughness, binaryDM->roughness);
  EXPECT_FLOAT_EQ(asciiDM->sheen, binaryDM->sheen);
  EXPECT_FLOAT_EQ(asciiDM->sheenTint, binaryDM->sheenTint);
  EXPECT_FLOAT_EQ(asciiDM->specTrans, binaryDM->specTrans);
  EXPECT_FLOAT_EQ(asciiDM->specularTint, binaryDM->specularTint);
  EXPECT_EQ(asciiDM->thin, binaryDM->thin);
}


// =======================================================
// Film
// =======================================================

TEST(PbrtParser, Film)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        Film "image" "integer xresolution" [1280] "integer yresolution" [720]
          "float cropwindow" [.3 .8 .15 .7] 
          "string filename" "film.exr"
          "float scale" 4
    )";

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->film, nullptr);

  EXPECT_FLOAT_EQ(ascii->film->resolution.x, 1280);
  EXPECT_FLOAT_EQ(ascii->film->resolution.y, 720);
  EXPECT_EQ(ascii->film->fileName, "film.exr");

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->film, nullptr);

  EXPECT_FLOAT_EQ(ascii->film->resolution.x, binary->film->resolution.x);
  EXPECT_FLOAT_EQ(ascii->film->resolution.y, binary->film->resolution.y);
  EXPECT_EQ(ascii->film->fileName, binary->film->fileName);
}


// =======================================================
// Attribute stack
// =======================================================

TEST(PbrtParser, AttributeStack)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            AttributeBegin
                Material "glass"
                Translate 1 1 1
                Shape "sphere" "float radius" 1

                AttributeBegin
                    Material "matte"
                    Translate 1 1 1
                    Shape "sphere" "float radius" 2
                    Shape "sphere" "float radius" 2

                    AttributeBegin
                        Material "uber"
                        Translate 1 1 1
                        Shape "sphere" "float radius" 3
                        Shape "sphere" "float radius" 3
                        Shape "sphere" "float radius" 3

                        AttributeBegin
                            Material "fourier"
                            Translate 1 1 1
                            Shape "sphere" "float radius" 4
                            Shape "sphere" "float radius" 4
                            Shape "sphere" "float radius" 4
                            Shape "sphere" "float radius" 4

                            AttributeBegin
                                Material "substrate"
                                Translate 1 1 1
                                Shape "sphere" "float radius" 5
                                Shape "sphere" "float radius" 5
                                Shape "sphere" "float radius" 5
                                Shape "sphere" "float radius" 5
                                Shape "sphere" "float radius" 5

                                AttributeBegin
                                    Material "metal"
                                    Translate 1 1 1
                                    Shape "sphere" "float radius" 6
                                    Shape "sphere" "float radius" 6
                                    Shape "sphere" "float radius" 6
                                    Shape "sphere" "float radius" 6
                                    Shape "sphere" "float radius" 6
                                    Shape "sphere" "float radius" 6

                                    AttributeBegin
                                        Material "disney"
                                        Translate 1 1 1
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7
                                        Shape "sphere" "float radius" 7

                                    AttributeEnd
                                AttributeEnd
                            AttributeEnd
                        AttributeEnd
                    AttributeEnd
                AttributeEnd
            AttributeEnd
        WorldEnd
    )";

  static constexpr int n = 7; // levels
  static constexpr int numShapesExpected = (n*(n+1))/2;

  Scene::SP ascii  = nullptr;
  Scene::SP binary = nullptr;

  parse(pbrt, ascii, binary);

  // First test ascii
  ASSERT_NE(ascii, nullptr);
  ASSERT_NE(ascii->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), numShapesExpected);

  int level = 1;
  for (int i = 0; i < numShapesExpected; i += level++)
  {
    for (int j = i; j < i + level; ++j)
    {
      Shape::SP shape = ascii->world->shapes[j];
      Sphere::SP sphere = std::dynamic_pointer_cast<Sphere>(shape);
      ASSERT_NE(sphere, nullptr);

      EXPECT_FLOAT_EQ(sphere->radius, (float)level);
      EXPECT_FLOAT_EQ(sphere->transform.p.x, (float)level);
      EXPECT_FLOAT_EQ(sphere->transform.p.y, (float)level);
      EXPECT_FLOAT_EQ(sphere->transform.p.z, (float)level);

      if (level == 1)
        EXPECT_NE(std::dynamic_pointer_cast<GlassMaterial>(shape->material), nullptr);
      else if (level == 2)
        EXPECT_NE(std::dynamic_pointer_cast<MatteMaterial>(shape->material), nullptr);
      else if (level == 3)
        EXPECT_NE(std::dynamic_pointer_cast<UberMaterial>(shape->material), nullptr);
      else if (level == 4)
        EXPECT_NE(std::dynamic_pointer_cast<FourierMaterial>(shape->material), nullptr);
      else if (level == 5)
        EXPECT_NE(std::dynamic_pointer_cast<SubstrateMaterial>(shape->material), nullptr);
      else if (level == 6)
        EXPECT_NE(std::dynamic_pointer_cast<MetalMaterial>(shape->material), nullptr);
      else if (level == 7)
        EXPECT_NE(std::dynamic_pointer_cast<DisneyMaterial>(shape->material), nullptr);
    }
  }

  // Make sure ascii and pbf are the same!
  ASSERT_NE(binary, nullptr);
  ASSERT_NE(binary->world, nullptr);
  ASSERT_EQ(ascii->world->shapes.size(), binary->world->shapes.size());

  for (std::size_t i = 0; i < ascii->world->shapes.size(); ++i)
  {
    Sphere::SP asciiSphere = std::dynamic_pointer_cast<Sphere>(ascii->world->shapes[i]);
    Sphere::SP binarySphere = std::dynamic_pointer_cast<Sphere>(binary->world->shapes[i]);
    ASSERT_NE(binarySphere, nullptr);

    EXPECT_FLOAT_EQ(asciiSphere->radius, binarySphere->radius);
    EXPECT_FLOAT_EQ(asciiSphere->transform.p.x, binarySphere->transform.p.x);
    EXPECT_FLOAT_EQ(asciiSphere->transform.p.y, binarySphere->transform.p.y);
    EXPECT_FLOAT_EQ(asciiSphere->transform.p.z, binarySphere->transform.p.z);

    // TODO: what if -fno-rtti ?!
    EXPECT_EQ(typeid(asciiSphere->material), typeid(binarySphere->material));
  }
}


// =======================================================
// Animated transformations
// =======================================================

TEST(PbrtParser, AnimatedTransformations)
{
  typename Stream::SP pbrt = std::make_shared<Stream>();

  (*pbrt) << R"(
        WorldBegin
            Translate 1 0 0

            ActiveTransform StartTime
            Translate 4 0 0

            ActiveTransform EndTime
            Translate 0 1 0

            ActiveTransform All
            Translate 0 0 2

            Shape "sphere" "float radius" 1

            TransformBegin
                ActiveTransform StartTime
                Translate -4 0 0

                ActiveTransform EndTime
                Translate 0 -1 0

                Shape "sphere" "float radius" 1
            TransformEnd

            Shape "sphere" "float radius" 1
        WorldEnd
    )";

  // Transforms with linear motion are currently only
  // parsed by the _syntactic_ parser
  IParser parser;
  parser.parse<std::stringstream>(pbrt);

  syntactic::Scene::SP sc = parser.getScene();

  ASSERT_NE(sc, nullptr);
  ASSERT_NE(sc->world, nullptr);
  ASSERT_EQ(sc->world->shapes.size(), 3);

  using pbrt::syntactic::affine3f;

  affine3f atStart, atEnd;

  // Before Transform{Begin|End}
  atStart = sc->world->shapes[0]->transform.atStart;
  EXPECT_FLOAT_EQ(atStart.p.x, 5.f);
  EXPECT_FLOAT_EQ(atStart.p.y, 0.f);
  EXPECT_FLOAT_EQ(atStart.p.z, 2.f);

  atEnd = sc->world->shapes[0]->transform.atEnd;
  EXPECT_FLOAT_EQ(atEnd.p.x, 1.f);
  EXPECT_FLOAT_EQ(atEnd.p.y, 1.f);
  EXPECT_FLOAT_EQ(atEnd.p.z, 2.f);

  // Inside Transform{Begin|End}
  atStart = sc->world->shapes[1]->transform.atStart;
  EXPECT_FLOAT_EQ(atStart.p.x, 1.f);
  EXPECT_FLOAT_EQ(atStart.p.y, 0.f);
  EXPECT_FLOAT_EQ(atStart.p.z, 2.f);

  atEnd = sc->world->shapes[1]->transform.atEnd;
  EXPECT_FLOAT_EQ(atEnd.p.x, 1.f);
  EXPECT_FLOAT_EQ(atEnd.p.y, 0.f);
  EXPECT_FLOAT_EQ(atEnd.p.z, 2.f);

  // After Transform{Begin|End}
  atStart = sc->world->shapes[2]->transform.atStart;
  EXPECT_FLOAT_EQ(atStart.p.x, 5.f);
  EXPECT_FLOAT_EQ(atStart.p.y, 0.f);
  EXPECT_FLOAT_EQ(atStart.p.z, 2.f);

  atEnd = sc->world->shapes[2]->transform.atEnd;
  EXPECT_FLOAT_EQ(atEnd.p.x, 1.f);
  EXPECT_FLOAT_EQ(atEnd.p.y, 1.f);
  EXPECT_FLOAT_EQ(atEnd.p.z, 2.f);
}
