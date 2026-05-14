# Retro-Frame Codepage Tool

**Endeavor: Retro-C**  
**Repository: \<[http://source.retro-c.net/util.stdc.rfcptool](http://source.retro-c.net/util.stdc.rfcptool)\>**  
**Version: 1.0!1 (alpha, dev)**  
**Environments: C90 [ C99 ]**  
**Compliance: Retro-Frame 1.0**  
**License: MIT (see `LICENSE`)**  

Copyright (c) 2026 Ingo Boehmer \<ingo@retro-leisure.net\>

All product names, logos, and brands are property of their respective owners.


## Contents

1. Overview

2. How to build an executable from the source

3. Usage

4. References


## 1. Overview

This repository contains the source code of a Retro-Frame Codepage Tool
(RFCPTOOL) written in Standard C (C90, conditionally using C99).

After build, the program can be run from the command line in order to encode or
decode binary CP files from or to codepage code (CPCODE) files. In addition,
CPCODE files can be created from codepoint specification (CPSPEC) files or the
native character set on compile time.

Specifications of those file formats can be found in the **Retro-Frame Data
Format Specifications** repository. Several codepage files in all formats can
be found in the **Retro-Frame Codepage Specifications** repository.

Development of the software is subject to Retro-C and complies to the
**Retro-Frame Common Documentation**.


## 2. How to build an executable from the source

The build of an executable from the source requires control files (e.g. build,
project and/or configuration files) depending on your development environment.

You may, of course, create your own control files and include the source files
from `src/` in order to build an executable. However, there are some control
files for selected development environments provided in `build/`.

### GCC / make

If you have the GNU Compiler Collection (GCC) and a make utility installed,
change to the directory `build/gcc/` and run the make utility (e.g. by `make`
or a similar command) in order to process the file `build/gcc/Makefile`.

The executable is created and stored in `build/gcc/bin` while temporary object
files are stored in `build/gcc/tmp`.

### Visual Studio

If you have Visual Studio installed, you may simply open one of the provided
solution files (`build/Visual Studio <version>/rfcptool.sln`). Each solution
file is implicitly linked to a Visual Studio project file (`rfcptool.vcxproj`).
Unfortunately, this file depends on two local prerequisites:

* Windows SDK and
* Platform toolset

While the platform toolset is related to the Visual Studio version, the Windows
SDK depends on the Windows version.

The project files provided match to specific Visual Studio versions (i.e. by
having the respective platform toolset versions configured). Choose the folder
of the Visual Studio version you are using.

If the Windows SDK version does not match, the build will fail with the
following error: "The Windows SDK version x.y was not found. [...]". In this
case, you may change the SDK version by right-clicking on the solution
respective project name (i.e. `rfcptool`) and then selecting "Retarget
solution" ("SDK-Version neu ausrichten" in German). You may choose any target
platform version which is offered.


## 3. Usage

After build, run the Retro-Frame Codepage Tool from the command line:

`rfcptool encode [ <option> ]* <src-file> <dest-file>`  
`rfcptool decode [ <option> ]* <src-file> [ <dest-file> ]`  
`rfcptool cpspec [ <option> ]* <reference> [ <dest-file> ]`  
`rfcptool native [ <option> ]* [ <dest-file> ]`  
`rfcptool verify <encoding> <src-file>`  
`rfcptool license`  

Arguments (encode):

* `<src-file>` is the path to a codepage code file (`*.CPC`).
* `<dest-file>` is the path to the resulting binary codepage file (`*.CP`).

Arguments (decode):

* `<src-file>` is the path to a binary codepage file (`*.CP`).
* `<dest-file>` is the path to the resulting codepage code file (`*.CPC`). If
  omitted, the contents is generated and written to stdout.

Arguments (cpspec):

* `<codepage>` is a codepage reference in the format `<domain>:<identifier>`
  where `<identifier>` represents either the name or the number of a codepage.
  The corresponding codepage specification file of the name `<domain>.CPS` is
  searched and the codepage, if found, is build according the standard rules
  (see **Retro-Frame Data Format Specifications**, `spec/rfdf-cpspec.txt`).
* `<dest-file>` is the path to the resulting binary codepage file (`*.CP`). If
  omitted, a codepage code file format is written to stdout.

Arguments (native):

* `<dest-file>` is the path to the resulting binary codepage file (`*.CP`). If
  omitted, a codepage code file format is generated and written to stdout.

Arguments (verify):

* `<encoding>` Character encoding to be verified (`ASCII`, `LATIN-1`, `PCS`,
  `UTF-8`, `UTF-8X` (UTF-8 relaxed), `CESU-8`, `CESU-8X` (CESU-8 relaxed),
  `UTF-16BE`, `UTF-16LE`, `UCS-2BE`, `UCS-2LE`, `UTF-32BE`, `UTF-32LE`,
  `UCS-4BE` or `UCS4-LE`).
* `<src-file>` is the path to a binary codepage file (`*.CP`).

Options:

* `-O` advises to overwrite an existing `<dest-file>`.
* `-CPSPECDIR <dir>` includes `<dir>` in codepage specification file (*.CPS)
  search. Note that the path must be complete because the filenames will be
  appended without any adjustion (e.g. use a trailing path separator if
  required).

The options are not case sensitive.


## 4. References

### Retro-Frame Codepage Specifications

Retro-Frame Codepage Specifications, see
\<[http://source.retro-frame.net/cp](http://source.retro-frame.net/cp)\>.

### Retro-Frame Common Documentation

Retro-Frame Common Documentation, see
\<[http://source.retro-frame.net/common](http://source.retro-frame.net/common)\>.

### Retro-Frame Data Format Specifications

Retro-Frame Data Format Specifications, see
\<[http://source.retro-frame.net/format](http://source.retro-frame.net/format)\>.
