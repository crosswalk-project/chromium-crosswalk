// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wtf/build_config.h"

#if ENABLE(WEBCL)
#include "modules/webcl/WebCLOpenCL.h"

#if OS(POSIX) || OS(ANDROID)
#include <dlfcn.h>
#endif
#include <string.h>
#include <stdio.h>
#include <wtf/CPU.h>

// Track different opencl libs.
#if OS(POSIX) || OS(ANDROID)
#if defined(WTF_CPU_ARM)
#define LIBS {"libOpenCL.so"}
#define SO_LEN 1

// After the byt, IA devices are shipped with "libOpenCL.so"
// by default. Before that, IA devices are shipped with
// "libPVROCL.so" to leverage PowerVR GPU for OpenCL.
//
// Note that there are some IA devices have "libOpenCL.so.1"
// but not "libOpenCL.so", such as: Asus Memo. So add "libOpenCL.so.1"
// to the OpenCL library list.
#elif defined(WTF_CPU_X86)
#define LIBS {"libOpenCL.so", "libOpenCL.so.1", "libPVROCL.so"}
#define SO_LEN 3

#else
#define LIBS {}
#define SO_LEN 0

#endif // defined(WTF_CPU_ARM) || defined(WTF_CPU_X86)
#endif // OS(POSIX) || OS(ANDROID)

/* Platform APIs */
cl_int (CL_API_CALL *web_clGetPlatformIDs)(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);

cl_int (CL_API_CALL *web_clGetPlatformInfo)(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clUnloadCompiler)(cl_platform_id platform);

cl_int (CL_API_CALL *web_clUnloadPlatformCompiler)(cl_platform_id platform);

/* Device APIs */
cl_int (CL_API_CALL *web_clGetDeviceInfo)(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clGetDeviceIDs)(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);

cl_int (CL_API_CALL *web_clReleaseDevice)(cl_device_id device);

/* Context APIs */
cl_int (CL_API_CALL *web_clGetContextInfo)(cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clReleaseContext)(cl_context context);

cl_context (CL_API_CALL *web_clCreateContext)(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_API_CALL* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret);

cl_context (CL_API_CALL *web_clCreateContextFromType)(const cl_context_properties* properties, cl_device_type device_type, void (CL_API_CALL *pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret);

/* CommandQueue APIs */
cl_int (CL_API_CALL *web_clGetCommandQueueInfo)(cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clReleaseCommandQueue)(cl_command_queue command_queue);

cl_command_queue (CL_API_CALL *web_clCreateCommandQueue)(cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret);

/* Memory Object APIs */
cl_mem (CL_API_CALL *web_clCreateBuffer)(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret);

cl_mem (CL_API_CALL *web_clCreateSubBuffer)(cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clGetMemObjectInfo)(cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clReleaseMemObject)(cl_mem memobj);

cl_int (CL_API_CALL *web_clGetImageInfo)(cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_mem (CL_API_CALL *web_clCreateImage2D)(cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret);

cl_mem (CL_API_CALL *web_clCreateImage)(cl_context context, cl_mem_flags flags, const cl_image_format* image_format, const cl_image_desc* image_desc, void* host_ptr, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clGetSupportedImageFormats)(cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats);

/* Sampler APIs */
cl_sampler (CL_API_CALL *web_clCreateSampler)(cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clGetSamplerInfo)(cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clReleaseSampler)(cl_sampler sampler);

/* Program Object APIs */
cl_int (CL_API_CALL *web_clGetProgramInfo)(cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clGetProgramBuildInfo)(cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clBuildProgram)(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_API_CALL *pfn_notify)(cl_program program, void* user_data), void* user_data);

cl_int (CL_API_CALL *web_clReleaseProgram)(cl_program program);

cl_program (CL_API_CALL *web_clCreateProgramWithSource)(cl_context context, cl_uint const, const char** strings, const size_t* lengths, cl_int* errcode_ret);

/* Kernel Object APIs */
cl_int (CL_API_CALL *web_clGetKernelInfo)(cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clGetKernelWorkGroupInfo)(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clSetKernelArg)(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value);

cl_int (CL_API_CALL *web_clReleaseKernel)(cl_kernel kernel);

cl_kernel (CL_API_CALL *web_clCreateKernel)(cl_program program, const char* kernel_name, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clCreateKernelsInProgram)(cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret);

/* Event Object APIs */
cl_event (CL_API_CALL *web_clCreateUserEvent)(cl_context context, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clWaitForEvents)(cl_uint num_events, const cl_event* event_list);

cl_int (CL_API_CALL *web_clGetEventInfo)(cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

cl_int (CL_API_CALL *web_clSetUserEventStatus)(cl_event event, cl_int execution_status);

cl_int (CL_API_CALL *web_clReleaseEvent)(cl_event event);

cl_int (CL_API_CALL *web_clSetEventCallback)(cl_event event, cl_int command_exec_callback_type, void (CL_API_CALL* pfn_event_notify)(cl_event event, cl_int event_command_exec_status, void* user_data), void* user_data);

cl_int (CL_API_CALL *web_clGetEventProfilingInfo)(cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

/* Flush and Finish APIs */
cl_int (CL_API_CALL *web_clFlush)(cl_command_queue command_queue);

cl_int (CL_API_CALL *web_clFinish)(cl_command_queue command_queue);

/* enqueue commands APIs */
cl_int (CL_API_CALL *web_clEnqueueReadBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueWriteBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueReadImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueWriteImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueCopyBuffer)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueBarrier)(cl_command_queue command_queue);

cl_int (CL_API_CALL *web_clEnqueueBarrierWithWaitList)(cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueMarker)(cl_command_queue command_queue, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueMarkerWithWaitList)(cl_command_queue command_queue, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueTask)(cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueReadBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueWriteBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueNDRangeKernel)(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueCopyBufferRect)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueCopyImage)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueCopyImageToBuffer)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueCopyBufferToImage)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueWaitForEvents)(cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list);

