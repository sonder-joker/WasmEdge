# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

if(WASMEDGE_BUILD_AOT_RUNTIME)
  wasmedge_add_executable(wasmedgec
    wasmedgec.cpp
  )

  install(TARGETS wasmedgec EXPORT wasmedge
    DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

  if(WASMEDGE_LINK_TOOLS_STATIC)
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedgeCAPI
    )
  else()
    target_link_libraries(wasmedgec
      PRIVATE
      wasmedge_shared
    )
  endif()
endif()

wasmedge_add_executable(wasmedge
  wasmedger.cpp
)

install(TARGETS wasmedge EXPORT wasmedge
  DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(WASMEDGE_LINK_TOOLS_STATIC)
  target_link_libraries(wasmedge
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedge
    PRIVATE
    wasmedge_shared
  )
endif()
