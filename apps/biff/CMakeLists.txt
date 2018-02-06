ADD_EXECUTABLE(pbrt2biff 
  biff.cpp
  pbrt2biff.cpp
  XML.cpp
  )
TARGET_LINK_LIBRARIES(pbrt2biff
  pbrt_parser
  )

ADD_EXECUTABLE(biffInfo
  biff.cpp
  info.cpp
  XML.cpp
  )
TARGET_LINK_LIBRARIES(biffInfo
  ospray_common
  )

