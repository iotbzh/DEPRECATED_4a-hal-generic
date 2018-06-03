###########################################################################
# Copyright 2015, 2016, 2017, 2018 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
# contrib: Romain Forlot <romain.forlot@iot.bzh>
# conrtib: Jonathan Aillet <jonathan.aillet@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

# Add target to project dependency list
PROJECT_TARGET_ADD(4a-hal)

	# Define project Targets
	add_library(${TARGET_NAME} MODULE
		    ${TARGET_NAME}-utilities/${TARGET_NAME}-utilities-appfw-responses-handler.c
		    ${TARGET_NAME}-utilities/${TARGET_NAME}-utilities-alsa-link.c
		    ${TARGET_NAME}-utilities/${TARGET_NAME}-utilities-data.c
		    ${TARGET_NAME}-utilities/${TARGET_NAME}-utilities-verbs-loader.c
		    ${TARGET_NAME}-manager/${TARGET_NAME}-manager.c
		    ${TARGET_NAME}-manager/${TARGET_NAME}-manager-cb.c
		    ${TARGET_NAME}-manager/${TARGET_NAME}-manager-events.c
		    ${TARGET_NAME}-controllers/${TARGET_NAME}-controllers-api-loader.c
		    ${TARGET_NAME}-controllers/${TARGET_NAME}-controllers-cb.c
		    ${TARGET_NAME}-controllers/${TARGET_NAME}-controllers-mixer-handler.c
		    )

	# Binder exposes a unique public entry point
	SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
		PREFIX ""
		LABELS "BINDING"		# Need to be in V3
		LINK_FLAGS ${BINDINGS_LINK_FLAG}
		OUTPUT_NAME ${TARGET_NAME}
	)

	# Library dependencies (include updates automatically)
	TARGET_LINK_LIBRARIES(${TARGET_NAME}
		afb-helpers
		ctl-utilities
		${link_libraries}
	)

	# Define target includes for this target client
	TARGET_INCLUDE_DIRECTORIES(${TARGET_NAME}
		PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	)
