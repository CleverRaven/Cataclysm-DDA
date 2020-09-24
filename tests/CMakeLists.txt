IF(BUILD_TESTING)
	FILE(GLOB CATACLYSM_DDA_TEST_SOURCES
		${CMAKE_SOURCE_DIR}/tests/*.cpp)

	# Enabling benchmarks
 	ADD_DEFINITIONS(-DCATCH_CONFIG_ENABLE_BENCHMARKING)

	IF(TILES)
		add_executable(cata_test-tiles ${CATACLYSM_DDA_TEST_SOURCES})
		target_link_libraries(cata_test-tiles cataclysm-tiles-common)
		add_test(NAME test-tiles
			COMMAND sh -c
			"$<TARGET_FILE:cata_test-tiles> -r cata --rng-seed `shuf -i 0-1000000000 -n 1`"
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	ENDIF(TILES)

	IF(CURSES)
		add_executable(cata_test ${CATACLYSM_DDA_TEST_SOURCES})
		target_link_libraries(cata_test cataclysm-common)
		add_test(NAME test
			COMMAND sh -c
			"$<TARGET_FILE:cata_test> -r cata --rng-seed `shuf -i 0-1000000000 -n 1`"
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		)
	ENDIF(CURSES)
ENDIF(BUILD_TESTING)

# vim:noet
