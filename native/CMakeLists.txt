file(READ "${CMAKE_CURRENT_SOURCE_DIR}/build_recipe.json" RECIPE)
string(JSON UNIT_COUNT LENGTH "${RECIPE}")
math(EXPR UNIT_LAST "${UNIT_COUNT} - 1")
foreach(INDEX RANGE ${UNIT_LAST})
  string(JSON MODEL GET "${RECIPE}" ${INDEX} model)
  string(JSON NAME GET "${RECIPE}" ${INDEX} name)
  string(JSON SOURCE GET "${RECIPE}" ${INDEX} source)
  set(UNIT "${MODEL}_${NAME}")
  string(JSON FLAG_COUNT LENGTH "${RECIPE}" ${INDEX} flags)
  math(EXPR FLAG_LAST "${FLAG_COUNT} - 1")
  set(FLAGS "")
  foreach(F RANGE ${FLAG_LAST})
    string(JSON FLAG GET "${RECIPE}" ${INDEX} flags ${F})
    list(APPEND FLAGS "${FLAG}")
  endforeach()
  add_library(${UNIT} OBJECT "${SOURCE}")
  target_include_directories(${UNIT} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/deps/serial_sci" "${PROJECT_SOURCE_DIR}/deps/bolt_sci/src"
    "${AQCF_EIGEN_DIR}" "${GMP_INCLUDE_DIR}")
  target_compile_options(${UNIT} PRIVATE ${FLAGS})
  # Native OT and HE adapters have distinct recorded flags. In particular,
  # do not inherit OpenMP compilation from the linked SCI library here.
  target_link_libraries(${UNIT} PRIVATE SEAL::seal)
  list(APPEND ${MODEL}_OBJECTS "$<TARGET_OBJECTS:${UNIT}>")
endforeach()
foreach(MODEL squeezenet resnet50)
  add_executable(aqcf_${MODEL} ${${MODEL}_OBJECTS})
  target_link_libraries(aqcf_${MODEL} PRIVATE aqcf_serial_sci aqcf_sci SEAL::seal
    ZLIB::ZLIB "${ZSTD_LIBRARY}" Threads::Threads OpenMP::OpenMP_CXX
    OpenSSL::SSL OpenSSL::Crypto "${GMP_LIBRARY}" ${CMAKE_DL_LIBS})
  target_link_options(aqcf_${MODEL} PRIVATE -no-pie -rdynamic "-Wl,--wrap=bind"
    "-Wl,--wrap=_ZN4seal4util21ntt_negacyclic_harveyENS0_7PtrIterIPmEERKNS0_9NTTTablesE"
    "-Wl,--wrap=_ZN4seal4util26ntt_negacyclic_harvey_lazyENS0_7PtrIterIPmEERKNS0_9NTTTablesE"
    "-Wl,--wrap=_ZN4seal4util29inverse_ntt_negacyclic_harveyENS0_7PtrIterIPmEERKNS0_9NTTTablesE"
    "-Wl,--wrap=_ZN4seal4util34inverse_ntt_negacyclic_harvey_lazyENS0_7PtrIterIPmEERKNS0_9NTTTablesE")
endforeach()
target_link_options(aqcf_resnet50 PRIVATE "-Wl,--wrap=fwrite" "-Wl,--wrap=fread" "-Wl,--wrap=fflush")
