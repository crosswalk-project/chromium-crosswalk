// Copyright (C) 2015 Intel Corporation All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebCLOpenCL_h
#define WebCLOpenCL_h

#include "modules/webcl/WebCLConfig.h"

#ifdef __cplusplus
extern "C" {
#endif

bool init(const char** libs, int len); /* load libs in lib list. */

/* Platform APIs */
extern cl_int (CL_API_CALL *web_clGetPlatformIDs)(cl_uint num_entries, cl_platform_id* platforms, cl_uint* num_platforms);

extern cl_int (CL_API_CALL *web_clGetPlatformInfo)(cl_platform_id platform, cl_platform_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clUnloadCompiler)(cl_platform_id platform);

/* Device APIs */
extern cl_int (CL_API_CALL *web_clGetDeviceInfo)(cl_device_id device, cl_device_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clGetDeviceIDs)(cl_platform_id platform, cl_device_type device_type, cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);

/* Context APIs */
extern cl_int (CL_API_CALL *web_clGetContextInfo)(cl_context context, cl_context_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clReleaseContext)(cl_context context);

extern cl_context (CL_API_CALL *web_clCreateContext)(const cl_context_properties* properties, cl_uint num_devices, const cl_device_id* devices, void (CL_API_CALL* pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret);

extern cl_context (CL_API_CALL *web_clCreateContextFromType)(const cl_context_properties* properties, cl_device_type device_type, void (CL_API_CALL *pfn_notify)(const char* errinfo, const void* private_info, size_t cb, void* user_data), void* user_data, cl_int* errcode_ret);

/* CommandQueue APIs */
extern cl_int (CL_API_CALL *web_clGetCommandQueueInfo)(cl_command_queue command_queue, cl_command_queue_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clReleaseCommandQueue)(cl_command_queue command_queue);

extern cl_command_queue (CL_API_CALL *web_clCreateCommandQueue)(cl_context context, cl_device_id device, cl_command_queue_properties properties, cl_int* errcode_ret);

/* Memory Object APIs */
extern cl_mem (CL_API_CALL *web_clCreateBuffer)(cl_context context, cl_mem_flags flags, size_t size, void* host_ptr, cl_int* errcode_ret);

extern cl_mem (CL_API_CALL *web_clCreateSubBuffer)(cl_mem buffer, cl_mem_flags flags, cl_buffer_create_type buffer_create_type, const void* buffer_create_info, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clGetMemObjectInfo)(cl_mem memobj, cl_mem_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clReleaseMemObject)(cl_mem memobj);

extern cl_int (CL_API_CALL *web_clGetImageInfo)(cl_mem image, cl_image_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_mem (CL_API_CALL *web_clCreateImage2D)(cl_context context, cl_mem_flags flags, const cl_image_format* image_format, size_t image_width, size_t image_height, size_t image_row_pitch, void* host_ptr, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clGetSupportedImageFormats)(cl_context context, cl_mem_flags flags, cl_mem_object_type image_type, cl_uint num_entries, cl_image_format* image_formats, cl_uint* num_image_formats);

/* Sampler APIs */
extern cl_sampler (CL_API_CALL *web_clCreateSampler)(cl_context context, cl_bool normalized_coords, cl_addressing_mode addressing_mode, cl_filter_mode filter_mode, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clGetSamplerInfo)(cl_sampler sampler, cl_sampler_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clReleaseSampler)(cl_sampler sampler);

/* Program Object APIs */
extern cl_int (CL_API_CALL *web_clGetProgramInfo)(cl_program program, cl_program_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clGetProgramBuildInfo)(cl_program program, cl_device_id device, cl_program_build_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clBuildProgram)(cl_program program, cl_uint num_devices, const cl_device_id* device_list, const char* options, void (CL_API_CALL *pfn_notify)(cl_program program, void* user_data), void* user_data);

extern cl_int (CL_API_CALL *web_clReleaseProgram)(cl_program program);

extern cl_program (CL_API_CALL *web_clCreateProgramWithSource)(cl_context context, cl_uint const, const char** strings, const size_t* lengths, cl_int* errcode_ret);

/* Kernel Object APIs */
extern cl_int (CL_API_CALL *web_clGetKernelInfo)(cl_kernel kernel, cl_kernel_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clGetKernelWorkGroupInfo)(cl_kernel kernel, cl_device_id device, cl_kernel_work_group_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clSetKernelArg)(cl_kernel kernel, cl_uint arg_index, size_t arg_size, const void* arg_value);

extern cl_int (CL_API_CALL *web_clReleaseKernel)(cl_kernel kernel);

extern cl_kernel (CL_API_CALL *web_clCreateKernel)(cl_program program, const char* kernel_name, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clCreateKernelsInProgram)(cl_program program, cl_uint num_kernels, cl_kernel* kernels, cl_uint* num_kernels_ret);

/* Event Object APIs */
extern cl_event (CL_API_CALL *web_clCreateUserEvent)(cl_context context, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clWaitForEvents)(cl_uint num_events, const cl_event* event_list);

extern cl_int (CL_API_CALL *web_clGetEventInfo)(cl_event event, cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

extern cl_int (CL_API_CALL *web_clSetUserEventStatus)(cl_event event, cl_int execution_status);

extern cl_int (CL_API_CALL *web_clReleaseEvent)(cl_event event);

extern cl_int (CL_API_CALL *web_clSetEventCallback)(cl_event event, cl_int command_exec_callback_type, void (CL_API_CALL* pfn_event_notify)(cl_event event, cl_int event_command_exec_status, void* user_data), void* user_data);

extern cl_int (CL_API_CALL *web_clGetEventProfilingInfo)(cl_event event, cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret);

/* Flush and Finish APIs */
extern cl_int (CL_API_CALL *web_clFlush)(cl_command_queue command_queue);

extern cl_int (CL_API_CALL *web_clFinish)(cl_command_queue command_queue);

/* enqueue commands APIs */
extern cl_int (CL_API_CALL *web_clEnqueueReadBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, size_t offset, size_t size, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueWriteBuffer)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, size_t offset, size_t size, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueReadImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_read, const size_t* origin, const size_t* region, size_t row_pitch, size_t slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueWriteImage)(cl_command_queue command_queue, cl_mem image, cl_bool blocking_write, const size_t* origin, const size_t* region, size_t input_row_pitch, size_t input_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueCopyBuffer)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset, size_t size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueBarrier)(cl_command_queue command_queue);

extern cl_int (CL_API_CALL *web_clEnqueueMarker)(cl_command_queue command_queue, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueTask)(cl_command_queue command_queue, cl_kernel kernel, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueReadBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_read, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueWriteBufferRect)(cl_command_queue command_queue, cl_mem buffer, cl_bool blocking_write, const size_t* buffer_origin, const size_t* host_origin, const size_t* region, size_t buffer_row_pitch, size_t buffer_slice_pitch, size_t host_row_pitch, size_t host_slice_pitch, const void* ptr, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueNDRangeKernel)(cl_command_queue command_queue, cl_kernel kernel, cl_uint work_dim, const size_t* global_work_offset, const size_t* global_work_size, const size_t* local_work_size, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueCopyBufferRect)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, const size_t* src_origin, const size_t* dst_origin, const size_t* region, size_t src_row_pitch, size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueCopyImage)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_image, const size_t* src_origin, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueCopyImageToBuffer)(cl_command_queue command_queue, cl_mem src_image, cl_mem dst_buffer, const size_t* src_origin, const size_t* region, size_t dst_offset, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueCopyBufferToImage)(cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_image, size_t src_offset, const size_t* dst_origin, const size_t* region, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueWaitForEvents)(cl_command_queue command_queue, cl_uint num_events, const cl_event* event_list);

