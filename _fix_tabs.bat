@set EMPTY=
@set PROCESS_FILES_MASKS=*.cpp^,*.c^,*.cc^,*.cxx^,*.c++^,*.hpp^,*.h^,*.hxx^,*.h++^,*.md_^ %EMPTY%
@set EXCLUDE_FILES_MASKS=*/_libs/*^ %EMPTY%
@umba-tabtool --tab-size=4 --tab-delta=1 --tab --remove-trailing-spaces=Y --scan --include-files=%PROCESS_FILES_MASKS% --exclude-files=%EXCLUDE_FILES_MASKS% ./doc ./src ./tests
