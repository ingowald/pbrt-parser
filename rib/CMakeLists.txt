find_program(LEX_EXE
    flex
)
if(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install flex!")
endif(LEX_EXE STREQUAL "LEX_EXE-NOTFOUND")

find_program(YACC_EXE
    bison
)
if(YACC_EXE STREQUAL "YACC_EXE-NOTFOUND")
    message(FATAL_ERROR "dear user, plase install bison!")
endif(YACC_EXE STREQUAL "YACC_EXE-NOTFOUND")

# reuseable cmake macro for yacc
MACRO(YACC_FILE _filename)
    GET_FILENAME_COMPONENT(_basename ${_filename} NAME_WE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.cpp
                ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_y.hpp
        COMMAND ${YACC_EXE} -d --debug --verbose
                --output=${_basename}_y.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
    )
ENDMACRO(YACC_FILE)

# reuseable cmake macro for lex
MACRO(LEX_FILE _filename)
    GET_FILENAME_COMPONENT(_basename ${_filename} NAME_WE)
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}_l.cpp
        COMMAND ${LEX_EXE}
                -o${_basename}_l.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/${_filename}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_filename} )
ENDMACRO(LEX_FILE)
# -------------------------------------------------------

YACC_FILE(parse.yy)
LEX_FILE(lex.ll)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(ribTest
  rib2pbf.cpp
  Parser.cpp
  lex_l.cpp
  parse_y.cpp
  )

TARGET_LINK_LIBRARIES(ribTest
  pbrtParser_semantic
  )


