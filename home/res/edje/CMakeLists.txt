FILE(GLOB edc_files ${CMAKE_CURRENT_SOURCE_DIR}/*.edc)
FOREACH(edc_file ${edc_files})
        GET_FILENAME_COMPONENT( FName ${edc_file} NAME_WE )
        MESSAGE("${FName}")
        ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/${FName}.edj
                        COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/images
                        ${CMAKE_CURRENT_SOURCE_DIR}/${FName}.edc ${FName}.edj ${EDJE_CFLAGS}
                        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FName}.edc
        )
        ADD_CUSTOM_TARGET(${FName}.edj DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${FName}.edj)
        ADD_DEPENDENCIES(${PROJECT_NAME} ${FName}.edj)
ENDFOREACH(edc_file)

# install edc files
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/ DESTINATION ${EDJEDIR} FILES_MATCHING PATTERN "*.edj")
