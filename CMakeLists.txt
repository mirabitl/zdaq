 #Nous voulons un cmake "récent" pour utiliser les dernières fonctionnalités
cmake_minimum_required(VERSION 3.0)

# Notre projet est étiqueté libhello

project(zdaq VERSION 1.0.0 DESCRIPTION "zdaq base library")

#package Settings
if(NOT UNIX AND NOT APPLE)
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Laurent Mirabito")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
endif()
set(CPACK_PACKAGE_NAME "zdaq")
set(CPACK_PACKAGE_VENDOR "ip2i")
set(CPACK_VERBATIM_VARIABLES true)
set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_SET_DESTDIR true)
set(CPACK_INSTALL_PREFIX /usr/local/zdaq)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
  set(CPACK_SYSTEM_NAME "Linux-arm")
  set(CPACK_TOPLEVEL_TAG "Linux-arm")
endif()

if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
  set(CPACK_SYSTEM_NAME "Linux-x86_64")
  set(CPACK_TOPLEVEL_TAG "Linux-x86_64")
endif()

#set(FAKE_TARGET fakeTarget)
#if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/include/netlink/config.h )
#  add_custom_target(fakeTarget  echo "netLink already installed")
#else()
#  if (NOT APPLE AND UNIX)
#    add_custom_target(fakeTarget  ${CMAKE_CURRENT_SOURCE_DIR}/extras/install_netlink)
#  else()
#    add_custom_target(fakeTarget  echo "netLink not yet installed")
#  endif()
#endif()

include(CMake.defs)

message(${CMAKE_SYSTEM_PROCESSOR})
add_subdirectory(builder)
add_subdirectory(example)
add_subdirectory(src/pluggins)
add_subdirectory(monitoring/daemon)
#add_subdirectory(evb)
#add_subdirectory(plugins/demo)
#if (NOT APPLE)
#  add_subdirectory(plugins/zup)
#  add_subdirectory(plugins/genesys)
#  add_subdirectory(plugins/wiener)
#endif()
#add_subdirectory(plugins/grStore)
#add_subdirectory(plugins/mgStore)
#if (CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
#  set(CPACK_SYSTEM_NAME "Linux-arm")
#  set(CPACK_TOPLEVEL_TAG "Linux-arm")
#  add_subdirectory(plugins/bmp)
#  add_subdirectory(plugins/hih)
#endif()

#if (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
#  set(CPACK_SYSTEM_NAME "Linux-x86_64")
#  set(CPACK_TOPLEVEL_TAG "Linux-x86_64")
#  if (NOT APPLE)
#    add_subdirectory(plugins/syx27)
#  endif()
#endif()
#add_subdirectory(lyon/db)
#add_subdirectory(lyon/mpi)
#add_subdirectory(lyon/febv1)
#add_subdirectory(lyon/mbmdcc)
#add_subdirectory(lyon/gricv0)
#add_subdirectory(lyon/gricv1)
#add_subdirectory(lyon/pmr)
#add_subdirectory(lyon/mdcc)

include(CPack)


set(SRCS
  src/baseApplication.cc
  src/fileTailer.cc
  src/fsm.cc
  src/fsmjob.cc
  src/fsmmessage.cc
  src/fsmwebCaller.cc
  src/fsmweb.cc
  src/zmMerger.cc
  src/zmPublisher.cc
  src/zmPuller.cc
  src/zmPusher.cc
  src/zmRouter.cc
  src/zmSender.cc
  src/zPublisher.cc
  src/zSubscriber.cc
  )

set(HEADERS
  include/baseApplication.hh
  include/binarywriter.hh
  include/dummywriter.hh
  include/fileTailer.hh
  include/fsm.hh
  include/fsmjob.hh
  include/fsmmessage.hh
  include/fsmwebCaller.hh
  include/fsmweb.hh
  include/monitorApplication.hh
  include/zdaqLogger.hh
  include/zmBuffer.hh
  include/zmMerger.hh
  include/zmonPlugin.hh
  include/zmonStore.hh
  include/zmPublisher.hh
  include/zmPuller.hh
  include/zmPusher.hh
  include/zmRouter.hh
  include/zmSender.hh
  include/zPublisher.hh
  include/zSubscriber.hh
	
)

add_library(zdaq SHARED ${SRCS} ${HEADERS})

#add_dependencies(zdaq ${FAKE_TARGET})






target_link_libraries(zdaq PRIVATE ${ZDAQ_LIBS})

#SET(log4cxx_DIR /usr/lib/x86_64-linux-gnu/cmake)
#find_package(log4cxx CONFIG REQUIRED)
#target_link_libraries(zdaq PRIVATE log4cxx)

target_include_directories(zdaq PRIVATE include )


# Créé des variables avec les fichiers à compiler

# La bibliothèque peut être soit une bibliothèque statique, soit dynamique, donc on précise où installer pour les deux cas
install(TARGETS zdaq  LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)


add_executable(ljc src/utils/ljc.cxx ${HEADERS})



target_link_libraries(ljc PRIVATE Threads::Threads)

target_include_directories(ljc PRIVATE include )
target_link_libraries(ljc PRIVATE zdaq ${ZDAQ_LIBS})

file(GLOB SCRIPT_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*.py"
     )
file(GLOB MG_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/scripts/mg*"
     )
file(GLOB DAEMON_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/scripts/*daemon"
     )
file(GLOB SH_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/etc/*.sh"
     )
   
file(GLOB ETC_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/etc/*.*"
     )
file(GLOB MONETC_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/monitoring/daemon/etc/*"
     )
file(GLOB INCLUDE_TARGET
     "${CMAKE_CURRENT_SOURCE_DIR}/include/*"
     )

install(FILES ${INCLUDE_TARGET}  DESTINATION include )
install(FILES ${SCRIPT_TARGET}  DESTINATION share )
install(FILES ${ETC_TARGET} DESTINATION etc)
install(FILES ${MG_TARGET} ${DAEMON_TARGET} ${SH_TARGET} DESTINATION bin)

install(TARGETS ljc  DESTINATION bin  ARCHIVE DESTINATION bin)
install(CODE "execute_process(COMMAND chmod -R +x ${CMAKE_INSTALL_PREFIX}/bin/)")
