# Copies the built images into firmware/builds as <name>_app_<YYYYMMDD>.hex and .uf2 - the publish target.
# Run as a script at build time (cmake -P), so the date is the day of publishing, not of configuring.
# Takes NAME, HEX_FILE, UF2_FILE and DEST_DIR.

string(TIMESTAMP DATE "%Y%m%d")
file(MAKE_DIRECTORY ${DEST_DIR})

foreach(SOURCE ${HEX_FILE} ${UF2_FILE})
  get_filename_component(EXTENSION ${SOURCE} LAST_EXT)
  set(DESTINATION "${DEST_DIR}/${NAME}_app_${DATE}${EXTENSION}")
  configure_file(${SOURCE} ${DESTINATION} COPYONLY)
  message(STATUS "Published ${DESTINATION}")
endforeach()