/* OpenCL Extention */
cl_int (CL_API_CALL *web_clEnqueueAcquireGLObjects)(cl_command_queue command_queue, cl_uint num_objects, const cl_mem* mem_objects, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_int (CL_API_CALL *web_clEnqueueReleaseGLObjects)(cl_command_queue command_queue, cl_uint num_objects, const cl_mem* mem_objects, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

cl_mem (CL_API_CALL *web_clCreateFromGLBuffer)(cl_context context, cl_mem_flags flags, GLuint bufobj, cl_int* errcode_ret);

cl_mem (CL_API_CALL *web_clCreateFromGLRenderbuffer)(cl_context context, cl_mem_flags flags, GLuint renderbuffer, cl_int* errcode_ret);

cl_mem (CL_API_CALL *web_clCreateFromGLTexture2D)(cl_context context, cl_mem_flags flags, GLenum texture_target, GLint miplevel, GLuint texture, cl_int* errcode_ret);

cl_mem (CL_API_CALL *web_clCreateFromGLTexture)(cl_context context, cl_mem_flags flags, GLenum texture_target, GLint miplevel, GLuint texture, cl_int* errcode_ret);

cl_int (CL_API_CALL *web_clGetGLTextureInfo)(cl_mem, cl_gl_texture_info, size_t, void *, size_t *);

// These aliases are missing from WebCLOpenCL.h. Put them here for internal use only.
#define clReleaseDevice web_clReleaseDevice
#define clCreateImage web_clCreateImage
#define clUnloadPlatformCompiler web_clUnloadPlatformCompiler
#define clEnqueueMarkerWithWaitList web_clEnqueueMarkerWithWaitList
#define clEnqueueBarrierWithWaitList web_clEnqueueBarrierWithWaitList
#define clCreateFromGLTexture web_clCreateFromGLTexture

#if OS(POSIX) || OS(ANDROID)
#define MAP_FUNC(fn)  { *(void**)(&fn) = dlsym(handle, #fn); }
#define MAP_FUNC_OR_BAIL(fn)  { *(void**)(&fn) = dlsym(handle, #fn); if(!fn) return false; }
// In case `fn' is not defined or deprecated in the OpenCL spec tagged by
// `major' and `minor', map `fn' to a wrapper implemented with APIs defined
// by this spec.
#define MAP_FUNC_TO_WRAPPER(fn, major, minor) { *(void**)(&fn) = (void*)fn##Impl##major##minor; }

static const char* DEFAULT_SO[] = LIBS;
static const int DEFAULT_SO_LEN = SO_LEN;
static void* handle = nullptr;
static bool getCLHandle(const char** libs, int length)
{
    for (int i = 0; i < length; ++i) {
        handle = dlopen(libs[i], RTLD_LAZY);
        if (handle)
            return true;
    }

    /* FAILURE: COULD NOT OPEN .SO */
    return false;
}
#endif // OS(POSIX) || OS(ANDROID)

// In OpenCL 1.1 spec, no release opertion is needed for device.
static cl_int CL_API_CALL clReleaseDeviceImpl11(cl_device_id device)
{
    return CL_SUCCESS;
}

static cl_mem CL_API_CALL clCreateImage2DImpl12(cl_context context, cl_mem_flags flags, const cl_image_format* format, size_t width, size_t height, size_t rowPitch, void* hostPtr, cl_int* err)
{
    cl_image_desc desc = {CL_MEM_OBJECT_IMAGE2D, static_cast<size_t>(width), static_cast<size_t>(height), 0, 0, static_cast<size_t>(rowPitch), 0, 0, 0, 0};
    ASSERT(clCreateImage);
    return clCreateImage(context, flags, format, &desc, hostPtr, err);
}

static cl_int CL_API_CALL clUnloadCompilerImpl12(cl_platform_id platform)
{
    ASSERT(clUnloadPlatformCompiler);
    return clUnloadPlatformCompiler(platform);
}

static cl_int CL_API_CALL clEnqueueMarkerImpl12(cl_command_queue queue, cl_event* event)
{
    ASSERT(clEnqueueMarkerWithWaitList);
    return clEnqueueMarkerWithWaitList(queue, 0, nullptr, event);
}

static cl_int CL_API_CALL clEnqueueBarrierImpl12(cl_command_queue queue)
{
    ASSERT(clEnqueueBarrierWithWaitList);
    return clEnqueueBarrierWithWaitList(queue, 0, nullptr, nullptr);
}

static cl_int CL_API_CALL clEnqueueWaitForEventsImpl12(cl_command_queue queue, cl_uint numEvents, const cl_event* eventList)
{
    ASSERT(clEnqueueBarrierWithWaitList);
    return clEnqueueBarrierWithWaitList(queue, numEvents, eventList, nullptr);
}

static cl_mem CL_API_CALL clCreateFromGLTexture2DImpl12(cl_context context, cl_mem_flags flags, GLenum textureTarget, GLint miplevel, GLuint texture, cl_int* err)
{
    ASSERT(clCreateFromGLTexture);
    return clCreateFromGLTexture(context, flags, textureTarget, miplevel, texture, err);
}

bool init(const char** libs, int length)
{
    const char** mLibs = (libs == 0 ? DEFAULT_SO : libs);
    int mLength = (libs == 0 ? DEFAULT_SO_LEN: length);

    if (!getCLHandle(mLibs, mLength))
        return false;

    MAP_FUNC_OR_BAIL(clBuildProgram);
    MAP_FUNC_OR_BAIL(clCreateBuffer);
    MAP_FUNC_OR_BAIL(clCreateCommandQueue);
    MAP_FUNC_OR_BAIL(clCreateContext);
    MAP_FUNC_OR_BAIL(clCreateContextFromType);
    MAP_FUNC_OR_BAIL(clCreateKernel);
    MAP_FUNC_OR_BAIL(clCreateKernelsInProgram);
    MAP_FUNC_OR_BAIL(clCreateProgramWithSource);
    MAP_FUNC_OR_BAIL(clCreateSampler);
    MAP_FUNC_OR_BAIL(clCreateSubBuffer);
    MAP_FUNC_OR_BAIL(clCreateUserEvent);

    MAP_FUNC_OR_BAIL(clEnqueueCopyBuffer);
    MAP_FUNC_OR_BAIL(clEnqueueReadBuffer);
    MAP_FUNC_OR_BAIL(clEnqueueWriteBuffer);
    MAP_FUNC_OR_BAIL(clEnqueueCopyImage);
    MAP_FUNC_OR_BAIL(clEnqueueReadImage);
    MAP_FUNC_OR_BAIL(clEnqueueWriteImage);
    MAP_FUNC_OR_BAIL(clEnqueueCopyBufferRect);
    MAP_FUNC_OR_BAIL(clEnqueueCopyBufferToImage);
    MAP_FUNC_OR_BAIL(clEnqueueCopyImageToBuffer);
    MAP_FUNC_OR_BAIL(clEnqueueReadBufferRect);
    MAP_FUNC_OR_BAIL(clEnqueueWriteBufferRect);
    MAP_FUNC_OR_BAIL(clEnqueueNDRangeKernel);
    MAP_FUNC_OR_BAIL(clEnqueueTask);

    MAP_FUNC_OR_BAIL(clFinish);
    MAP_FUNC_OR_BAIL(clFlush);

    MAP_FUNC_OR_BAIL(clGetContextInfo);
    MAP_FUNC_OR_BAIL(clGetCommandQueueInfo);
    MAP_FUNC_OR_BAIL(clGetDeviceIDs);
    MAP_FUNC_OR_BAIL(clGetDeviceInfo);
    MAP_FUNC_OR_BAIL(clGetEventInfo);
    MAP_FUNC_OR_BAIL(clGetEventProfilingInfo);
    MAP_FUNC_OR_BAIL(clGetImageInfo);
    MAP_FUNC_OR_BAIL(clGetKernelInfo);
    MAP_FUNC_OR_BAIL(clGetKernelWorkGroupInfo);
    MAP_FUNC_OR_BAIL(clGetPlatformIDs);
    MAP_FUNC_OR_BAIL(clGetSamplerInfo);
    MAP_FUNC_OR_BAIL(clGetSupportedImageFormats);

    MAP_FUNC_OR_BAIL(clGetMemObjectInfo);
    MAP_FUNC_OR_BAIL(clGetPlatformInfo);
    MAP_FUNC_OR_BAIL(clGetProgramBuildInfo);
    MAP_FUNC_OR_BAIL(clGetProgramInfo);

    MAP_FUNC_OR_BAIL(clReleaseCommandQueue);
    MAP_FUNC_OR_BAIL(clReleaseContext);
    MAP_FUNC_OR_BAIL(clReleaseEvent);
    MAP_FUNC_OR_BAIL(clReleaseKernel);
    MAP_FUNC_OR_BAIL(clReleaseMemObject);
    MAP_FUNC_OR_BAIL(clReleaseProgram);
    MAP_FUNC_OR_BAIL(clReleaseSampler);

    MAP_FUNC_OR_BAIL(clSetEventCallback);
    MAP_FUNC_OR_BAIL(clSetKernelArg);
    MAP_FUNC_OR_BAIL(clSetUserEventStatus);

    MAP_FUNC_OR_BAIL(clWaitForEvents);

    // They depends on whether OpenCL library support gl_sharing extension.
    // which aren't required mandatorily for OpenCL spec.
    MAP_FUNC(clEnqueueAcquireGLObjects);
    MAP_FUNC(clEnqueueReleaseGLObjects);
    MAP_FUNC(clCreateFromGLBuffer);
    MAP_FUNC(clCreateFromGLRenderbuffer);
    MAP_FUNC(clGetGLTextureInfo);

    // The following APIs are not available in all versions of the OpenCL
    // spec, so wrappers may be needed if they are not exported by the OpenCL
    // runtime library.
    MAP_FUNC(clReleaseDevice)
    if (!clReleaseDevice)
        MAP_FUNC_TO_WRAPPER(clReleaseDevice, 1, 1)

    MAP_FUNC(clCreateImage)
    if (clCreateImage)
        MAP_FUNC_TO_WRAPPER(clCreateImage2D, 1, 2)
    else
        MAP_FUNC_OR_BAIL(clCreateImage2D)

    MAP_FUNC(clUnloadPlatformCompiler)
    if (clUnloadPlatformCompiler)
        MAP_FUNC_TO_WRAPPER(clUnloadCompiler, 1, 2)
    else
        MAP_FUNC_OR_BAIL(clUnloadCompiler)

    MAP_FUNC(clEnqueueMarkerWithWaitList)
    if (clEnqueueMarkerWithWaitList)
        MAP_FUNC_TO_WRAPPER(clEnqueueMarker, 1, 2)
    else
        MAP_FUNC_OR_BAIL(clEnqueueMarker)

    MAP_FUNC(clEnqueueBarrierWithWaitList)
    if (clEnqueueBarrierWithWaitList)
        MAP_FUNC_TO_WRAPPER(clEnqueueBarrier, 1, 2)
    else
        MAP_FUNC_OR_BAIL(clEnqueueBarrier)

    MAP_FUNC(clEnqueueWaitForEvents)
    if (!clEnqueueWaitForEvents)
        MAP_FUNC_TO_WRAPPER(clEnqueueWaitForEvents, 1, 2)

    MAP_FUNC(clCreateFromGLTexture)
    if (clCreateFromGLTexture)
        MAP_FUNC_TO_WRAPPER(clCreateFromGLTexture2D, 1, 2)
    else
        MAP_FUNC(clCreateFromGLTexture2D)

    return true;
}

#endif // ENABLE(WEBCL)
