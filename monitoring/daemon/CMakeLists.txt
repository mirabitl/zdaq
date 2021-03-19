 #Nous voulons un cmake "récent" pour utiliser les dernières fonctionnalités
cmake_minimum_required(VERSION 3.0)

# Notre projet est étiqueté libhello

project(zdaq_monitoring VERSION 1.0.0 DESCRIPTION "zdaq monitoring daemon")
include(../../CMake.defs)

message(${CMAKE_SYSTEM_PROCESSOR})

include(CPack)


set(builder_SRCS   src/lscm.cc src/lsc.cxx )
set(builder_HEADERS  include/lsc.hh )
add_executable(lscm ${builder_SRCS} ${builder_HEADERS})
target_link_libraries(lscm PRIVATE Threads::Threads)
target_include_directories(lscm PRIVATE include ../../include )
target_link_libraries(lscm PRIVATE zdaq dl ${ZDAQ_LIBS})


file(GLOB SCRIPT_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.py"
     )
file(GLOB MG_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mg*"
     )
file(GLOB DAEMON_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/bin/*daemon"
     )
   
file(GLOB ETC_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/etc/*"
     )


#install(FILES ${SCRIPT_TARGET}  DESTINATION share )
#install(FILES ${ETC_TARGET} DESTINATION etc)
#install(FILES ${MG_TARGET} ${DAEMON_TARGET} DESTINATION bin)

install(TARGETS lscm DESTINATION bin  ARCHIVE DESTINATION bin)
#install(CODE "execute_process(COMMAND chmod -R +x ${CMAKE_INSTALL_PREFIX}/bin/)")
