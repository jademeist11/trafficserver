#######################
#
#  Licensed to the Apache Software Foundation (ASF) under one or more contributor license
#  agreements.  See the NOTICE file distributed with this work for additional information regarding
#  copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0
#  (the "License"); you may not use this file except in compliance with the License.  You may obtain
#  a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software distributed under the License
#  is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
#  or implied. See the License for the specific language governing permissions and limitations under
#  the License.
#
#######################

add_subdirectory(common)
add_subdirectory(fetcher)
add_subdirectory(lib)

add_atsplugin(esi esi.cc serverIntercept.cc)
target_link_libraries(esi PRIVATE esi-common esicore fetcher libswoc::libswoc)
target_include_directories(esi PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
verify_global_plugin(esi)
verify_remap_plugin(esi)

add_atsplugin(combo_handler combo_handler.cc)

target_link_libraries(combo_handler PRIVATE esicore fetcher)
target_include_directories(combo_handler PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
verify_global_plugin(combo_handler)
verify_remap_plugin(combo_handler)

if(BUILD_TESTING)
  add_subdirectory(test)
endif()
