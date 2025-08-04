#include "object.hpp"

#include <algorithm>
#include <array>

#include "SDL3/SDL_gpu.h"
#include "SDL3/SDL_stdinc.h"
#include "SDL3/SDL_video.h"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/trigonometric.hpp"

#include "exception.hpp"
#include "resource.hpp"

namespace cse::base
{
  object::transform::property::property(const glm::vec3 &value_)
    : value(value_), velocity(glm::vec3(0.0f, 0.0f, 0.0f)), acceleration(glm::vec3(0.0f, 0.0f, 0.0f)), previous(value_),
      interpolated(value_)
  {
  }

  glm::vec3 object::transform::property::get_previous() const { return previous; }

  glm::vec3 object::transform::property::get_interpolated() const { return interpolated; }

  void object::transform::property::update_previous() { previous = value; }

  void object::transform::property::update_interpolated(float simulation_alpha)
  {
    interpolated = previous + ((value - previous) * simulation_alpha);
  }

  object::transform::transform(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_)
    : translation(translation_), rotation(rotation_), scale(scale_)
  {
  }

  object::graphics::graphics(const resource::compiled_shader &vertex_shader_,
                             const resource::compiled_shader &fragment_shader_)
    : shader(vertex_shader_, fragment_shader_)
  {
  }

  object::object(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_,
                 const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_)
    : transform(translation_, rotation_, scale_), graphics(vertex_shader_, fragment_shader_)
  {
  }

  object::~object()
  {
    graphics.index_buffer = nullptr;
    graphics.vertex_buffer = nullptr;
    graphics.pipeline = nullptr;
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  auto object::get_graphics() -> struct graphics const { return graphics; }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    SDL_GPUShaderFormat current_format = SDL_GPU_SHADERFORMAT_INVALID;
    const SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(gpu);
    if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
      current_format = SDL_GPU_SHADERFORMAT_SPIRV;
    else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
      current_format = SDL_GPU_SHADERFORMAT_DXIL;
    else
      throw cse::utility::sdl_exception("Could not find supported shader format for object");

    SDL_GPUShaderCreateInfo vertex_shader_info = {};
    vertex_shader_info.code = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.vertex.dxil.data()
                                                                          : graphics.shader.vertex.spirv.data();
    vertex_shader_info.code_size = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.vertex.dxil.size()
                                                                               : graphics.shader.vertex.spirv.size();
    vertex_shader_info.format = current_format;
    vertex_shader_info.entrypoint = "main";
    vertex_shader_info.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertex_shader_info.num_samplers = 0;
    vertex_shader_info.num_uniform_buffers = 1;
    vertex_shader_info.num_storage_buffers = 0;
    vertex_shader_info.num_storage_textures = 0;
    SDL_GPUShader *vertex_shader = SDL_CreateGPUShader(gpu, &vertex_shader_info);
    if (!vertex_shader) throw cse::utility::sdl_exception("Could not create vertex shader for object");

    SDL_GPUShaderCreateInfo fragment_shader_info = {};
    fragment_shader_info.code = current_format == SDL_GPU_SHADERFORMAT_DXIL ? graphics.shader.fragment.dxil.data()
                                                                            : graphics.shader.fragment.spirv.data();
    fragment_shader_info.code_size = current_format == SDL_GPU_SHADERFORMAT_DXIL
                                       ? graphics.shader.fragment.dxil.size()
                                       : graphics.shader.fragment.spirv.size();
    fragment_shader_info.format = current_format;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 0;
    fragment_shader_info.num_uniform_buffers = 0;
    fragment_shader_info.num_storage_buffers = 0;
    fragment_shader_info.num_storage_textures = 0;
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &fragment_shader_info);
    if (!fragment_shader) throw cse::utility::sdl_exception("Could not create fragment shader for object");

    SDL_GPUVertexBufferDescription vertex_buffer_description = {};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_description.instance_step_rate = 0;
    vertex_buffer_description.pitch = sizeof(graphics::position_color_vertex);
    std::array<SDL_GPUVertexAttribute, 2> vertex_attributes;
    vertex_attributes.at(0).buffer_slot = 0;
    vertex_attributes.at(0).format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes.at(0).location = 0;
    vertex_attributes.at(0).offset = 0;
    vertex_attributes.at(1).buffer_slot = 0;
    vertex_attributes.at(1).format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    vertex_attributes.at(1).location = 1;
    vertex_attributes.at(1).offset = sizeof(float) * 3;
    SDL_GPUVertexInputState vertex_input = {};
    vertex_input.num_vertex_buffers = 1;
    vertex_input.num_vertex_attributes = 2;
    vertex_input.vertex_buffer_descriptions = &vertex_buffer_description;
    vertex_input.vertex_attributes = vertex_attributes.data();

