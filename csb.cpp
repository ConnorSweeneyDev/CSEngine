#include "csb.hpp"

#include <filesystem>
#include <format>
#include <string>
#include <vector>

void csb::configure()
{
  csb::target_name = "cse";
  csb::target_artifact = STATIC_LIBRARY;
  csb::target_linkage = STATIC;
  csb::target_subsystem = CONSOLE;
  csb::target_configuration = RELEASE;
  csb::cxx_standard = CXX20;
  csb::warning_level = W4;
  csb::include_files = csb::choose_files({"program/include"});
  csb::source_files = csb::choose_files({"program/source"});
}

int csb::clean()
{
  csb::clean_build_directory();
  return csb::build();
}

int csb::build()
{
  if (!csb::is_subproject) csb::clang_format("21.1.8");

  csb::vcpkg_install("2025.08.27", {{"builtin-baseline", "120deac3062162151622ca4860575a33844ba10b"},
                                    {"dependencies",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"features", {"vulkan"}},
                                       },
                                       "glm",
                                     }},
                                    {"overrides",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"version", "3.2.18"},
                                       },
                                       {
                                         {"name", "glm"},
                                         {"version", "1.0.1#3"},
                                       },
                                     }}});

  if (!std::filesystem::exists("build/include")) std::filesystem::create_directories("build/include");
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"),
                      csb::choose_files({"program/include"}), {"build/include/cse/(filename)"});

  csb::generate_compile_commands();
  csb::compile();
  csb::link();
  return csb::run();
}

int csb::run() { return CSB_SUCCESS; }

CSB_MAIN()
