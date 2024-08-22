@set EMPTY=
@set PROCESS_FILES_MASKS=*.cpp^,*.c^,*.cc^,*.cxx^,*.c++^,*.hpp^,*.h^,*.hxx^,*.h++^,*.md_^ %EMPTY%
@set EXCLUDE_FILES_MASKS=*/_libs/*^ %EMPTY%
@umba-tabtool --check --space --scan --include-files=%PROCESS_FILES_MASKS% --exclude-files=%EXCLUDE_FILES_MASKS% ./doc ./src ./tests

@rem umba-tabtool --help >umba-tabtool.txt