    SDL_GPUColorTargetDescription color_target_description = {};
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(gpu, instance);
    SDL_GPUGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.vertex_shader = vertex_shader;
    pipeline_info.fragment_shader = fragment_shader;
    pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipeline_info.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;
    pipeline_info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipeline_info.vertex_input_state = vertex_input;
    pipeline_info.target_info.num_color_targets = 1;
    pipeline_info.target_info.color_target_descriptions = &color_target_description;
    graphics.pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!graphics.pipeline) throw cse::utility::sdl_exception("Could not create graphics pipeline for object");
    SDL_ReleaseGPUShader(gpu, vertex_shader);
    SDL_ReleaseGPUShader(gpu, fragment_shader);

    SDL_GPUBufferCreateInfo vertex_buffer_info = {};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = sizeof(graphics::position_color_vertex) * 4;
    graphics.vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!graphics.vertex_buffer) throw cse::utility::sdl_exception("Could not create vertex buffer for object");
    SDL_GPUBufferCreateInfo index_buffer_info = {};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = sizeof(Uint16) * 6;
    graphics.index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!graphics.index_buffer) throw cse::utility::sdl_exception("Could not create index buffer for object");

    SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {};
    transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_info.size = (sizeof(graphics::position_color_vertex) * 4) + (sizeof(Uint16) * 6);
    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &transfer_buffer_info);
    if (!transfer_buffer) throw cse::utility::sdl_exception("Could not create transfer buffer for object");
    auto vertex_data =
      reinterpret_cast<graphics::position_color_vertex *>(SDL_MapGPUTransferBuffer(gpu, transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    std::copy(graphics::default_quad_vertices.begin(), graphics::default_quad_vertices.end(), vertex_data);
    auto index_data = reinterpret_cast<Uint16 *>(&vertex_data[graphics::default_quad_vertices.size()]);
    if (!index_data) throw cse::utility::sdl_exception("Could not map index data for object");
    std::copy(graphics::default_quad_indices.begin(), graphics::default_quad_indices.end(), index_data);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");
    SDL_GPUTransferBufferLocation vertex_transfer_buffer_location = {};
    vertex_transfer_buffer_location.transfer_buffer = transfer_buffer;
    vertex_transfer_buffer_location.offset = 0;
    SDL_GPUBufferRegion vertex_buffer_region = {};
    vertex_buffer_region.buffer = graphics.vertex_buffer;
    vertex_buffer_region.offset = 0;
    vertex_buffer_region.size = sizeof(graphics::position_color_vertex) * 4;
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_region, false);
    SDL_GPUTransferBufferLocation index_transfer_buffer_location = {};
    index_transfer_buffer_location.transfer_buffer = transfer_buffer;
    index_transfer_buffer_location.offset = sizeof(graphics::position_color_vertex) * 4;
    SDL_GPUBufferRegion index_buffer_region = {};
    index_buffer_region.buffer = graphics.index_buffer;
    index_buffer_region.offset = 0;
    index_buffer_region.size = sizeof(Uint16) * 6;
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buffer);
  }

  void object::cleanup(SDL_GPUDevice *gpu)
  {
    SDL_ReleaseGPUBuffer(gpu, graphics.index_buffer);
    SDL_ReleaseGPUBuffer(gpu, graphics.vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, graphics.pipeline);
  }

  void object::input(const bool *key_state)
  {
    if (handle_input) handle_input(key_state);
  }

  void object::simulate(double simulation_alpha)
  {
    transform.translation.update_previous();
    transform.rotation.update_previous();
    transform.scale.update_previous();

    if (handle_simulate) handle_simulate();

    transform.translation.update_interpolated(static_cast<float>(simulation_alpha));
    transform.rotation.update_interpolated(static_cast<float>(simulation_alpha));
    transform.scale.update_interpolated(static_cast<float>(simulation_alpha));
  }

  void object::render(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass,
                      const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix)
  {
    SDL_BindGPUGraphicsPipeline(render_pass, graphics.pipeline);
    SDL_GPUBufferBinding buffer_binding = {};
    buffer_binding.buffer = graphics.vertex_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &buffer_binding, 1);
    buffer_binding.buffer = graphics.index_buffer;
    buffer_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, transform.translation.get_interpolated());
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.get_interpolated().x), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.get_interpolated().y), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.get_interpolated().z), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = glm::scale(model_matrix, transform.scale.get_interpolated());
    std::array<glm::mat4, 3> matrices = {projection_matrix, view_matrix, model_matrix};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));

    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
  }
}
