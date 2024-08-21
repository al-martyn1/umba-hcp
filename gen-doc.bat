@rem set BATCH_ROOT=--batch-output-path=C:\work\temp\mdpp-test
@set EXCLUDE_DIRS=--batch-exclude-dir=_libs,libs,_lib,lib,tests,test,rc,_generators,_distr_conf,src,.msvc2019,boost,icons
@set COMMON=--overwrite --copy-images --add-examples-path=.\src
@rem set GRAPHVIZ_OPTIONS=--graphviz-output-path=.\.doc.generated_images
@rem set TEMP_OPTS=--graphviz-keep-temp-dot-files
@set OPTIONS= %TEMP_OPTS% %COMMON% %VARS% %EXCLUDE% %GRAPHVIZ_OPTIONS% --batch-scan-recurse=.

umba-md-pp %OPTIONS%

@rem C:\work\temp\mdpp-test\ 

