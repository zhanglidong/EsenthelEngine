#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
GREP=grep
NM=nm
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=gfortran
AS=as

# Macros
CND_PLATFORM=GNU-Linux
CND_DLIB_EXT=so
CND_CONF=Debug
CND_DISTDIR=dist
CND_BUILDDIR=build

# Include project Makefile
include Makefile

# Object Directory
OBJECTDIR=${CND_BUILDDIR}/${CND_CONF}/${CND_PLATFORM}

# Object Files
OBJECTFILES= \
	${OBJECTDIR}/_ext/a59f760b/Buffer.o \
	${OBJECTDIR}/_ext/a59f760b/common.o \
	${OBJECTDIR}/_ext/a59f760b/convertRoutine.o \
	${OBJECTDIR}/_ext/a59f760b/cvwrap.o \
	${OBJECTDIR}/_ext/a59f760b/modelHandler.o \
	${OBJECTDIR}/_ext/a59f760b/modelHandler_avx.o \
	${OBJECTDIR}/_ext/a59f760b/modelHandler_fma.o \
	${OBJECTDIR}/_ext/a59f760b/modelHandler_sse.o \
	${OBJECTDIR}/_ext/a59f760b/w2xconv.o


# C Compiler Flags
CFLAGS=-m64

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Assembler Flags
ASFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS}
	"${MAKE}"  -f nbproject/Makefile-${CND_CONF}.mk ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblinux.a

${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblinux.a: ${OBJECTFILES}
	${MKDIR} -p ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}
	${RM} ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblinux.a
	${AR} -rv ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblinux.a ${OBJECTFILES} 
	$(RANLIB) ${CND_DISTDIR}/${CND_CONF}/${CND_PLATFORM}/liblinux.a

${OBJECTDIR}/_ext/a59f760b/Buffer.o: ../lib/src/Buffer.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/Buffer.o ../lib/src/Buffer.cpp

${OBJECTDIR}/_ext/a59f760b/common.o: ../lib/src/common.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/common.o ../lib/src/common.cpp

${OBJECTDIR}/_ext/a59f760b/convertRoutine.o: ../lib/src/convertRoutine.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/convertRoutine.o ../lib/src/convertRoutine.cpp

${OBJECTDIR}/_ext/a59f760b/cvwrap.o: ../lib/src/cvwrap.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/cvwrap.o ../lib/src/cvwrap.cpp

${OBJECTDIR}/_ext/a59f760b/modelHandler.o: ../lib/src/modelHandler.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/modelHandler.o ../lib/src/modelHandler.cpp

${OBJECTDIR}/_ext/a59f760b/modelHandler_avx.o: ../lib/src/modelHandler_avx.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/modelHandler_avx.o ../lib/src/modelHandler_avx.cpp

${OBJECTDIR}/_ext/a59f760b/modelHandler_fma.o: ../lib/src/modelHandler_fma.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/modelHandler_fma.o ../lib/src/modelHandler_fma.cpp

${OBJECTDIR}/_ext/a59f760b/modelHandler_sse.o: ../lib/src/modelHandler_sse.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/modelHandler_sse.o ../lib/src/modelHandler_sse.cpp

${OBJECTDIR}/_ext/a59f760b/w2xconv.o: ../lib/src/w2xconv.cpp
	${MKDIR} -p ${OBJECTDIR}/_ext/a59f760b
	${RM} "$@.d"
	$(COMPILE.cc) -g -MMD -MP -MF "$@.d" -o ${OBJECTDIR}/_ext/a59f760b/w2xconv.o ../lib/src/w2xconv.cpp

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r ${CND_BUILDDIR}/${CND_CONF}

# Subprojects
.clean-subprojects:

# Enable dependency checking
.dep.inc: .depcheck-impl

include .dep.inc
