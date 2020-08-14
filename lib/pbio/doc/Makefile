# You can set these variables from the command line.
DOXYGEN       = doxygen
DOXYGEN_CONF  = doxygen.conf

doc:
	@$(DOXYGEN) "$(DOXYGEN_CONF)"

clean:
	@rm -rf `grep "^OUTPUT_DIRECTORY" $(DOXYGEN_CONF) | cut -f 2 -d "="`
