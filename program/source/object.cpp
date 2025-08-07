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
                             const resource::compiled_shader &fragment_shader_,
                             const resource::compiled_texture &texture_, int frame_width_, int frame_count_,
                             int current_frame_)
    : shader(vertex_shader_, fragment_shader_), texture(texture_, frame_width_, frame_count_, current_frame_)
  {
  }

  void object::graphics::create_pipeline(SDL_Window *instance, SDL_GPUDevice *gpu)
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
    vertex_shader_info.code =
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.vertex.dxil.data() : shader.vertex.spirv.data();
    vertex_shader_info.code_size =
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.vertex.dxil.size() : shader.vertex.spirv.size();
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
    fragment_shader_info.code =
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.fragment.dxil.data() : shader.fragment.spirv.data();
    fragment_shader_info.code_size =
      current_format == SDL_GPU_SHADERFORMAT_DXIL ? shader.fragment.dxil.size() : shader.fragment.spirv.size();
    fragment_shader_info.format = current_format;
    fragment_shader_info.entrypoint = "main";
    fragment_shader_info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    fragment_shader_info.num_samplers = 1;
    fragment_shader_info.num_uniform_buffers = 0;
    fragment_shader_info.num_storage_buffers = 0;
    fragment_shader_info.num_storage_textures = 0;
    SDL_GPUShader *fragment_shader = SDL_CreateGPUShader(gpu, &fragment_shader_info);
    if (!fragment_shader) throw cse::utility::sdl_exception("Could not create fragment shader for object");

    SDL_GPUVertexBufferDescription vertex_buffer_description = {};
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_description.instance_step_rate = 0;
    vertex_buffer_description.pitch = sizeof(vertex);
    std::array<SDL_GPUVertexAttribute, 3> vertex_attributes;
    vertex_attributes.at(0).buffer_slot = 0;
    vertex_attributes.at(0).format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes.at(0).location = 0;
    vertex_attributes.at(0).offset = 0;
    vertex_attributes.at(1).buffer_slot = 0;
    vertex_attributes.at(1).format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    vertex_attributes.at(1).location = 1;
    vertex_attributes.at(1).offset = sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z);
    vertex_attributes.at(2).buffer_slot = 0;
    vertex_attributes.at(2).format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertex_attributes.at(2).location = 2;
    vertex_attributes.at(2).offset = sizeof(vertex::x) + sizeof(vertex::y) + sizeof(vertex::z) + sizeof(vertex::r) +
                                     sizeof(vertex::g) + sizeof(vertex::b) + sizeof(vertex::a);
    SDL_GPUVertexInputState vertex_input = {};
    vertex_input.num_vertex_buffers = 1;
    vertex_input.num_vertex_attributes = 3;
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
    pipeline = SDL_CreateGPUGraphicsPipeline(gpu, &pipeline_info);
    if (!pipeline) throw cse::utility::sdl_exception("Could not create graphics pipeline for object");

    SDL_ReleaseGPUShader(gpu, fragment_shader);
    SDL_ReleaseGPUShader(gpu, vertex_shader);
  }

  void object::graphics::create_vertex_and_index(SDL_GPUDevice *gpu)
  {
    SDL_GPUBufferCreateInfo vertex_buffer_info = {};
    vertex_buffer_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertex_buffer_info.size = sizeof(quad_vertices);
    vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertex_buffer_info);
    if (!vertex_buffer) throw cse::utility::sdl_exception("Could not create vertex buffer for object");

    SDL_GPUBufferCreateInfo index_buffer_info = {};
    index_buffer_info.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    index_buffer_info.size = sizeof(quad_indices);
    index_buffer = SDL_CreateGPUBuffer(gpu, &index_buffer_info);
    if (!index_buffer) throw cse::utility::sdl_exception("Could not create index buffer for object");
  }

  void object::graphics::create_sampler_and_texture(SDL_GPUDevice *gpu)
  {
    SDL_GPUSamplerCreateInfo sampler_info = {};
    sampler_info.min_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mag_filter = SDL_GPU_FILTER_NEAREST;
    sampler_info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sampler_info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sampler_buffer = SDL_CreateGPUSampler(gpu, &sampler_info);
    if (!sampler_buffer) throw cse::utility::sdl_exception("Could not create sampler for object");

    SDL_GPUTextureCreateInfo texture_info = {};
    texture_info.type = SDL_GPU_TEXTURETYPE_2D;
    texture_info.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    texture_info.width = static_cast<Uint32>(texture.raw.width);
    texture_info.height = static_cast<Uint32>(texture.raw.height);
    texture_info.layer_count_or_depth = 1;
    texture_info.num_levels = 1;
    texture_info.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    texture_buffer = SDL_CreateGPUTexture(gpu, &texture_info);
    if (!texture_buffer) throw cse::utility::sdl_exception("Could not create texture for object");
  }

  void object::graphics::transfer_vertex_and_index(SDL_GPUDevice *gpu)
  {
    SDL_GPUTransferBufferCreateInfo buffer_transfer_buffer_info = {};
    buffer_transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    buffer_transfer_buffer_info.size = sizeof(quad_vertices) + sizeof(quad_indices);
    buffer_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &buffer_transfer_buffer_info);
    if (!buffer_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for buffer object");

    auto vertex_data = reinterpret_cast<vertex *>(SDL_MapGPUTransferBuffer(gpu, buffer_transfer_buffer, false));
    if (!vertex_data) throw cse::utility::sdl_exception("Could not map vertex data for object");
    if (texture.frame_width * texture.current_frame >= texture.raw.width)
      throw cse::utility::exception("Current frame width exceeds texture width for object");
    quad_vertices = {vertex{1.0f, 1.0f, 0.0f, 0, 0, 0, 255,
                            (static_cast<float>(texture.current_frame) / static_cast<float>(texture.frame_count)) +
                              static_cast<float>(texture.frame_width) / static_cast<float>(texture.raw.width),
                            1.0f},
                     vertex{1.0f, -1.0f, 0.0f, 0, 0, 0, 255,
                            (static_cast<float>(texture.current_frame) / static_cast<float>(texture.frame_count)) +
                              static_cast<float>(texture.frame_width) / static_cast<float>(texture.raw.width),
                            0.0f},
                     vertex{-1.0f, 1.0f, 0.0f, 0, 0, 0, 255,
                            static_cast<float>(texture.current_frame) / static_cast<float>(texture.frame_count), 1.0f},
                     vertex{-1.0f, -1.0f, 0.0f, 0, 0, 0, 255,
                            static_cast<float>(texture.current_frame) / static_cast<float>(texture.frame_count), 0.0f}};
    std::copy(quad_vertices.begin(), quad_vertices.end(), vertex_data);

    auto index_data = reinterpret_cast<Uint16 *>(&vertex_data[quad_vertices.size()]);
    if (!index_data) throw cse::utility::sdl_exception("Could not map index data for object");
    quad_indices = {3, 1, 0, 3, 0, 2};
    std::copy(quad_indices.begin(), quad_indices.end(), index_data);

    SDL_UnmapGPUTransferBuffer(gpu, buffer_transfer_buffer);
  }

  void object::graphics::transfer_texture(SDL_GPUDevice *gpu)
  {
    SDL_GPUTransferBufferCreateInfo texture_transfer_buffer_info = {};
    texture_transfer_buffer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    texture_transfer_buffer_info.size = static_cast<Uint32>(texture.raw.width) *
                                        static_cast<Uint32>(texture.raw.height) *
                                        static_cast<Uint32>(texture.raw.channels);
    texture_transfer_buffer = SDL_CreateGPUTransferBuffer(gpu, &texture_transfer_buffer_info);
    if (!texture_transfer_buffer)
      throw cse::utility::sdl_exception("Could not create transfer buffer for texture for object");

    auto *texture_data = reinterpret_cast<Uint8 *>(SDL_MapGPUTransferBuffer(gpu, texture_transfer_buffer, false));
    if (!texture_data) throw cse::utility::sdl_exception("Could not map texture data for object");
    SDL_memcpy(texture_data, texture.raw.image.data(), texture.raw.width * texture.raw.height * texture.raw.channels);

    SDL_UnmapGPUTransferBuffer(gpu, texture_transfer_buffer);
  }

  void object::graphics::upload_to_gpu(SDL_GPUDevice *gpu)
  {
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(gpu);
    if (!command_buffer) throw cse::utility::sdl_exception("Could not acquire GPU command buffer for object");
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);
    if (!copy_pass) throw cse::utility::sdl_exception("Could not begin GPU copy pass for object");

    SDL_GPUTransferBufferLocation vertex_transfer_buffer_location = {};
    vertex_transfer_buffer_location.transfer_buffer = buffer_transfer_buffer;
    vertex_transfer_buffer_location.offset = 0;
    SDL_GPUBufferRegion vertex_buffer_region = {};
    vertex_buffer_region.buffer = vertex_buffer;
    vertex_buffer_region.offset = 0;
    vertex_buffer_region.size = sizeof(quad_vertices);
    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_region, false);

    SDL_GPUTransferBufferLocation index_transfer_buffer_location = {};
    index_transfer_buffer_location.transfer_buffer = buffer_transfer_buffer;
    index_transfer_buffer_location.offset = sizeof(quad_vertices);
    SDL_GPUBufferRegion index_buffer_region = {};
    index_buffer_region.buffer = index_buffer;
    index_buffer_region.offset = 0;
    index_buffer_region.size = sizeof(quad_indices);
    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

    SDL_GPUTextureTransferInfo texture_transfer_info = {};
    texture_transfer_info.transfer_buffer = texture_transfer_buffer;
    texture_transfer_info.offset = 0;
    SDL_GPUTextureRegion texture_region = {};
    texture_region.texture = texture_buffer;
    texture_region.w = static_cast<Uint32>(texture.raw.width);
    texture_region.h = static_cast<Uint32>(texture.raw.height);
    texture_region.d = 1;
    SDL_UploadToGPUTexture(copy_pass, &texture_transfer_info, &texture_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);

    SDL_ReleaseGPUTransferBuffer(gpu, texture_transfer_buffer);
    SDL_ReleaseGPUTransferBuffer(gpu, buffer_transfer_buffer);
    texture_transfer_buffer = nullptr;
    buffer_transfer_buffer = nullptr;
  }

  void object::graphics::bind_pipeline_and_buffers(SDL_GPURenderPass *render_pass)
  {
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
    SDL_GPUBufferBinding vertex_buffer_binding = {};
    vertex_buffer_binding.buffer = vertex_buffer;
    vertex_buffer_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
    SDL_GPUBufferBinding index_buffer_binding = {};
    index_buffer_binding.buffer = index_buffer;
    index_buffer_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding texture_sampler_binding = {};
    texture_sampler_binding.texture = texture_buffer;
    texture_sampler_binding.sampler = sampler_buffer;
    SDL_BindGPUFragmentSamplers(render_pass, 0, &texture_sampler_binding, 1);
  }

  void object::graphics::push_uniform_data(SDL_GPUCommandBuffer *command_buffer, const glm::mat4 &model_matrix,
                                           const glm::mat4 &projection_matrix, const glm::mat4 &view_matrix)
  {
    std::array<glm::mat4, 3> matrices = {projection_matrix, view_matrix, model_matrix};
    SDL_PushGPUVertexUniformData(command_buffer, 0, &matrices, sizeof(matrices));
  }

  void object::graphics::draw_primitives(SDL_GPURenderPass *render_pass)
  {
    SDL_DrawGPUIndexedPrimitives(render_pass, 6, 1, 0, 0, 0);
  }

  void object::graphics::cleanup_object(SDL_GPUDevice *gpu)
  {
    SDL_ReleaseGPUSampler(gpu, sampler_buffer);
    SDL_ReleaseGPUTexture(gpu, texture_buffer);
    SDL_ReleaseGPUBuffer(gpu, index_buffer);
    SDL_ReleaseGPUBuffer(gpu, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(gpu, pipeline);

    sampler_buffer = nullptr;
    texture_buffer = nullptr;
    index_buffer = nullptr;
    vertex_buffer = nullptr;
    pipeline = nullptr;
  }

  object::object(const glm::vec3 &translation_, const glm::vec3 &rotation_, const glm::vec3 &scale_,
                 const resource::compiled_shader &vertex_shader_, const resource::compiled_shader &fragment_shader_,
                 const resource::compiled_texture &texture_, int frame_width_, int frame_count_, int current_frame_)
    : transform(translation_, rotation_, scale_),
      graphics(vertex_shader_, fragment_shader_, texture_, frame_width_, frame_count_, current_frame_)
  {
  }

  object::~object()
  {
    handle_simulate = nullptr;
    handle_input = nullptr;
  }

  void object::initialize(SDL_Window *instance, SDL_GPUDevice *gpu)
  {
    graphics.create_pipeline(instance, gpu);
    graphics.create_vertex_and_index(gpu);
    graphics.create_sampler_and_texture(gpu);
    graphics.transfer_vertex_and_index(gpu);
    graphics.transfer_texture(gpu);
    graphics.upload_to_gpu(gpu);
  }

  void object::cleanup(SDL_GPUDevice *gpu) { graphics.cleanup_object(gpu); }

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
    graphics.bind_pipeline_and_buffers(render_pass);

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, transform.translation.interpolated);
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model_matrix =
      glm::rotate(model_matrix, glm::radians(transform.rotation.interpolated.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = glm::scale(model_matrix, transform.scale.interpolated);
    graphics.push_uniform_data(command_buffer, model_matrix, projection_matrix, view_matrix);

    graphics.draw_primitives(render_pass);
  }
}