/* OpenCL Extention */
extern cl_int (CL_API_CALL *web_clEnqueueAcquireGLObjects)(cl_command_queue command_queue, cl_uint num_objects, const cl_mem* mem_objects, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_int (CL_API_CALL *web_clEnqueueReleaseGLObjects)(cl_command_queue command_queue, cl_uint num_objects, const cl_mem* mem_objects, cl_uint num_events_in_wait_list, const cl_event* event_wait_list, cl_event* event);

extern cl_mem (CL_API_CALL *web_clCreateFromGLBuffer)(cl_context context, cl_mem_flags flags, GLuint bufobj, cl_int* errcode_ret);

extern cl_mem (CL_API_CALL *web_clCreateFromGLRenderbuffer)(cl_context context, cl_mem_flags flags, GLuint renderbuffer, cl_int* errcode_ret);

extern cl_mem (CL_API_CALL *web_clCreateFromGLTexture2D)(cl_context context, cl_mem_flags flags, GLenum texture_target, GLint miplevel, GLuint texture, cl_int* errcode_ret);

extern cl_int (CL_API_CALL *web_clGetGLTextureInfo)(cl_mem, cl_gl_texture_info, size_t, void *, size_t *);

#ifdef __cplusplus
}
#endif

// Mapping real OpenCL function to the function pointer.
#define clGetImageInfo web_clGetImageInfo
#define clCreateBuffer web_clCreateBuffer
#define clCreateSubBuffer web_clCreateSubBuffer
#define clGetCommandQueueInfo web_clGetCommandQueueInfo
#define clEnqueueWriteBuffer web_clEnqueueWriteBuffer
#define clFinish web_clFinish
#define clFlush web_clFlush
#define clEnqueueReadBuffer web_clEnqueueReadBuffer
#define clReleaseCommandQueue web_clReleaseCommandQueue
#define clEnqueueWriteImage web_clEnqueueWriteImage
#define clEnqueueAcquireGLObjects web_clEnqueueAcquireGLObjects
#define clEnqueueReleaseGLObjects web_clEnqueueReleaseGLObjects
#define clEnqueueCopyBuffer web_clEnqueueCopyBuffer
#define clEnqueueBarrier web_clEnqueueBarrier
#define clEnqueueMarker web_clEnqueueMarker
#define clEnqueueTask web_clEnqueueTask
#define clEnqueueWriteBufferRect web_clEnqueueWriteBufferRect
#define clEnqueueReadBufferRect web_clEnqueueReadBufferRect
#define clEnqueueReadImage web_clEnqueueReadImage
#define clEnqueueNDRangeKernel web_clEnqueueNDRangeKernel
#define clEnqueueCopyBufferRect web_clEnqueueCopyBufferRect
#define clEnqueueCopyImage web_clEnqueueCopyImage
#define clEnqueueCopyImageToBuffer web_clEnqueueCopyImageToBuffer
#define clEnqueueCopyBufferToImage web_clEnqueueCopyBufferToImage
#define clEnqueueWaitForEvents web_clEnqueueWaitForEvents
#define clCreateCommandQueue web_clCreateCommandQueue
#define clGetContextInfo web_clGetContextInfo
#define clCreateProgramWithSource web_clCreateProgramWithSource
#define clCreateImage2D web_clCreateImage2D
#define clCreateFromGLBuffer web_clCreateFromGLBuffer
#define clCreateFromGLRenderbuffer web_clCreateFromGLRenderbuffer
#define clCreateSampler web_clCreateSampler
#define clCreateFromGLTexture2D web_clCreateFromGLTexture2D
#define clCreateUserEvent web_clCreateUserEvent
#define clWaitForEvents web_clWaitForEvents
#define clReleaseContext web_clReleaseContext
#define clGetSupportedImageFormats web_clGetSupportedImageFormats
#define clCreateContext web_clCreateContext
#define clCreateContextFromType web_clCreateContextFromType
#define clGetPlatformIDs web_clGetPlatformIDs
#define clUnloadCompiler web_clUnloadCompiler
#define clGetDeviceIDs web_clGetDeviceIDs
#define clGetDeviceInfo web_clGetDeviceInfo
#define clGetEventInfo web_clGetEventInfo
#define clSetUserEventStatus web_clSetUserEventStatus
#define clReleaseEvent web_clReleaseEvent
#define clSetEventCallback web_clSetEventCallback
#define clGetEventProfilingInfo web_clGetEventProfilingInfo
#define clGetGLTextureInfo web_clGetGLTextureInfo
#define clGetKernelInfo web_clGetKernelInfo
#define clGetKernelWorkGroupInfo web_clGetKernelWorkGroupInfo
#define clSetKernelArg web_clSetKernelArg
#define clReleaseKernel web_clReleaseKernel
#define clGetMemObjectInfo web_clGetMemObjectInfo
#define clReleaseMemObject web_clReleaseMemObject
#define clGetPlatformInfo web_clGetPlatformInfo
#define clGetProgramInfo web_clGetProgramInfo
#define clCreateKernel web_clCreateKernel
#define clGetProgramBuildInfo web_clGetProgramBuildInfo
#define clCreateKernelsInProgram web_clCreateKernelsInProgram
#define clBuildProgram web_clBuildProgram
#define clReleaseProgram web_clReleaseProgram
#define clGetSamplerInfo web_clGetSamplerInfo
#define clReleaseSampler web_clReleaseSampler

#endif // WebCLOpenCL_h
