BUILD_LIBAIO := true

ifeq ($(BUILD_LIBAIO), true)

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	src/io_cancel.c \
	src/io_destroy.c \
	src/io_getevents.c \
	src/io_queue_init.c \
	src/io_queue_release.c \
	src/io_queue_run.c \
	src/io_queue_wait.c \
	src/io_setup.c \
	src/io_submit.c \

LOCAL_C_INCLUDES := $(LOCAL_PATH)/src/
LOCAL_MODULE := libaio
LOCAL_MODULE_TAGS := optional
LOCAL_COPY_HEADERS_TO := libaio
LOCAL_COPY_HEADERS := src/libaio.h

include $(BUILD_SHARED_LIBRARY)

endif
