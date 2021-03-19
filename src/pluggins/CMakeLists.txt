 #Nous voulons un cmake "récent" pour utiliser les dernières fonctionnalités
cmake_minimum_required(VERSION 3.0)

# Notre projet est étiqueté libhello

project(evb_processors VERSION 0.0.1 DESCRIPTION "Event builder plugin")

include(../../CMake.defs)

      
add_library(proc_binarywriter SHARED binarywriter.cc ../../include/binarywriter.hh)
target_link_libraries(proc_binarywriter PRIVATE  zmq zdaq ${ZDAQ_LIBS} )
target_include_directories(proc_binarywriter PRIVATE  ../../include  )

install(TARGETS proc_binarywriter  LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)


add_library(proc_dummywriter SHARED dummywriter.cc ../../include/dummywriter.hh)
target_link_libraries(proc_dummywriter PRIVATE  zmq zdaq ${ZDAQ_LIBS} )
target_include_directories(proc_dummywriter PRIVATE  ../../include )

install(TARGETS proc_dummywriter  LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

# Créé des variables avec les fichiers à compiler

# La bibliothèque peut être soit une bibliothèque statique, soit dynamique, donc on précise où installer pour les deux cas


