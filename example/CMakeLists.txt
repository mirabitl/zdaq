 #Nous voulons un cmake "récent" pour utiliser les dernières fonctionnalités
cmake_minimum_required(VERSION 3.0)

# Notre projet est étiqueté libhello

project(zdaq_example VERSION 1.0.0 DESCRIPTION "zdaq example tools")
include(../CMake.defs)

message(${CMAKE_SYSTEM_PROCESSOR})

include(CPack)


set(builder_SRCS   src/exBuilder.cc src/exBuilder.cxx )
set(builder_HEADERS  include/exBuilder.hh )
add_executable(zdaq_exbuilder ${builder_SRCS} ${builder_HEADERS})
target_link_libraries(zdaq_exbuilder PRIVATE Threads::Threads)
target_include_directories(zdaq_exbuilder PRIVATE include ../include )
target_link_libraries(zdaq_exbuilder PRIVATE zdaq ${ZDAQ_LIBS})

set(server_SRCS   src/exServer.cc src/exServer.cxx )
set(server_HEADERS  include/exServer.hh )
add_executable(zdaq_exserver ${server_SRCS} ${server_HEADERS})
target_link_libraries(zdaq_exserver PRIVATE Threads::Threads)
target_include_directories(zdaq_exserver PRIVATE include ../include )
target_link_libraries(zdaq_exserver PRIVATE zdaq ${ZDAQ_LIBS})

set(softtrigger_SRCS   src/softTrigger.cc src/softTrigger.cxx )
set(softtrigger_HEADERS  include/softTrigger.hh )
add_executable(zdaq_exsofttrigger ${softtrigger_SRCS} ${softtrigger_HEADERS})
target_link_libraries(zdaq_exsofttrigger PRIVATE Threads::Threads)
target_include_directories(zdaq_exsofttrigger PRIVATE include ../include )
target_link_libraries(zdaq_exsofttrigger PRIVATE zdaq ${ZDAQ_LIBS})

set(runcontrol_SRCS   src/exRunControl.cc src/exRunControl.cxx )
set(runcontrol_HEADERS  include/exRunControl.hh )
add_executable(zdaq_exruncontrol ${runcontrol_SRCS} ${runcontrol_HEADERS})
target_link_libraries(zdaq_exruncontrol PRIVATE Threads::Threads)
target_include_directories(zdaq_exruncontrol PRIVATE include ../include )
target_link_libraries(zdaq_exruncontrol PRIVATE zdaq ${ZDAQ_LIBS})

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

install(TARGETS zdaq_exbuilder  zdaq_exserver zdaq_exruncontrol zdaq_exsofttrigger DESTINATION bin  ARCHIVE DESTINATION bin)
#install(CODE "execute_process(COMMAND chmod -R +x ${CMAKE_INSTALL_PREFIX}/bin/)")
