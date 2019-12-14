LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE       := waifu2x
LOCAL_CFLAGS       := -O3 -fshort-wchar -ffast-math -fomit-frame-pointer
LOCAL_CPPFLAGS     := -O3 -fshort-wchar -ffast-math -fomit-frame-pointer
LOCAL_CPP_FEATURES := # rtti exceptions
LOCAL_ARM_NEON     := true # force NEON usage for all files
LOCAL_SRC_FILES    := \
   ../../lib/src/Buffer.cpp \
   ../../lib/src/common.cpp \
   ../../lib/src/convertRoutine.cpp \
   ../../lib/src/cvwrap.cpp \
   ../../lib/src/modelHandler.cpp \
   ../../lib/src/modelHandler_neon.cpp \
   ../../lib/src/w2xconv.cpp

include $(BUILD_STATIC_LIBRARY)
