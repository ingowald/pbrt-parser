
ADD_LIBRARY(plib_pbrt
  Parser.cpp
  Scene.cpp
  parsePLY.cpp
  ../3rdParty/ply.cpp
  )
TARGET_LINK_LIBRARIES(plib_pbrt plib_common)

ADD_EXECUTABLE(pbrtInfo info.cpp)
TARGET_LINK_LIBRARIES(pbrtInfo plib_pbrt)

ADD_EXECUTABLE(pbrt2obj pbrt2obj.cpp)
TARGET_LINK_LIBRARIES(pbrt2obj plib_pbrt)
