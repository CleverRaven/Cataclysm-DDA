IF(GIT_EXECUTABLE)
	EXECUTE_PROCESS(
		COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty --match "[0-9]*.[0-9]*"
		OUTPUT_VARIABLE VERSION
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
ELSE(GIT_EXECUTABLE)
	MESSAGE(WARNING "Git binary not found. Build version will be set to NULL. Install Git package or use -DGIT_BINARY to set path to git binary.")
	SET (VERSION "NULL")
ENDIF(GIT_EXECUTABLE)

CONFIGURE_FILE(${SRC} ${DST} @ONLY)
