ADD_LIBRARY(biff SHARED
  biff.cpp
  XML.cpp
  )
TARGET_LINK_LIBRARIES(biff
  ospray_common
  )

ADD_EXECUTABLE(pbrt2biff 
  pbrt2biff.cpp
  )
TARGET_LINK_LIBRARIES(pbrt2biff
  pbrt_parser
  biff
  )

ADD_EXECUTABLE(biffInfo
  info.cpp
  )
TARGET_LINK_LIBRARIES(biffInfo
  biff
  )

