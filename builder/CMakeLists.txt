 #Nous voulons un cmake "récent" pour utiliser les dernières fonctionnalités
cmake_minimum_required(VERSION 3.0)

# Notre projet est étiqueté libhello

project(zdaq_builder VERSION 1.0.0 DESCRIPTION "zdaq builder tools")
include(../CMake.defs)

message(${CMAKE_SYSTEM_PROCESSOR})

include(CPack)


set(SRCS
  src/localCollector.cc
  src/builder.cc
  )

set(HEADERS
  include/localCollector.hh
)

add_executable(zdaq_builder ${SRCS} ${HEADERS})



target_link_libraries(zdaq_builder PRIVATE Threads::Threads)

target_include_directories(zdaq_builder PRIVATE include ../include )
target_link_libraries(zdaq_builder PRIVATE zdaq ${ZDAQ_LIBS})

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

install(TARGETS zdaq_builder  DESTINATION bin  ARCHIVE DESTINATION bin)
#install(CODE "execute_process(COMMAND chmod -R +x ${CMAKE_INSTALL_PREFIX}/bin/)")
