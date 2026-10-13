set(SCI_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/bolt_sci/src")
set(AUTHOR_SCI_SOURCES
  FloatingPoint/fp-math.cpp FloatingPoint/floating-point.cpp
  FloatingPoint/fixed-point.cpp FloatingPoint/bool-data.cpp
  BuildingBlocks/value-extension.cpp BuildingBlocks/aux-protocols.cpp
  BuildingBlocks/truncation.cpp Math/math-functions.cpp LinearOT/linear-ot.cpp
  GC/emp-tool.cpp GC/halfgate_gen.cpp GC/halfgate_eva.cpp)
list(TRANSFORM AUTHOR_SCI_SOURCES PREPEND "${SCI_ROOT}/")
add_library(aqcf_sci STATIC ${AUTHOR_SCI_SOURCES})
target_include_directories(aqcf_sci PUBLIC "${SCI_ROOT}" "${AQCF_EIGEN_DIR}" "${GMP_INCLUDE_DIR}")
target_compile_options(aqcf_sci PUBLIC -maes -msse4.1 -mavx -mavx2 -faligned-new)
target_compile_options(aqcf_sci PRIVATE -O3)
target_compile_definitions(aqcf_sci PUBLIC EMP_USE_RANDOM_DEVICE=1)
target_link_libraries(aqcf_sci PUBLIC Threads::Threads OpenMP::OpenMP_CXX OpenSSL::SSL OpenSSL::Crypto "${GMP_LIBRARY}")
add_library(aqcf_serial_sci STATIC
  serial_sci/BuildingBlocks/aux-protocols.cpp
  serial_sci/BuildingBlocks/truncation.cpp
  serial_sci/BuildingBlocks/value-extension.cpp)
target_include_directories(aqcf_serial_sci BEFORE PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/serial_sci")
target_link_libraries(aqcf_serial_sci PUBLIC aqcf_sci)
target_compile_options(aqcf_serial_sci PRIVATE -O3)
