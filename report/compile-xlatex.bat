@echo off
set MAIN=article
set BUILD_DIR=.\

call clean.bat

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

@echo --------------------------------------------------------------
@echo -                     Inizializzazione                       -
@echo --------------------------------------------------------------

xelatex -output-directory=%BUILD_DIR% %MAIN%

call compile-bib.bat
call compile-gloss.bat

::pdflatex -output-directory=%BUILD_DIR% %MAIN%

::call compile-bib.bat
::call compile-gloss.bat

@echo --------------------------------------------------------------
@echo -                    Generazione Documento                   -
@echo --------------------------------------------------------------

xelatex -output-directory=%BUILD_DIR% %MAIN%
::pdflatex -output-directory=%BUILD_DIR% %MAIN%

call clean.bat

@echo ---------------------------- END -----------------------------



set /p keys=Done...
