#include "csb.hpp"

#include <filesystem>
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
  csb::clean_build();
  return csb::build();
}

int csb::build()
{
  if (!csb::is_subproject) csb::clang_format("21.1.8");

  csb::vcpkg_install("2025.12.12", {{"builtin-baseline", "84bab45d415d22042bd0b9081aea57f362da3f35"},
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

  auto build_include_path = csb::path("build/include/cse");
  csb::mkdir(build_include_path);
  for (const auto &file : std::filesystem::directory_iterator(build_include_path))
    if (csb::contains(csb::include_files, csb::path("program/include") / file.path().filename()))
      csb::remove(file.path());
  csb::copy(csb::include_files, build_include_path);

  csb::generate_compile_commands();
  csb::compile();
  csb::link();
  return csb::run();
}

int csb::run() { return csb::success; }

CSB_MAIN()
