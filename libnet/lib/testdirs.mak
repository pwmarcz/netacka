# This file exists to check the user expanded the stored directory
# structure properly.

.PHONY: bad_dir_structure

bad_dir_structure:
	@echo "You didn't tell your unzip program to recreate the directory"
	@echo "structure when unzipping this!"
	@echo ""
	@echo "You'll have to delete all this and start again. If you're"
	@echo "using PKUNZIP, pass it the `-d' switch."

