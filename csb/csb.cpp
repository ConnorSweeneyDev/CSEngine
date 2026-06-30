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
  csb::vcpkg_install("2026.04.27", {{"builtin-baseline", "56bb2411609227288b70117ead2c47585ba07713"},
                                    {"dependencies",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"default-features", false},
                                         {"features",
                                          {
                                            "vulkan",
                                            {{"name", "x11"}, {"platform", "linux"}},
                                            {{"name", "wayland"}, {"platform", "linux"}},
                                          }},
                                       },
                                       "sdl3-ttf",
                                       {
                                         {"name", "sdl3-mixer"},
                                         {"default-features", false},
                                         {"features",
                                          {
                                            "opusfile",
                                          }},
                                       },
                                       "glm",
                                       "nlohmann-json",
                                     }},
                                    {"overrides",
                                     {
                                       {
                                         {"name", "sdl3"},
                                         {"version", "3.4.4"},
                                       },
                                       {
                                         {"name", "sdl3-ttf"},
                                         {"version", "3.2.2#1"},
                                       },
                                       {
                                         {"name", "sdl3-mixer"},
                                         {"version", "3.2.0#2"},
                                       },
                                       {
                                         {"name", "glm"},
                                         {"version", "1.0.1#3"},
                                       },
                                       {
                                         {"name", "nlohmann-json"},
                                         {"version", "3.12.0#2"},
                                       },
                                     }}});

  csb::subproject_install({"ConnorSweeneyDev/CSPack", "1.0.0", HEADER_LIBRARY});

  const auto build_include_path = csb::path("build/include");
  const auto cse_include_path = build_include_path / "cse";
  if (!csb::exists(cse_include_path)) csb::mkdir(cse_include_path);
  for (const auto &file : csb::directory(cse_include_path))
    if (!csb::contains(csb::include_files, csb::path("program/include") / file.path().filename()))
      csb::remove(file.path());
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"), csb::include_files,
                      {cse_include_path / "(filename)"});

  std::vector<std::filesystem::path> external_includes{};
  for (const auto &entry : csb::directory(csb::vcpkg_include())) external_includes.push_back(entry.path());
  for (const auto &entry : csb::directory(csb::subproject_include("CSPack"))) external_includes.push_back(entry.path());
  csb::multi_task_run(
    [](const std::filesystem::path &source, const std::vector<std::filesystem::path> &,
       const std::vector<std::filesystem::path> &) -> std::string
    {
      if (csb::host_platform == WINDOWS)
        return std::filesystem::is_directory(source) ? "xcopy /E /I /Y /Q () []" : "copy /Y () []";
      return std::filesystem::is_directory(source) ? "cp -rT () []" : "cp () []";
    },
    external_includes, {build_include_path / "(filename)"});

  const auto build_library_path{csb::path("build") / (csb::target_configuration == RELEASE ? "release" : "debug")};
  if (!csb::exists(build_library_path)) csb::mkdir(build_library_path);
  std::vector<std::filesystem::path> external_libraries{};
  for (const auto &entry : csb::directory_recurse(csb::vcpkg_library()))
    if (entry.is_regular_file() && entry.path().extension() == (csb::host_platform == WINDOWS ? ".lib" : ".a"))
      external_libraries.push_back(entry.path());
  csb::multi_task_run(std::format("{} () []", csb::host_platform == WINDOWS ? "copy /Y" : "cp"), external_libraries,
                      {build_library_path / "(filename)"});

  if (!csb::is_subproject)
  {
    csb::generate_clang_format({{"BasedOnStyle", "LLVM"},
                                {"ColumnLimit", "120"},
                                {"IndentWidth", "2"},
                                {"ConstructorInitializerIndentWidth", "2"},
                                {"ContinuationIndentWidth", "2"},
                                {"Language", "Cpp"},
                                {"BreakBeforeBraces", "Allman"},
                                {"AllowShortBlocksOnASingleLine", "true"},
                                {"AllowShortIfStatementsOnASingleLine", "true"},
                                {"AllowShortCaseLabelsOnASingleLine", "true"},
                                {"AllowShortLoopsOnASingleLine", "true"},
                                {"AllowShortFunctionsOnASingleLine", "true"},
                                {"AllowShortLambdasOnASingleLine", "true"},
                                {"AllowShortEnumsOnASingleLine", "true"},
                                {"AllowShortNamespacesOnASingleLine", "true"},
                                {"BreakTemplateDeclarations", "No"},
                                {"IndentPPDirectives", "BeforeHash"},
                                {"IndentCaseLabels", "true"},
                                {"NamespaceIndentation", "All"},
                                {"FixNamespaceComments", "false"}});
    csb::format("22.1.5");
  }
  csb::generate_clangd({{"Diagnostics", {{"UnusedIncludes", "Strict"}, {"MissingIncludes", "Strict"}}}});
  csb::generate_compile_commands();

  csb::compile();
  csb::link();
  return csb::run();
}

int csb::run() { return csb::success; }

CSB_MAIN()
