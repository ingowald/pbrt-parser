ADD_EXECUTABLE(pbrt2biff 
  biff.cpp
  pbrt2biff.cpp
  )
TARGET_LINK_LIBRARIES(pbrt2biff
  pbrt_parser
)

